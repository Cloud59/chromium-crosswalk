// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/gpu_browser_compositor_output_surface.h"

#include "cc/output/compositor_frame.h"
#include "cc/output/output_surface_client.h"
#include "content/browser/compositor/reflector_impl.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/common/gpu/client/context_provider_command_buffer.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"

namespace content {

GpuBrowserCompositorOutputSurface::GpuBrowserCompositorOutputSurface(
    const scoped_refptr<ContextProviderCommandBuffer>& context,
    int surface_id,
    IDMap<BrowserCompositorOutputSurface>* output_surface_map,
    const scoped_refptr<ui::CompositorVSyncManager>& vsync_manager,
    scoped_ptr<cc::OverlayCandidateValidator> overlay_candidate_validator)
    : BrowserCompositorOutputSurface(context,
                                     surface_id,
                                     output_surface_map,
                                     vsync_manager),
      swap_buffers_completion_callback_(
          base::Bind(&GpuBrowserCompositorOutputSurface::OnSwapBuffersCompleted,
                     base::Unretained(this))) {
  overlay_candidate_validator_ = overlay_candidate_validator.Pass();
}

GpuBrowserCompositorOutputSurface::~GpuBrowserCompositorOutputSurface() {}

CommandBufferProxyImpl*
GpuBrowserCompositorOutputSurface::GetCommandBufferProxy() {
  ContextProviderCommandBuffer* provider_command_buffer =
      static_cast<content::ContextProviderCommandBuffer*>(
          context_provider_.get());
  CommandBufferProxyImpl* command_buffer_proxy =
      provider_command_buffer->GetCommandBufferProxy();
  DCHECK(command_buffer_proxy);
  return command_buffer_proxy;
}

bool GpuBrowserCompositorOutputSurface::BindToClient(
    cc::OutputSurfaceClient* client) {
  if (!BrowserCompositorOutputSurface::BindToClient(client))
    return false;

  GetCommandBufferProxy()->SetSwapBuffersCompletionCallback(
      swap_buffers_completion_callback_.callback());
  return true;
}

void GpuBrowserCompositorOutputSurface::SwapBuffers(
    cc::CompositorFrame* frame) {
  DCHECK(frame->gl_frame_data);

  GetCommandBufferProxy()->SetLatencyInfo(frame->metadata.latency_info);

  if (reflector_.get()) {
    if (frame->gl_frame_data->sub_buffer_rect ==
        gfx::Rect(frame->gl_frame_data->size))
      reflector_->OnSwapBuffers();
    else
      reflector_->OnPostSubBuffer(frame->gl_frame_data->sub_buffer_rect);
  }

  if (frame->gl_frame_data->sub_buffer_rect ==
      gfx::Rect(frame->gl_frame_data->size)) {
    context_provider_->ContextSupport()->Swap();
  } else {
    context_provider_->ContextSupport()->PartialSwapBuffers(
        frame->gl_frame_data->sub_buffer_rect);
  }

  client_->DidSwapBuffers();
}

void GpuBrowserCompositorOutputSurface::OnSwapBuffersCompleted(
    const std::vector<ui::LatencyInfo>& latency_info) {
#if defined(OS_MACOSX)
  // On Mac, delay acknowledging the swap to the output surface client until
  // it has been drawn, see OnSurfaceDisplayed();
  NOTREACHED();
#else
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    RenderWidgetHostImpl::CompositorFrameDrawn(latency_info);
  } else {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&RenderWidgetHostImpl::CompositorFrameDrawn, latency_info));
  }
  OnSwapBuffersComplete();
#endif
}

#if defined(OS_MACOSX)
void GpuBrowserCompositorOutputSurface::OnSurfaceDisplayed() {
  cc::OutputSurface::OnSwapBuffersComplete();
}
#endif

}  // namespace content
