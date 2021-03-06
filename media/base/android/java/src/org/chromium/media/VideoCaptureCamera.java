// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.util.Log;
import android.view.Surface;
import android.view.WindowManager;

import org.chromium.base.JNINamespace;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Video Capture Device extension of VideoCapture to provide common functionality
 * for capture using android.hardware.Camera API (deprecated in API 21). Normal
 * Android and Tango devices are extensions of this class.
 **/
@JNINamespace("media")
@SuppressWarnings("deprecation")
public abstract class VideoCaptureCamera extends VideoCapture
        implements android.hardware.Camera.PreviewCallback {

    protected android.hardware.Camera mCamera;
    // Lock to mutually exclude execution of OnPreviewFrame() and {start/stop}Capture().
    protected ReentrantLock mPreviewBufferLock = new ReentrantLock();
    // True when native code has started capture.
    protected boolean mIsRunning = false;

    protected int[] mGlTextures = null;
    protected SurfaceTexture mSurfaceTexture = null;
    protected static final int GL_TEXTURE_EXTERNAL_OES = 0x8D65;

    protected int mCameraOrientation;
    protected int mCameraFacing;
    protected int mDeviceOrientation;
    private static final String TAG = "VideoCaptureCamera";

    protected static android.hardware.Camera.CameraInfo getCameraInfo(int id) {
        android.hardware.Camera.CameraInfo cameraInfo =
                new android.hardware.Camera.CameraInfo();
        try {
            android.hardware.Camera.getCameraInfo(id, cameraInfo);
        } catch (RuntimeException ex) {
            Log.e(TAG, "getCameraInfo: Camera.getCameraInfo: " + ex);
            return null;
        }
        return cameraInfo;
    }

    protected static android.hardware.Camera.Parameters getCameraParameters(
            android.hardware.Camera camera) {
        android.hardware.Camera.Parameters parameters;
        try {
            parameters = camera.getParameters();
        } catch (RuntimeException ex) {
            Log.e(TAG, "getCameraParameters: android.hardware.Camera.getParameters: " + ex);
            camera.release();
            return null;
        }
        return parameters;
    }

    VideoCaptureCamera(Context context,
                       int id,
                       long nativeVideoCaptureDeviceAndroid) {
        super(context, id, nativeVideoCaptureDeviceAndroid);
    }

    @Override
    public boolean allocate(int width, int height, int frameRate) {
        Log.d(TAG, "allocate: requested (" + width + "x" + height + ")@"
                + frameRate + "fps");
        try {
            mCamera = android.hardware.Camera.open(mId);
        } catch (RuntimeException ex) {
            Log.e(TAG, "allocate: Camera.open: " + ex);
            return false;
        }

        android.hardware.Camera.CameraInfo cameraInfo = VideoCaptureCamera.getCameraInfo(mId);
        if (cameraInfo == null) {
            mCamera.release();
            mCamera = null;
            return false;
        }

        mCameraOrientation = cameraInfo.orientation;
        mCameraFacing = cameraInfo.facing;
        mDeviceOrientation = getDeviceOrientation();
        Log.d(TAG, "allocate: orientation dev=" + mDeviceOrientation
                  + ", cam=" + mCameraOrientation + ", facing=" + mCameraFacing);

        android.hardware.Camera.Parameters parameters = getCameraParameters(mCamera);
        if (parameters == null) {
            mCamera = null;
            return false;
        }

        // getSupportedPreviewFpsRange() returns a List with at least one
        // element, but when camera is in bad state, it can return null pointer.
        List<int[]> listFpsRange = parameters.getSupportedPreviewFpsRange();
        if (listFpsRange == null || listFpsRange.size() == 0) {
            Log.e(TAG, "allocate: no fps range found");
            return false;
        }
        // API fps ranges are scaled up x1000 to avoid floating point.
        int frameRateScaled = frameRate * 1000;
        // Use the first range as the default chosen range.
        int[] chosenFpsRange = listFpsRange.get(0);
        int frameRateNearest = Math.abs(frameRateScaled - chosenFpsRange[0])
                               < Math.abs(frameRateScaled - chosenFpsRange[1])
                               ? chosenFpsRange[0] : chosenFpsRange[1];
        int chosenFrameRate = (frameRateNearest + 999) / 1000;
        int fpsRangeSize = Integer.MAX_VALUE;
        for (int[] fpsRange : listFpsRange) {
            if (fpsRange[0] <= frameRateScaled && frameRateScaled <= fpsRange[1]
                    && (fpsRange[1] - fpsRange[0]) <= fpsRangeSize) {
                chosenFpsRange = fpsRange;
                chosenFrameRate = frameRate;
                fpsRangeSize = fpsRange[1] - fpsRange[0];
            }
        }
        Log.d(TAG, "allocate: fps set to " + chosenFrameRate + ", ["
                + chosenFpsRange[0] + "-" + chosenFpsRange[1] + "]");

        // Calculate size.
        List<android.hardware.Camera.Size> listCameraSize =
                parameters.getSupportedPreviewSizes();
        int minDiff = Integer.MAX_VALUE;
        int matchedWidth = width;
        int matchedHeight = height;
        for (android.hardware.Camera.Size size : listCameraSize) {
            int diff = Math.abs(size.width - width)
                       + Math.abs(size.height - height);
            Log.d(TAG, "allocate: supported ("
                    + size.width + ", " + size.height + "), diff=" + diff);
            // TODO(wjia): Remove this hack (forcing width to be multiple
            // of 32) by supporting stride in video frame buffer.
            // Right now, VideoCaptureController requires compact YV12
            // (i.e., with no padding).
            if (diff < minDiff && (size.width % 32 == 0)) {
                minDiff = diff;
                matchedWidth = size.width;
                matchedHeight = size.height;
            }
        }
        if (minDiff == Integer.MAX_VALUE) {
            Log.e(TAG, "allocate: can not find a multiple-of-32 resolution");
            return false;
        }
        Log.d(TAG, "allocate: matched (" + matchedWidth + "x" + matchedHeight + ")");

        if (parameters.isVideoStabilizationSupported()) {
            Log.d(TAG, "Image stabilization supported, currently: "
                    + parameters.getVideoStabilization() + ", setting it.");
            parameters.setVideoStabilization(true);
        } else {
            Log.d(TAG, "Image stabilization not supported.");
        }

        setCaptureParameters(matchedWidth, matchedHeight, chosenFrameRate, parameters);
        parameters.setPictureSize(matchedWidth, matchedHeight);
        parameters.setPreviewSize(matchedWidth, matchedHeight);
        parameters.setPreviewFpsRange(chosenFpsRange[0], chosenFpsRange[1]);
        parameters.setPreviewFormat(mCaptureFormat.mPixelFormat);
        try {
            mCamera.setParameters(parameters);
        } catch (RuntimeException ex) {
            Log.e(TAG, "setParameters: " + ex);
            return false;
        }

        // Set SurfaceTexture. Android Capture needs a SurfaceTexture even if
        // it is not going to be used.
        mGlTextures = new int[1];
        // Generate one texture pointer and bind it as an external texture.
        GLES20.glGenTextures(1, mGlTextures, 0);
        GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, mGlTextures[0]);
        // No mip-mapping with camera source.
        GLES20.glTexParameterf(GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        // Clamp to edge is only option.
        GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

        mSurfaceTexture = new SurfaceTexture(mGlTextures[0]);
        mSurfaceTexture.setOnFrameAvailableListener(null);
        try {
            mCamera.setPreviewTexture(mSurfaceTexture);
        } catch (IOException ex) {
            Log.e(TAG, "allocate: " + ex);
            return false;
        }

        allocateBuffers();
        return true;
    }

    @Override
    public int startCapture() {
        if (mCamera == null) {
            Log.e(TAG, "startCapture: camera is null");
            return -1;
        }

        mPreviewBufferLock.lock();
        try {
            if (mIsRunning) {
                return 0;
            }
            mIsRunning = true;
        } finally {
            mPreviewBufferLock.unlock();
        }
        setPreviewCallback(this);
        try {
            mCamera.startPreview();
        } catch (RuntimeException ex) {
            Log.e(TAG, "startCapture: Camera.startPreview: " + ex);
            return -1;
        }
        return 0;
    }

    @Override
    public int stopCapture() {
        if (mCamera == null) {
            Log.e(TAG, "stopCapture: camera is null");
            return 0;
        }

        mPreviewBufferLock.lock();
        try {
            if (!mIsRunning) {
                return 0;
            }
            mIsRunning = false;
        } finally {
            mPreviewBufferLock.unlock();
        }

        mCamera.stopPreview();
        setPreviewCallback(null);
        return 0;
    }

    @Override
    public void deallocate() {
        if (mCamera == null) return;

        stopCapture();
        try {
            mCamera.setPreviewTexture(null);
            if (mGlTextures != null)
                GLES20.glDeleteTextures(1, mGlTextures, 0);
            mCaptureFormat = null;
            mCamera.release();
            mCamera = null;
        } catch (IOException ex) {
            Log.e(TAG, "deallocate: failed to deallocate camera, " + ex);
            return;
        }
    }

    // Local hook to allow derived classes to fill capture format and modify
    // camera parameters as they see fit.
    abstract void setCaptureParameters(
            int width,
            int height,
            int frameRate,
            android.hardware.Camera.Parameters cameraParameters);

    // Local method to be overriden with the particular setPreviewCallback to be
    // used in the implementations.
    abstract void setPreviewCallback(android.hardware.Camera.PreviewCallback cb);

    protected int getDeviceOrientation() {
        int orientation = 0;
        if (mContext != null) {
            WindowManager wm = (WindowManager) mContext.getSystemService(
                    Context.WINDOW_SERVICE);
            switch(wm.getDefaultDisplay().getRotation()) {
                case Surface.ROTATION_90:
                    orientation = 90;
                    break;
                case Surface.ROTATION_180:
                    orientation = 180;
                    break;
                case Surface.ROTATION_270:
                    orientation = 270;
                    break;
                case Surface.ROTATION_0:
                default:
                    orientation = 0;
                    break;
            }
        }
        return orientation;
    }
}
