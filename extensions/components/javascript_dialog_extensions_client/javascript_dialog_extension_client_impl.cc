// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/components/javascript_dialog_extensions_client/javascript_dialog_extension_client_impl.h"

#include "components/app_modal_dialogs/javascript_dialog_extensions_client.h"
#include "components/app_modal_dialogs/javascript_dialog_manager.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "ui/gfx/native_widget_types.h"

namespace {

using extensions::Extension;

// Returns the ProcessManager for the browser context from |web_contents|.
extensions::ProcessManager* GetProcessManager(
    content::WebContents* web_contents) {
  return extensions::ProcessManager::Get(web_contents->GetBrowserContext());
}

// Returns the extension associated with |web_contents| or NULL if there is no
// associated extension (or extensions are not supported).
const Extension* GetExtensionForWebContents(
    content::WebContents* web_contents) {
  extensions::ProcessManager* pm = GetProcessManager(web_contents);
  return pm->GetExtensionForRenderViewHost(web_contents->GetRenderViewHost());
}

class JavaScriptDialogExtensionsClientImpl
    : public JavaScriptDialogExtensionsClient {
 public:
  JavaScriptDialogExtensionsClientImpl() {}
  ~JavaScriptDialogExtensionsClientImpl() override {}

  // JavaScriptDialogExtensionsClient:
  void OnDialogOpened(content::WebContents* web_contents) override {
    const Extension* extension = GetExtensionForWebContents(web_contents);
    if (extension == nullptr)
      return;

    DCHECK(web_contents);
    extensions::ProcessManager* pm = GetProcessManager(web_contents);
    if (pm)
      pm->IncrementLazyKeepaliveCount(extension);
  }
  void OnDialogClosed(content::WebContents* web_contents) override {
    const Extension* extension = GetExtensionForWebContents(web_contents);
    if (extension == nullptr)
      return;

    DCHECK(web_contents);
    extensions::ProcessManager* pm = GetProcessManager(web_contents);
    if (pm)
      pm->DecrementLazyKeepaliveCount(extension);
  }
  bool GetExtensionName(content::WebContents* web_contents,
                        const GURL& origin_url,
                        std::string* name_out) override {
    const Extension* extension = GetExtensionForWebContents(web_contents);
    if (extension &&
        web_contents->GetLastCommittedURL().GetOrigin() == origin_url) {
      *name_out = extension->name();
      return true;
    }
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(JavaScriptDialogExtensionsClientImpl);
};

}  // namespace

void InstallJavaScriptDialogExtensionsClient() {
  SetJavaScriptDialogExtensionsClient(
      make_scoped_ptr(new JavaScriptDialogExtensionsClientImpl));
}
