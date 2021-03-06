// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_REMOTE_IMPL_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_REMOTE_IMPL_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/test/chromedriver/chrome/chrome_impl.h"

class DevToolsClient;
class DevToolsHttpClient;

class ChromeRemoteImpl : public ChromeImpl {
 public:
  ChromeRemoteImpl(
      scoped_ptr<DevToolsHttpClient> http_client,
      scoped_ptr<DevToolsClient> websocket_client,
      ScopedVector<DevToolsEventListener>& devtools_event_listeners);
  ~ChromeRemoteImpl() override;

  // Overridden from Chrome.
  std::string GetOperatingSystemName() override;

  // Overridden from ChromeImpl.
  Status QuitImpl() override;
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_CHROME_REMOTE_IMPL_H_
