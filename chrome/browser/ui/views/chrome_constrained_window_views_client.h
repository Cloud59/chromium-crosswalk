// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_CHROME_CONSTRAINED_WINDOW_VIEWS_CLIENT_H_
#define CHROME_BROWSER_UI_VIEWS_CHROME_CONSTRAINED_WINDOW_VIEWS_CLIENT_H_

#include "base/memory/scoped_ptr.h"
#include "components/constrained_window/constrained_window_views_client.h"

// Creates a ConstrainedWindowViewsClient for the Chrome environment.
scoped_ptr<ConstrainedWindowViewsClient>
CreateChromeConstrainedWindowViewsClient();

#endif  // CHROME_BROWSER_UI_VIEWS_CHROME_CONSTRAINED_WINDOW_VIEWS_CLIENT_H_
