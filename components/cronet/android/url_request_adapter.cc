// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url_request_adapter.h"

#include <string.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "components/cronet/android/url_request_context_adapter.h"
#include "components/cronet/android/wrapped_channel_upload_element_reader.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"

namespace cronet {

static const size_t kBufferSizeIncrement = 8192;

URLRequestAdapter::URLRequestAdapter(URLRequestContextAdapter* context,
                                     URLRequestAdapterDelegate* delegate,
                                     GURL url,
                                     net::RequestPriority priority)
    : method_("GET"),
      read_buffer_(new net::GrowableIOBuffer()),
      bytes_read_(0),
      total_bytes_read_(0),
      error_code_(0),
      http_status_code_(0),
      canceled_(false),
      expected_size_(0),
      chunked_upload_(false),
      disable_redirect_(false) {
  context_ = context;
  delegate_ = delegate;
  url_ = url;
  priority_ = priority;
}

URLRequestAdapter::~URLRequestAdapter() {
  DCHECK(OnNetworkThread());
  CHECK(url_request_ == NULL);
}

void URLRequestAdapter::SetMethod(const std::string& method) {
  method_ = method;
}

void URLRequestAdapter::AddHeader(const std::string& name,
                                  const std::string& value) {
  headers_.SetHeader(name, value);
}

void URLRequestAdapter::SetUploadContent(const char* bytes, int bytes_len) {
  std::vector<char> data(bytes, bytes + bytes_len);
  scoped_ptr<net::UploadElementReader> reader(
      new net::UploadOwnedBytesElementReader(&data));
  upload_data_stream_ =
      net::ElementsUploadDataStream::CreateWithReader(reader.Pass(), 0);
}

void URLRequestAdapter::SetUploadChannel(JNIEnv* env, int64 content_length) {
  scoped_ptr<net::UploadElementReader> reader(
      new WrappedChannelElementReader(delegate_, content_length));
  upload_data_stream_ =
      net::ElementsUploadDataStream::CreateWithReader(reader.Pass(), 0);
}

void URLRequestAdapter::DisableRedirects() {
  disable_redirect_ = true;
}

void URLRequestAdapter::EnableChunkedUpload() {
  chunked_upload_ = true;
}

void URLRequestAdapter::AppendChunk(const char* bytes, int bytes_len,
                                    bool is_last_chunk) {
  VLOG(1) << "AppendChunk, len: " << bytes_len << ", last: " << is_last_chunk;
  scoped_ptr<char[]> buf(new char[bytes_len]);
  memcpy(buf.get(), bytes, bytes_len);
  context_->PostTaskToNetworkThread(
      FROM_HERE,
      base::Bind(&URLRequestAdapter::OnAppendChunk,
                 base::Unretained(this),
                 Passed(buf.Pass()),
                 bytes_len,
                 is_last_chunk));
}

std::string URLRequestAdapter::GetHeader(const std::string& name) const {
  std::string value;
  if (url_request_ != NULL) {
    url_request_->GetResponseHeaderByName(name, &value);
  }
  return value;
}

net::HttpResponseHeaders* URLRequestAdapter::GetResponseHeaders() const {
  if (url_request_ == NULL) {
    return NULL;
  }
  return url_request_->response_headers();
}

std::string URLRequestAdapter::GetNegotiatedProtocol() const {
  if (url_request_ == NULL)
    return std::string();
  return url_request_->response_info().npn_negotiated_protocol;
}

void URLRequestAdapter::Start() {
  context_->PostTaskToNetworkThread(
      FROM_HERE,
      base::Bind(&URLRequestAdapter::OnInitiateConnection,
                 base::Unretained(this)));
}

void URLRequestAdapter::OnAppendChunk(const scoped_ptr<char[]> bytes,
                                      int bytes_len, bool is_last_chunk) {
  DCHECK(OnNetworkThread());
  url_request_->AppendChunkToUpload(bytes.get(), bytes_len, is_last_chunk);
}

void URLRequestAdapter::OnInitiateConnection() {
  DCHECK(OnNetworkThread());
  if (canceled_) {
    return;
  }

  VLOG(1) << "Starting chromium request: "
          << url_.possibly_invalid_spec().c_str()
          << " priority: " << RequestPriorityToString(priority_);
  url_request_ = context_->GetURLRequestContext()->CreateRequest(
      url_, net::DEFAULT_PRIORITY, this, NULL);
  url_request_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES);
  url_request_->set_method(method_);
  url_request_->SetExtraRequestHeaders(headers_);
  if (!headers_.HasHeader(net::HttpRequestHeaders::kUserAgent)) {
    std::string user_agent;
    user_agent = context_->GetUserAgent(url_);
    url_request_->SetExtraRequestHeaderByName(
        net::HttpRequestHeaders::kUserAgent, user_agent, true /* override */);
  }

  if (upload_data_stream_) {
    url_request_->set_upload(upload_data_stream_.Pass());
  } else if (chunked_upload_) {
    url_request_->EnableChunkedUpload();
  }

  url_request_->SetPriority(priority_);

  url_request_->Start();
}

void URLRequestAdapter::Cancel() {
  if (canceled_) {
    return;
  }

  canceled_ = true;

  context_->PostTaskToNetworkThread(
      FROM_HERE,
      base::Bind(&URLRequestAdapter::OnCancelRequest, base::Unretained(this)));
}

