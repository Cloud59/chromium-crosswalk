// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/apps/chrome_apps_client.h"

#include "chrome/browser/ui/views/apps/chrome_native_app_window_views.h"

// static
extensions::NativeAppWindow* ChromeAppsClient::CreateNativeAppWindowImpl(
    extensions::AppWindow* app_window,
    const extensions::AppWindow::CreateParams& params) {
  ChromeNativeAppWindowViews* window = new ChromeNativeAppWindowViews;
  window->Init(app_window, params);
  return window;
}
