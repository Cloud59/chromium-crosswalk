// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_IMAGE_SOURCE_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_IMAGE_SOURCE_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/url_data_source.h"

namespace base {
class SequencedTaskRunner;
}

// TODO(michaelpg): Remove dependency on user_manager.
namespace user_manager {
class UserImage;
}

namespace chromeos {

// TODO(michaelpg): Generalize UserImageLoader for classes like ImageSource.
class UserImageLoader;

// Data source that reads and decodes an image from the RO file system.
class ImageSource : public content::URLDataSource {
 public:
  ImageSource();
  ~ImageSource() override;

  // content::URLDataSource implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      int render_process_id,
      int render_frame_id,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;

 private:
  void StartOnFileThread(
      const std::string& path,
      const content::URLDataSource::GotDataCallback& callback);

  // Callback for user_manager::UserImageLoader.
  void ImageLoaded(
    const content::URLDataSource::GotDataCallback& callback,
    const user_manager::UserImage& user_image) const;

  // Checks whether we have allowed the image to be loaded.
  bool IsWhitelisted(const std::string& path) const;

  // The background task runner on which file I/O and image decoding are done.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  scoped_refptr<UserImageLoader> image_loader_;

  base::WeakPtrFactory<ImageSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImageSource);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_IMAGE_SOURCE_H_
