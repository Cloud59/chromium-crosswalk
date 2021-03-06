// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_INTERNAL_API_PUBLIC_ATTACHMENTS_ATTACHMENT_DOWNLOADER_IMPL_H_
#define SYNC_INTERNAL_API_PUBLIC_ATTACHMENTS_ATTACHMENT_DOWNLOADER_IMPL_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/threading/non_thread_safe.h"
#include "google_apis/gaia/oauth2_token_service_request.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "sync/internal_api/public/attachments/attachment_downloader.h"
#include "url/gurl.h"

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace syncer {

// An implementation of AttachmentDownloader.
class AttachmentDownloaderImpl : public AttachmentDownloader,
                                 public OAuth2TokenService::Consumer,
                                 public net::URLFetcherDelegate,
                                 public base::NonThreadSafe {
 public:
  // |sync_service_url| is the URL of the sync service.
  //
  // |url_request_context_getter| provides a URLRequestContext.
  //
  // |account_id| is the account id to use for downloads.
  //
  // |scopes| is the set of scopes to use for downloads.
  //
  // |token_service_provider| provides an OAuth2 token service.
  AttachmentDownloaderImpl(
      const GURL& sync_service_url,
      const scoped_refptr<net::URLRequestContextGetter>&
          url_request_context_getter,
      const std::string& account_id,
      const OAuth2TokenService::ScopeSet& scopes,
      const scoped_refptr<OAuth2TokenServiceRequest::TokenServiceProvider>&
          token_service_provider);
  ~AttachmentDownloaderImpl() override;

  // AttachmentDownloader implementation.
  void DownloadAttachment(const AttachmentId& attachment_id,
                          const DownloadCallback& callback) override;

  // OAuth2TokenService::Consumer implementation.
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(AttachmentDownloaderImplTest, ExtractCrc32c_First);
  FRIEND_TEST_ALL_PREFIXES(AttachmentDownloaderImplTest, ExtractCrc32c_None);
  FRIEND_TEST_ALL_PREFIXES(AttachmentDownloaderImplTest, ExtractCrc32c_Empty);

  struct DownloadState;
  typedef std::string AttachmentUrl;
  typedef base::ScopedPtrHashMap<AttachmentUrl, DownloadState> StateMap;
  typedef std::vector<DownloadState*> StateList;

  scoped_ptr<net::URLFetcher> CreateFetcher(const AttachmentUrl& url,
                                            const std::string& access_token);
  void RequestAccessToken(DownloadState* download_state);
  void ReportResult(
      const DownloadState& download_state,
      const DownloadResult& result,
      const scoped_refptr<base::RefCountedString>& attachment_data);

  // Verify the integrity of |data| using the hash received in |fetcher|.
  //
  // Assumes that the request in |fetcher| has completed.
  //
  // Returns true if the hash of |data| matches the hash contained in |fetcher|
  // or if |fetcher| contains no hash (no hash, no problem).
  static bool VerifyHashIfPresent(const net::URLFetcher& fetcher,
                                  const std::string& data);

  // Extract the crc32c from an X-Goog-Hash header in |headers|.
  //
  // Return true if a crc32c was found and set |crc32c|.
  SYNC_EXPORT_PRIVATE static bool ExtractCrc32c(
      const net::HttpResponseHeaders& headers,
      std::string* crc32c);

  GURL sync_service_url_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  std::string account_id_;
  OAuth2TokenService::ScopeSet oauth2_scopes_;
  scoped_refptr<OAuth2TokenServiceRequest::TokenServiceProvider>
      token_service_provider_;
  scoped_ptr<OAuth2TokenService::Request> access_token_request_;

  StateMap state_map_;
  // |requests_waiting_for_access_token_| only keeps references to DownloadState
  // objects while access token request is pending. It doesn't control objects'
  // lifetime.
  StateList requests_waiting_for_access_token_;

  DISALLOW_COPY_AND_ASSIGN(AttachmentDownloaderImpl);
};

}  // namespace syncer

#endif  // SYNC_INTERNAL_API_PUBLIC_ATTACHMENTS_ATTACHMENT_DOWNLOADER_IMPL_H_
