// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_SKCANVAS_VIDEO_RENDERER_H_
#define MEDIA_FILTERS_SKCANVAS_VIDEO_RENDERER_H_

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/base/media_export.h"
#include "media/base/video_rotation.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkXfermode.h"
#include "ui/gfx/rect.h"

class SkCanvas;

namespace media {

class VideoFrame;
class VideoImageGenerator;

// Handles rendering of VideoFrames to SkCanvases, doing any necessary YUV
// conversion and caching of resulting RGB bitmaps.
class MEDIA_EXPORT SkCanvasVideoRenderer {
 public:
  SkCanvasVideoRenderer();
  ~SkCanvasVideoRenderer();

  // Paints |video_frame| on |canvas|, scaling and rotating the result to fit
  // dimensions specified by |dest_rect|.
  //
  // Black will be painted on |canvas| if |video_frame| is null.
  void Paint(const scoped_refptr<VideoFrame>& video_frame,
             SkCanvas* canvas,
             const gfx::RectF& dest_rect,
             uint8 alpha,
             SkXfermode::Mode mode,
             VideoRotation video_rotation);

  // Copy |video_frame| on |canvas|.
  void Copy(const scoped_refptr<VideoFrame>&, SkCanvas* canvas);

 private:
  void ResetLastFrame();
  void ResetAcceleratedLastFrame();

  // An RGB bitmap and corresponding timestamp of the previously converted
  // video frame data by software color space conversion.
  SkBitmap last_frame_;
  base::TimeDelta last_frame_timestamp_;
  // If |last_frame_| is not used for a while, it's deleted to save memory.
  base::DelayTimer<SkCanvasVideoRenderer> frame_deleting_timer_;

  // This is a hardware accelerated copy of the frame generated by
  // |accelerated_generator_|.
  // It's used when |canvas| parameter in Paint() is Ganesh canvas.
  // Note: all GrContext in SkCanvas instances are same.
  SkBitmap accelerated_last_frame_;
  VideoImageGenerator* accelerated_generator_;
  base::TimeDelta accelerated_last_frame_timestamp_;
  base::DelayTimer<SkCanvasVideoRenderer> accelerated_frame_deleting_timer_;

  DISALLOW_COPY_AND_ASSIGN(SkCanvasVideoRenderer);
};

}  // namespace media

#endif  // MEDIA_FILTERS_SKCANVAS_VIDEO_RENDERER_H_
