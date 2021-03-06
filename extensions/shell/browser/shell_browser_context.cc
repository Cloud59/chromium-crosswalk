// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_browser_context.h"

#include "base/command_line.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/guest_view/guest_view_manager.h"
#include "extensions/shell/browser/shell_network_delegate.h"
#include "extensions/shell/browser/shell_special_storage_policy.h"
#include "extensions/shell/browser/shell_url_request_context_getter.h"

namespace extensions {

namespace {

bool IgnoreCertificateErrors() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      ::switches::kIgnoreCertificateErrors);
}

}  // namespace

// Create a normal recording browser context. If we used an incognito context
// then app_shell would also have to create a normal context and manage both.
ShellBrowserContext::ShellBrowserContext(net::NetLog* net_log)
    : content::ShellBrowserContext(false, NULL),
      net_log_(net_log),
      storage_policy_(new ShellSpecialStoragePolicy) {
}

ShellBrowserContext::~ShellBrowserContext() {
}

content::BrowserPluginGuestManager* ShellBrowserContext::GetGuestManager() {
  return GuestViewManager::FromBrowserContext(this);
}

storage::SpecialStoragePolicy* ShellBrowserContext::GetSpecialStoragePolicy() {
  return storage_policy_.get();
}

net::URLRequestContextGetter* ShellBrowserContext::CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors,
      InfoMap* extension_info_map) {
  DCHECK(!url_request_context_getter());
  set_url_request_context_getter(
      new ShellURLRequestContextGetter(
          this,
          IgnoreCertificateErrors(),
          GetPath(),
          content::BrowserThread::UnsafeGetMessageLoopForThread(
              content::BrowserThread::IO),
          content::BrowserThread::UnsafeGetMessageLoopForThread(
              content::BrowserThread::FILE),
          protocol_handlers,
          request_interceptors.Pass(),
          net_log_,
          extension_info_map));
  resource_context_->set_url_request_context_getter(
      url_request_context_getter());
  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(
          &ShellBrowserContext::InitURLRequestContextOnIOThread,
          base::Unretained(this)));
  return url_request_context_getter();
}

void ShellBrowserContext::InitURLRequestContextOnIOThread() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  // GetURLRequestContext() will create a URLRequestContext if it isn't
  // initialized.
  url_request_context_getter()->GetURLRequestContext();
}

void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext1() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext2() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext3() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext4() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext5() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext6() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext7() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext8() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext9() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext10() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext11() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext12() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext13() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext14() {
  NOTREACHED();
}
void ShellBrowserContext::ProfileFunctionCallOnNonProfileBrowserContext15() {
  NOTREACHED();
}

}  // namespace extensions
