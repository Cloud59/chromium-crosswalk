// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/extensions_activity_monitor.h"

#include "content/public/browser/browser_thread.h"
#include "sync/util/extensions_activity.h"

#if defined(ENABLE_EXTENSIONS)
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/bookmarks/bookmarks_api.h"
#include "content/public/browser/notification_service.h"
#include "extensions/common/extension.h"
#endif

using content::BrowserThread;

namespace browser_sync {

ExtensionsActivityMonitor::ExtensionsActivityMonitor()
    : extensions_activity_(new syncer::ExtensionsActivity()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // It would be nice if we could specify a Source for each specific function
  // we wanted to observe, but the actual function objects are allocated on
  // the fly so there is no reliable object to point to (same problem if we
  // wanted to use the string name).  Thus, we use all sources and filter in
  // Observe.
#if defined(ENABLE_EXTENSIONS)
  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSION_BOOKMARKS_API_INVOKED,
                 content::NotificationService::AllSources());
#endif
}

ExtensionsActivityMonitor::~ExtensionsActivityMonitor() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void ExtensionsActivityMonitor::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
#if defined(ENABLE_EXTENSIONS)
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(extensions::NOTIFICATION_EXTENSION_BOOKMARKS_API_INVOKED, type);
  const extensions::Extension* extension =
      content::Source<const extensions::Extension>(source).ptr();
  const extensions::BookmarksFunction* f =
      content::Details<const extensions::BookmarksFunction>(details).ptr();
  if (f->name() == "bookmarks.update" ||
      f->name() == "bookmarks.move" ||
      f->name() == "bookmarks.create" ||
      f->name() == "bookmarks.removeTree" ||
      f->name() == "bookmarks.remove") {
    extensions_activity_->UpdateRecord(extension->id());
  }
#endif
}

const scoped_refptr<syncer::ExtensionsActivity>&
ExtensionsActivityMonitor::GetExtensionsActivity() {
  return extensions_activity_;
}

}  // namespace browser_sync
