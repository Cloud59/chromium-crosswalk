// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/image_transport_surface.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "content/common/gpu/gpu_channel.h"
#include "content/common/gpu/gpu_channel_manager.h"
#include "content/common/gpu/gpu_command_buffer_stub.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/gpu/null_transport_surface.h"
#include "content/common/gpu/sync_point_manager.h"
#include "gpu/command_buffer/service/gpu_scheduler.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_switches.h"

namespace content {

ImageTransportSurface::ImageTransportSurface() {}

ImageTransportSurface::~ImageTransportSurface() {}

scoped_refptr<gfx::GLSurface> ImageTransportSurface::CreateSurface(
    GpuChannelManager* manager,
    GpuCommandBufferStub* stub,
    const gfx::GLSurfaceHandle& handle) {
  scoped_refptr<gfx::GLSurface> surface;
  if (handle.transport_type == gfx::NULL_TRANSPORT) {
#if defined(OS_ANDROID)
    surface = CreateTransportSurface(manager, stub, handle);
#else
    surface = new NullTransportSurface(manager, stub, handle);
#endif
  } else {
    surface = CreateNativeSurface(manager, stub, handle);
  }

  if (!surface.get() || !surface->Initialize())
    return NULL;
  return surface;
}

ImageTransportHelper::ImageTransportHelper(ImageTransportSurface* surface,
                                           GpuChannelManager* manager,
                                           GpuCommandBufferStub* stub,
                                           gfx::PluginWindowHandle handle)
    : surface_(surface),
      manager_(manager),
      stub_(stub->AsWeakPtr()),
      handle_(handle) {
  route_id_ = manager_->GenerateRouteID();
  manager_->AddRoute(route_id_, this);
}

ImageTransportHelper::~ImageTransportHelper() {
  if (stub_.get()) {
    stub_->SetLatencyInfoCallback(
        base::Callback<void(const std::vector<ui::LatencyInfo>&)>());
  }
  manager_->RemoveRoute(route_id_);
}

bool ImageTransportHelper::Initialize() {
  gpu::gles2::GLES2Decoder* decoder = Decoder();

  if (!decoder)
    return false;

  decoder->SetResizeCallback(
       base::Bind(&ImageTransportHelper::Resize, base::Unretained(this)));

  stub_->SetLatencyInfoCallback(
      base::Bind(&ImageTransportHelper::SetLatencyInfo,
                 base::Unretained(this)));

  manager_->Send(new GpuHostMsg_AcceleratedSurfaceInitialized(
      stub_->surface_id(), route_id_));

  return true;
}

bool ImageTransportHelper::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ImageTransportHelper, message)
#if defined(OS_MACOSX)
    IPC_MESSAGE_HANDLER(AcceleratedSurfaceMsg_BufferPresented,
                        OnBufferPresented)
#endif
    IPC_MESSAGE_HANDLER(AcceleratedSurfaceMsg_WakeUpGpu, OnWakeUpGpu);
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ImageTransportHelper::SendAcceleratedSurfaceBuffersSwapped(
    GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params params) {
  // TRACE_EVENT for gpu tests:
  TRACE_EVENT_INSTANT2("test_gpu", "SwapBuffers",
                       TRACE_EVENT_SCOPE_THREAD,
                       "GLImpl", static_cast<int>(gfx::GetGLImplementation()),
                       "width", params.size.width());
  params.surface_id = stub_->surface_id();
  params.route_id = route_id_;
  manager_->Send(new GpuHostMsg_AcceleratedSurfaceBuffersSwapped(params));
}

void ImageTransportHelper::SendUpdateVSyncParameters(
      base::TimeTicks timebase, base::TimeDelta interval) {
  manager_->Send(new GpuHostMsg_UpdateVSyncParameters(stub_->surface_id(),
                                                      timebase,
                                                      interval));
}

void ImageTransportHelper::SwapBuffersCompleted(
    const std::vector<ui::LatencyInfo>& latency_info) {
  stub_->SwapBuffersCompleted(latency_info);
}

void ImageTransportHelper::SetScheduled(bool is_scheduled) {
  gpu::GpuScheduler* scheduler = Scheduler();
  if (!scheduler)
    return;

  scheduler->SetScheduled(is_scheduled);
}

void ImageTransportHelper::DeferToFence(base::Closure task) {
  gpu::GpuScheduler* scheduler = Scheduler();
  DCHECK(scheduler);

  scheduler->DeferToFence(task);
}

void ImageTransportHelper::SetPreemptByFlag(
    scoped_refptr<gpu::PreemptionFlag> preemption_flag) {
  stub_->channel()->SetPreemptByFlag(preemption_flag);
}

bool ImageTransportHelper::MakeCurrent() {
  gpu::gles2::GLES2Decoder* decoder = Decoder();
  if (!decoder)
    return false;
  return decoder->MakeCurrent();
}