void URLRequestAdapter::OnCancelRequest() {
  DCHECK(OnNetworkThread());
  VLOG(1) << "Canceling chromium request: " << url_.possibly_invalid_spec();

  if (url_request_ != NULL) {
    url_request_->Cancel();
  }

  OnRequestCanceled();
}

void URLRequestAdapter::Destroy() {
  context_->PostTaskToNetworkThread(
      FROM_HERE, base::Bind(&URLRequestAdapter::OnDestroyRequest, this));
}

// static
void URLRequestAdapter::OnDestroyRequest(URLRequestAdapter* self) {
  DCHECK(self->OnNetworkThread());
  VLOG(1) << "Destroying chromium request: "
          << self->url_.possibly_invalid_spec();
  delete self;
}

// static
void URLRequestAdapter::OnResponseStarted(net::URLRequest* request) {
  DCHECK(OnNetworkThread());
  if (request->status().status() != net::URLRequestStatus::SUCCESS) {
    OnRequestFailed();
    return;
  }

  http_status_code_ = request->GetResponseCode();
  VLOG(1) << "Response started with status: " << http_status_code_;

  net::HttpResponseHeaders* headers = request->response_headers();
  if (headers)
    http_status_text_ = headers->GetStatusText();

  request->GetResponseHeaderByName("Content-Type", &content_type_);
  expected_size_ = request->GetExpectedContentSize();
  delegate_->OnResponseStarted(this);

  Read();
}

// Reads all available data or starts an asynchronous read.
void URLRequestAdapter::Read() {
  DCHECK(OnNetworkThread());
  while (true) {
    if (read_buffer_->RemainingCapacity() == 0) {
      int new_capacity = read_buffer_->capacity() + kBufferSizeIncrement;
      read_buffer_->SetCapacity(new_capacity);
    }

    int bytes_read;
    if (url_request_->Read(read_buffer_.get(),
                           read_buffer_->RemainingCapacity(),
                           &bytes_read)) {
      if (bytes_read == 0) {
        OnRequestSucceeded();
        break;
      }

      VLOG(1) << "Synchronously read: " << bytes_read << " bytes";
      OnBytesRead(bytes_read);
    } else if (url_request_->status().status() ==
               net::URLRequestStatus::IO_PENDING) {
      if (bytes_read_ != 0) {
        VLOG(1) << "Flushing buffer: " << bytes_read_ << " bytes";

        delegate_->OnBytesRead(this);
        read_buffer_->set_offset(0);
        bytes_read_ = 0;
      }
      VLOG(1) << "Started async read";
      break;
    } else {
      OnRequestFailed();
      break;
    }
  }
}

void URLRequestAdapter::OnReadCompleted(net::URLRequest* request,
                                        int bytes_read) {
  DCHECK(OnNetworkThread());
  VLOG(1) << "Asynchronously read: " << bytes_read << " bytes";
  if (bytes_read < 0) {
    OnRequestFailed();
    return;
  } else if (bytes_read == 0) {
    OnRequestSucceeded();
    return;
  }

  OnBytesRead(bytes_read);
  Read();
}

void URLRequestAdapter::OnReceivedRedirect(net::URLRequest* request,
                                           const net::RedirectInfo& info,
                                           bool* defer_redirect) {
  DCHECK(OnNetworkThread());
  if (disable_redirect_) {
    http_status_code_ = request->GetResponseCode();
    request->CancelWithError(net::ERR_TOO_MANY_REDIRECTS);
    error_code_ = net::ERR_TOO_MANY_REDIRECTS;
    canceled_ = true;
    *defer_redirect = false;
    OnRequestCompleted();
  }
}

void URLRequestAdapter::OnBytesRead(int bytes_read) {
  DCHECK(OnNetworkThread());
  read_buffer_->set_offset(read_buffer_->offset() + bytes_read);
  bytes_read_ += bytes_read;
  total_bytes_read_ += bytes_read;
}

void URLRequestAdapter::OnRequestSucceeded() {
  DCHECK(OnNetworkThread());
  if (canceled_) {
    return;
  }

  VLOG(1) << "Request completed with HTTP status: " << http_status_code_
          << ". Total bytes read: " << total_bytes_read_;

  OnRequestCompleted();
}

void URLRequestAdapter::OnRequestFailed() {
  DCHECK(OnNetworkThread());
  if (canceled_) {
    return;
  }

  error_code_ = url_request_->status().error();
  VLOG(1) << "Request failed with status: " << url_request_->status().status()
          << " and error: " << net::ErrorToString(error_code_);
  OnRequestCompleted();
}

void URLRequestAdapter::OnRequestCanceled() {
  DCHECK(OnNetworkThread());
  OnRequestCompleted();
}

void URLRequestAdapter::OnRequestCompleted() {
  DCHECK(OnNetworkThread());
  VLOG(1) << "Completed: " << url_.possibly_invalid_spec();

  delegate_->OnBytesRead(this);
  delegate_->OnRequestFinished(this);
  url_request_.reset();
}

unsigned char* URLRequestAdapter::Data() const {
  DCHECK(OnNetworkThread());
  return reinterpret_cast<unsigned char*>(read_buffer_->StartOfBuffer());
}

bool URLRequestAdapter::OnNetworkThread() const {
  return context_->GetNetworkTaskRunner()->BelongsToCurrentThread();
}

}  // namespace cronet
