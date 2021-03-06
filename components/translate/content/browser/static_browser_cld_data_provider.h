// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CONTENT_BROWSER_STATIC_BROWSER_CLD_DATA_PROVIDER_H_
#define COMPONENTS_TRANSLATE_CONTENT_BROWSER_STATIC_BROWSER_CLD_DATA_PROVIDER_H_

#include "base/macros.h"
#include "components/translate/content/browser/browser_cld_data_provider.h"

namespace translate {

class StaticBrowserCldDataProvider : public BrowserCldDataProvider {
 public:
  explicit StaticBrowserCldDataProvider();
  ~StaticBrowserCldDataProvider() override;
  // BrowserCldDataProvider implementations:
  bool OnMessageReceived(const IPC::Message&) override;
  void OnCldDataRequest() override;
  void SendCldDataResponse() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(StaticBrowserCldDataProvider);
};

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CONTENT_BROWSER_STATIC_BROWSER_CLD_DATA_PROVIDER_H_