void ImageTransportHelper::SetSwapInterval(gfx::GLContext* context) {
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kDisableGpuVsync))
    context->SetSwapInterval(0);
  else
    context->SetSwapInterval(1);
}

gpu::GpuScheduler* ImageTransportHelper::Scheduler() {
  if (!stub_.get())
    return NULL;
  return stub_->scheduler();
}

gpu::gles2::GLES2Decoder* ImageTransportHelper::Decoder() {
  if (!stub_.get())
    return NULL;
  return stub_->decoder();
}

#if defined(OS_MACOSX)
void ImageTransportHelper::OnBufferPresented(
    const AcceleratedSurfaceMsg_BufferPresented_Params& params) {
  surface_->OnBufferPresented(params);
}
#endif

void ImageTransportHelper::OnWakeUpGpu() {
  surface_->WakeUpGpu();
}

void ImageTransportHelper::Resize(gfx::Size size, float scale_factor) {
  surface_->OnResize(size, scale_factor);

#if defined(OS_ANDROID)
  manager_->gpu_memory_manager()->ScheduleManage(
      GpuMemoryManager::kScheduleManageNow);
#endif
}

void ImageTransportHelper::SetLatencyInfo(
    const std::vector<ui::LatencyInfo>& latency_info) {
  surface_->SetLatencyInfo(latency_info);
}

PassThroughImageTransportSurface::PassThroughImageTransportSurface(
    GpuChannelManager* manager,
    GpuCommandBufferStub* stub,
    gfx::GLSurface* surface)
    : GLSurfaceAdapter(surface),
      did_set_swap_interval_(false) {
  helper_.reset(new ImageTransportHelper(this,
                                         manager,
                                         stub,
                                         gfx::kNullPluginWindow));
}

bool PassThroughImageTransportSurface::Initialize() {
  // The surface is assumed to have already been initialized.
  return helper_->Initialize();
}

void PassThroughImageTransportSurface::Destroy() {
  GLSurfaceAdapter::Destroy();
}

void PassThroughImageTransportSurface::SetLatencyInfo(
    const std::vector<ui::LatencyInfo>& latency_info) {
  for (size_t i = 0; i < latency_info.size(); i++)
    latency_info_.push_back(latency_info[i]);
}

bool PassThroughImageTransportSurface::SwapBuffers() {
  // GetVsyncValues before SwapBuffers to work around Mali driver bug:
  // crbug.com/223558.
  SendVSyncUpdateIfAvailable();
  for (size_t i = 0; i < latency_info_.size(); ++i) {
    latency_info_[i].AddLatencyNumber(
        ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT, 0, 0);
  }
  bool result = gfx::GLSurfaceAdapter::SwapBuffers();
  for (size_t i = 0; i < latency_info_.size(); i++) {
    latency_info_[i].AddLatencyNumber(
        ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0, 0);
  }

  helper_->SwapBuffersCompleted(latency_info_);
  latency_info_.clear();
  return result;
}

bool PassThroughImageTransportSurface::PostSubBuffer(
    int x, int y, int width, int height) {
  SendVSyncUpdateIfAvailable();
  bool result = gfx::GLSurfaceAdapter::PostSubBuffer(x, y, width, height);
  for (size_t i = 0; i < latency_info_.size(); i++) {
    latency_info_[i].AddLatencyNumber(
        ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0, 0);
  }

  helper_->SwapBuffersCompleted(latency_info_);
  latency_info_.clear();
  return result;
}

bool PassThroughImageTransportSurface::OnMakeCurrent(gfx::GLContext* context) {
  if (!did_set_swap_interval_) {
    ImageTransportHelper::SetSwapInterval(context);
    did_set_swap_interval_ = true;
  }
  return true;
}

#if defined(OS_MACOSX)
void PassThroughImageTransportSurface::OnBufferPresented(
    const AcceleratedSurfaceMsg_BufferPresented_Params& /* params */) {
  NOTREACHED();
}
#endif

void PassThroughImageTransportSurface::OnResize(gfx::Size size,
                                                float scale_factor) {
  Resize(size);
}

gfx::Size PassThroughImageTransportSurface::GetSize() {
  return GLSurfaceAdapter::GetSize();
}

void PassThroughImageTransportSurface::WakeUpGpu() {
  NOTREACHED();
}

PassThroughImageTransportSurface::~PassThroughImageTransportSurface() {}

void PassThroughImageTransportSurface::SendVSyncUpdateIfAvailable() {
  gfx::VSyncProvider* vsync_provider = GetVSyncProvider();
  if (vsync_provider) {
    vsync_provider->GetVSyncParameters(
      base::Bind(&ImageTransportHelper::SendUpdateVSyncParameters,
                 helper_->AsWeakPtr()));
  }
}

}  // namespace content
