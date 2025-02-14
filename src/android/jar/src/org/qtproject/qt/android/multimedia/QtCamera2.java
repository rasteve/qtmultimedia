// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Rect;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Range;
import android.view.Surface;
import java.lang.Thread;
import java.util.ArrayList;
import java.util.List;

import org.qtproject.qt.android.UsedFromNativeCode;

@TargetApi(23)
class QtCamera2 {

    CameraDevice mCameraDevice = null;
    QtVideoDeviceManager mVideoDeviceManager = null;
    HandlerThread mBackgroundThread;
    Handler mBackgroundHandler;
    ImageReader mImageReader = null;
    ImageReader mCapturedPhotoReader = null;
    CameraManager mCameraManager;
    CameraCaptureSession mCaptureSession;
    CaptureRequest.Builder mPreviewRequestBuilder;
    CaptureRequest mPreviewRequest;
    String mCameraId;
    List<Surface> mTargetSurfaces = new ArrayList<>();

    // The following constants are used during the capturePhoto routine.
    // It should happen in the following order:
    // 1. Acquire focus
    // 2. Calibrate auto-exposure for pre-capture
    // 3. Calibrate auto-exposure for capture
    // 4. Capture the photo
    //
    // Still photo captures should be finalized using a single CameraCaptureSession.capture() call,
    // but precapture calibration steps (auto-focus, auto-exposure) should be done using the
    // continuously repeating preview request.
    private static final int STATE_PREVIEW = 0;
    // We are waiting for focus lock
    private static final int STATE_WAITING_FOCUS_LOCK = 1;
    // We are waiting for exposure calibration
    private static final int STATE_WAITING_EXPOSURE_PRECAPTURE = 2;
    private static final int STATE_WAITING_EXPOSURE_NON_PRECAPTURE = 3;
    // The picture is ready to be read into an image object.
    private static final int STATE_PICTURE_TAKEN = 4;

    // An mState that is not set to STATE_PREVIEW implies we are currently trying to capture a still
    // photo.
    private int mState = STATE_PREVIEW;
    private static int MaxNumberFrames = 12;

    private static final int DEFAULT_FLASH_MODE = CaptureRequest.CONTROL_AE_MODE_ON;
    private static final int DEFAULT_TORCH_MODE = CameraMetadata.FLASH_MODE_OFF;
    // Default value in QPlatformCamera is FocusModeAuto, which maps to CONTINUOUS_PICTURE.
    private static final int DEFAULT_AF_MODE = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE;
    private static final float DEFAULT_FOCUS_DISTANCE = 1.f;
    private static final float DEFAULT_ZOOM_FACTOR = 1.0f;

    // The purpose of this class is to gather variables that are accessed across
    // the C++ QCamera's thread, and the background capture-processing thread.
    // It also acts as the mutex for these variables.
    // All access to these variables must happen after locking the instance.
    class SyncedMembers {
        private boolean mIsStarted = false;

        // Not to be confused with QCamera::FlashMode.
        // This controls the currently desired CaptureRequest.CONTROL_AE_MODE.
        // QCamera::FlashMode::FlashOff maps to CaptureRequest.CONTROL_AE_MODE_ON. This implies regular
        // automatic exposure.
        // QCamera::FlashMode::FlashAuto maps to CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH.
        // QCamera::FlashMode::FlashOn maps to CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH.
        private int mFlashMode = DEFAULT_FLASH_MODE;

        // Not to be confused with QCamera::TorchMode.
        // This controls the currently desired CaptureRequest.FLASH_MODE
        // QCamera::TorchMode::TorchOff maps to CaptureRequest.FLASH_MODE_OFF
        // QCamera::TorchMode::TorchAuto is not supported.
        // QCamera::TorhcMode::TorchOn maps to CaptureRequest.FLASH_MODE_TORCH.
        private int mTorchMode = DEFAULT_TORCH_MODE;

        // Not to be confused with QCamera::FocusMode
        // This controls the currently desired CaptureRequest.CONTROL_AF_MODE
        // QCamera::FocusMode::FocusModeAuto maps to CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
        //
        // This variable only controls the AF_MODE we desire to apply. If the device
        // does not support this AF_MODE this will not reflect what the camera is currently doing.
        private int mAFMode = DEFAULT_AF_MODE;

        // Not to be confused with CaptureRequest.LENS_FOCUS_DISTANCE. This variable stores the
        // current QCamera::focusDistance. Must be applied whenever the focus-mode is set to
        // Manual.
        // Must have the same default value as the one set in QPlatformCamera.
        private float mFocusDistance = DEFAULT_FOCUS_DISTANCE;

        // Not to be confused with CaptureRequest.CONTROL_ZOOM_RATIO
        // This matches the current QCamera::zoomFactor of the C++ QCamera object.
        private float mZoomFactor = DEFAULT_ZOOM_FACTOR;
    }
    private final SyncedMembers mSyncedMembers = new SyncedMembers();

    // Resets the control properties of this camera to their default values.
    @UsedFromNativeCode
    public void resetControlProperties() {
        synchronized (mSyncedMembers) {
            mSyncedMembers.mFlashMode = DEFAULT_FLASH_MODE;
            mSyncedMembers.mTorchMode = DEFAULT_TORCH_MODE;
            mSyncedMembers.mAFMode = DEFAULT_AF_MODE;
            mSyncedMembers.mFocusDistance = DEFAULT_FOCUS_DISTANCE;
            mSyncedMembers.mZoomFactor = DEFAULT_ZOOM_FACTOR;
        }
    }

    private Range<Integer> mFpsRange = null;
    private QtExifDataHandler mExifDataHandler = null;

    native void onCameraOpened(String cameraId);
    native void onCameraDisconnect(String cameraId);
    native void onCameraError(String cameraId, int error);
    CameraDevice.StateCallback mStateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice cameraDevice) {
            if (mCameraDevice != null)
                mCameraDevice.close();
            mCameraDevice = cameraDevice;
            onCameraOpened(mCameraId);
        }
        @Override
        public void onDisconnected(CameraDevice cameraDevice) {
            cameraDevice.close();
            if (mCameraDevice == cameraDevice)
                mCameraDevice = null;
            onCameraDisconnect(mCameraId);
        }
        @Override
        public void onError(CameraDevice cameraDevice, int error) {
            cameraDevice.close();
            if (mCameraDevice == cameraDevice)
                mCameraDevice = null;
            onCameraError(mCameraId, error);
        }
    };

    native void onCaptureSessionConfigured(String cameraId);
    native void onCaptureSessionConfigureFailed(String cameraId);
    CameraCaptureSession.StateCallback mCaptureStateCallback = new CameraCaptureSession.StateCallback() {
        @Override
        public void onConfigured(CameraCaptureSession cameraCaptureSession) {
            mCaptureSession = cameraCaptureSession;
            onCaptureSessionConfigured(mCameraId);
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
            onCaptureSessionConfigureFailed(mCameraId);
        }

        @Override
        public void onActive(CameraCaptureSession cameraCaptureSession) {
           super.onActive(cameraCaptureSession);
           onSessionActive(mCameraId);
        }

        @Override
        public void onClosed(CameraCaptureSession cameraCaptureSession) {
            super.onClosed(cameraCaptureSession);
            onSessionClosed(mCameraId);
        }
    };

    native void onSessionActive(String cameraId);
    native void onSessionClosed(String cameraId);
    native void onCaptureSessionFailed(String cameraId, int reason, long frameNumber);
    CameraCaptureSession.CaptureCallback mCaptureCallback = new CameraCaptureSession.CaptureCallback() {
        @Override
        public void onCaptureFailed(CameraCaptureSession session,  CaptureRequest request,  CaptureFailure failure) {
            super.onCaptureFailed(session, request, failure);
            onCaptureSessionFailed(mCameraId, failure.getReason(), failure.getFrameNumber());
        }

        private void handleCaptureFocusLock(CaptureResult result) {
            final Integer afStateObj = result.get(CaptureResult.CONTROL_AF_STATE);
            if (afStateObj == null) {
                capturePhoto();
                return;
            }
            final int afState = afStateObj;
            // The focus can get locked either with or without focus, depending on whether
            // the camera-device was able to find the focus target. Either way,
            // we want to continue to the next step once it stops scanning for focus target.
            final boolean focusLocked =
                afState == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED
                || afState == CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
            if (focusLocked) {
                // If exposure is already converged, or unavailable entirely, we go
                // straight to capturing the photo.
                Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                if (aeState == null ||  aeState == CaptureResult.CONTROL_AE_STATE_CONVERGED) {
                    mState = STATE_PICTURE_TAKEN;
                    capturePhoto();
                } else {
                    // Focusing phase is finished, transition to exposure calibration for
                    // pre-capture.
                    try {
                        mPreviewRequestBuilder.set(
                            CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
                            CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_START);
                        mState = STATE_WAITING_EXPOSURE_PRECAPTURE;
                        mCaptureSession.capture(mPreviewRequestBuilder.build(),
                            mCaptureCallback,
                            mBackgroundHandler);
                    } catch (CameraAccessException e) {
                        Log.w("QtCamera2", "Cannot get access to the camera: " + e);
                    }
                }
            }
        }

        private void handleCaptureExposurePrecapture(CaptureResult result) {
            Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
            if (aeState == null || aeState == CaptureResult.CONTROL_AE_STATE_PRECAPTURE) {
                mState = STATE_WAITING_EXPOSURE_NON_PRECAPTURE;
            }
        }

        private void handleCaptureExposureNonPrecapture(CaptureResult result) {
            Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
            if (aeState == null || aeState != CaptureResult.CONTROL_AE_STATE_PRECAPTURE) {
                mState = STATE_PICTURE_TAKEN;
                capturePhoto();
            }
        }

        // Dispatches to state handlers based on the current state in the photo-capture routine.
        private void process(CaptureResult result) {
            switch (mState) {
                case STATE_WAITING_FOCUS_LOCK:
                    handleCaptureFocusLock(result);
                    break;
                case STATE_WAITING_EXPOSURE_PRECAPTURE:
                    handleCaptureExposurePrecapture(result);
                    break;
                case STATE_WAITING_EXPOSURE_NON_PRECAPTURE:
                    handleCaptureExposureNonPrecapture(result);
                    break;
                default:
                    break;
            }
        }

        @Override
        public void onCaptureProgressed(CameraCaptureSession s, CaptureRequest r, CaptureResult partialResult) {
            process(partialResult);
        }

        @Override
        public void onCaptureCompleted(CameraCaptureSession s, CaptureRequest r, TotalCaptureResult result) {
            process(result);
        }
    };

    QtCamera2(Context context) {
        mCameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        mVideoDeviceManager = new QtVideoDeviceManager(context);
        startBackgroundThread();
    }

    void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("CameraBackground");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

    @UsedFromNativeCode
    void stopBackgroundThread() {
        mBackgroundThread.quitSafely();
        try {
            mBackgroundThread.join();
            mBackgroundThread = null;
            mBackgroundHandler = null;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @SuppressLint("MissingPermission")
    boolean open(String cameraId) {
        try {
            mCameraId = cameraId;
            mCameraManager.openCamera(cameraId,mStateCallback,mBackgroundHandler);
            return true;
        } catch (Exception e){
            Log.w("QtCamera2", "Failed to open camera:" + e);
        }

        return false;
    }

    native void onPhotoAvailable(String cameraId, Image frame);

    ImageReader.OnImageAvailableListener mOnPhotoAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            QtCamera2.this.onPhotoAvailable(mCameraId, reader.acquireLatestImage());
        }
    };

    native void onFrameAvailable(String cameraId, Image frame);

    ImageReader.OnImageAvailableListener mOnImageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            try {
                Image img = reader.acquireLatestImage();
                if (img != null)
                    QtCamera2.this.onFrameAvailable(mCameraId, img);
            } catch (IllegalStateException e) {
                // It seems that ffmpeg is processing images for too long (and does not close it)
                // Give it a little more time. Restarting the camera session if it doesn't help
                Log.e("QtCamera2", "Image processing taking too long. Let's wait 0,5s more " + e);
                try {
                    Thread.sleep(500);
                    QtCamera2.this.onFrameAvailable(mCameraId, reader.acquireLatestImage());
                } catch (IllegalStateException | InterruptedException e2) {
                    Log.e("QtCamera2", "Will not wait anymore. Restart camera session. " + e2);
                    // Remember current used camera ID, because stopAndClose will clear the value
                    String cameraId = mCameraId;
                    stopAndClose();
                    addImageReader(mImageReader.getWidth(), mImageReader.getHeight(),
                                   mImageReader.getImageFormat());
                    open(cameraId);
                }
            }
        }
    };

    @UsedFromNativeCode
    void prepareCamera(int width, int height, int format, int minFps, int maxFps) {

        addImageReader(width, height, format);
        setFrameRate(minFps, maxFps);
    }

    private void addImageReader(int width, int height, int format) {

        if (mImageReader != null)
            removeSurface(mImageReader.getSurface());

        if (mCapturedPhotoReader != null)
            removeSurface(mCapturedPhotoReader.getSurface());

        mImageReader = ImageReader.newInstance(width, height, format, MaxNumberFrames);
        mImageReader.setOnImageAvailableListener(mOnImageAvailableListener, mBackgroundHandler);
        addSurface(mImageReader.getSurface());

        mCapturedPhotoReader = ImageReader.newInstance(width, height, format, MaxNumberFrames);
        mCapturedPhotoReader.setOnImageAvailableListener(mOnPhotoAvailableListener, mBackgroundHandler);
        addSurface(mCapturedPhotoReader.getSurface());
    }

    private void setFrameRate(int minFrameRate, int maxFrameRate) {

        if (minFrameRate <= 0 || maxFrameRate <= 0)
            mFpsRange = null;
        else
            mFpsRange = new Range<>(minFrameRate, maxFrameRate);
    }

    boolean addSurface(Surface surface) {
        if (mTargetSurfaces.contains(surface))
            return true;

        return mTargetSurfaces.add(surface);
    }

    boolean removeSurface(Surface surface) {
        return  mTargetSurfaces.remove(surface);
    }

    @UsedFromNativeCode
    void clearSurfaces() {
        mTargetSurfaces.clear();
    }

    @UsedFromNativeCode
    boolean createSession() {
        if (mCameraDevice == null)
            return false;

        try {
            mCameraDevice.createCaptureSession(mTargetSurfaces, mCaptureStateCallback, mBackgroundHandler);
            return true;
        } catch (Exception exception) {
            Log.w("QtCamera2", "Failed to create a capture session:" + exception);
        }
        return false;
    }

    @UsedFromNativeCode
    boolean start(int template) {

        if (mCameraDevice == null)
            return false;

        if (mCaptureSession == null)
            return false;

        synchronized (mSyncedMembers) {
            try {
                mPreviewRequestBuilder = mCameraDevice.createCaptureRequest(template);
                mPreviewRequestBuilder.addTarget(mImageReader.getSurface());

                applyFocusSettingsToCaptureRequestBuilder(mPreviewRequestBuilder);

                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, mSyncedMembers.mFlashMode);
                mPreviewRequestBuilder.set(CaptureRequest.FLASH_MODE, mSyncedMembers.mTorchMode);
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_IDLE);
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_CAPTURE_INTENT, CameraMetadata.CONTROL_CAPTURE_INTENT_VIDEO_RECORD);
                if (mSyncedMembers.mZoomFactor != 1.0f)
                    updateZoom(mPreviewRequestBuilder, mSyncedMembers.mZoomFactor);
                if (mFpsRange != null)
                    mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, mFpsRange);
                mPreviewRequest = mPreviewRequestBuilder.build();
                mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
                mSyncedMembers.mIsStarted = true;
                return true;

            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to start preview:" + exception);
            }
            return false;
        }
    }

    @UsedFromNativeCode
    void stopAndClose() {
        synchronized (mSyncedMembers) {
            try {
                if (null != mCaptureSession) {
                    mCaptureSession.close();
                    mCaptureSession = null;
                }
                if (null != mCameraDevice) {
                    mCameraDevice.close();
                    mCameraDevice = null;
                }
                mCameraId = "";
                mTargetSurfaces.clear();
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to stop and close:" + exception);
            }
            mSyncedMembers.mIsStarted = false;
        }
    }

    // Used for finalizing a still photo capture. Will reset mState and preview-request back to
    // default when capture is done. This should be used for a singular capture-call, not a
    // repeating request.
    class StillPhotoCaptureSessionCallback extends CameraCaptureSession.CaptureCallback {
        @Override
        public void onCaptureCompleted(
            CameraCaptureSession session,
            CaptureRequest request,
            TotalCaptureResult result)
        {
            try {
                mExifDataHandler = new QtExifDataHandler(result);
                // Reset the focus/flash and go back to the normal state of preview.
                mPreviewRequestBuilder.set(
                    CaptureRequest.CONTROL_AF_TRIGGER,
                    CameraMetadata.CONTROL_AF_TRIGGER_IDLE);
                mPreviewRequestBuilder.set(
                    CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
                    CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_IDLE);
                mPreviewRequest = mPreviewRequestBuilder.build();
                mState = STATE_PREVIEW;
                mCaptureSession.setRepeatingRequest(
                    mPreviewRequest,
                    mCaptureCallback,
                    mBackgroundHandler);
            } catch (CameraAccessException e) {
                e.printStackTrace();
            } catch (NullPointerException e) {
                // See QTBUG-130901:
                // It should not be possible for mCaptureSession to be null here
                // because we always call .close() on mCaptureSession and then set it to null.
                // Calling .close() should flush all pending callbacks, including this one.
                // Either way, user has evidence this is happening, and catching this exception
                // stops us from crashing the program.
                Log.e(
                    "QtCamera2",
                    "Null-pointer access exception thrown when finalizing still photo capture. " +
                    "This should not be possible.");
                e.printStackTrace();
            }
        }
    }

    // Can be called from C++ thread through 'takePhoto()' or directly by CameraCaptureCallback
    // on background thread in order to finalize a still photo capture.
    private void capturePhoto() {
        int aeMode = 0;
        float zoomFactor = 0.f;
        synchronized (mSyncedMembers) {
            aeMode = mSyncedMembers.mFlashMode;
            zoomFactor = mSyncedMembers.mZoomFactor;
        }

        try {
            final CaptureRequest.Builder captureBuilder =
                   mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureBuilder.addTarget(mCapturedPhotoReader.getSurface());

            applyFocusSettingsToCaptureRequestBuilder(captureBuilder);

            captureBuilder.set(CaptureRequest.CONTROL_AE_MODE, aeMode);
            if (zoomFactor != 1.0f)
                updateZoom(captureBuilder, zoomFactor);

            final StillPhotoCaptureSessionCallback captureCallback =
                new StillPhotoCaptureSessionCallback();

            mCaptureSession.capture(captureBuilder.build(), captureCallback, mBackgroundHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    // If auto-focus is enabled, will initiate the still photo precapture routine by adjusting
    // focusing and exposure. Otherwise, will finalize a still photo immediately.
    @UsedFromNativeCode
    void takePhoto() {
        // Load copies of synced members before applying.
        int afMode = 0;
        synchronized (mSyncedMembers) {
            afMode = mSyncedMembers.mAFMode;
        }

        try {
            // If we are doing continuous auto-focusing, we trigger the still-photo capture routine.
            // This will make the focus make an additional attempt to lock focus on the subject
            // before capturing the photo.
            if (afMode == CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
                && isAfModeSupported(afMode))
            {
                applyFocusSettingsToCaptureRequestBuilder(mPreviewRequestBuilder);
                mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START);
                mState = STATE_WAITING_FOCUS_LOCK;
                mCaptureSession.capture(mPreviewRequestBuilder.build(), mCaptureCallback, mBackgroundHandler);
            } else {
                capturePhoto();
            }
        } catch (CameraAccessException e) {
            Log.w("QtCamera2", "Cannot get access to the camera: " + e);
        }
    }

    @UsedFromNativeCode
    void saveExifToFile(String path)
    {
        if (mExifDataHandler != null)
            mExifDataHandler.save(path);
        else
            Log.e("QtCamera2", "No Exif data that could be saved to " + path);
    }

    private Rect getScalerCropRegion(float zoomFactor)
    {
        Rect activePixels = mVideoDeviceManager.getActiveArraySize(mCameraId);
        float zoomRatio = 1.0f;
        if (zoomFactor != 0.0f)
            zoomRatio = 1.0f / zoomFactor;

        int croppedWidth = activePixels.width() - (int)(activePixels.width() * zoomRatio);
        int croppedHeight = activePixels.height() - (int)(activePixels.height() * zoomRatio);
        return new Rect(croppedWidth/2, croppedHeight/2, activePixels.width() - croppedWidth/2,
                             activePixels.height() - croppedHeight/2);
    }

    private void updateZoom(CaptureRequest.Builder requBuilder, float zoomFactor)
    {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.R) {
            requBuilder.set(CaptureRequest.SCALER_CROP_REGION, getScalerCropRegion(zoomFactor));
        } else {
            requBuilder.set(CaptureRequest.CONTROL_ZOOM_RATIO, zoomFactor);
        }
    }

    @UsedFromNativeCode
    void zoomTo(float factor)
    {
        synchronized (mSyncedMembers) {
            mSyncedMembers.mZoomFactor = factor;

            if (!mSyncedMembers.mIsStarted) {
                // Camera capture has not begun. Zoom will be applied during start().
                return;
            }

            updateZoom(mPreviewRequestBuilder, factor);
            mPreviewRequest = mPreviewRequestBuilder.build();

            try {
                mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to set zoom:" + exception);
            }
        }
    }

    // As described in QPlatformCamera::setFocusMode, this function must apply the focus-distance
    // whenever the new QCamera::focusMode is set to Manual.
    // For now, the QtCamera2 implementation only supports Auto and Manual FocusModes.
    @UsedFromNativeCode
    void setFocusMode(int newFocusMode)
    {
        // TODO: In the future, not all QCamera::FocusModes will have a 1:1 mapping to the
        // CONTROL_AF_MODE values. We will need a general solution to translate between
        // QCamera::FocusModes and the relevant Android Camera2 properties.

        // Expand with more values in the future.
        // Translate into the corresponding CONTROL_AF_MODE.
        int newAfMode = 0;
        if (newFocusMode == 0) // FocusModeAuto
            newAfMode = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        else if (newFocusMode == 5) // FocusModeManual
            newAfMode = CaptureRequest.CONTROL_AF_MODE_OFF;
        else {
            Log.d(
                "QtCamera2",
                "received a QCamera::FocusMode from native code that is not recognized. " +
                "Likely Qt developer bug. Ignoring.");
            return;
        }

        // TODO: Ideally this should check if newAfMode is supported through isAfModeSupported()
        // but in some situation, mCameraId will be null and therefore isAfModeSupported will always
        // return false. One example of this is during QCamera::setCameraDevice.
        /*
        if (!isAfModeSupported(newAfMode)) {
            Log.d(
                "QtCamera2",
                "received a QCamera::FocusMode from native code that is not reported as supported. " +
                "Likely Qt developer bug. Ignoring.");
            return;
        }
         */

        synchronized (mSyncedMembers) {
            mSyncedMembers.mAFMode = newAfMode;

            // If the camera is not in the started state yet, we skip activating focus-mode here.
            // Instead it will get applied when the camera is initialized.
            if (!mSyncedMembers.mIsStarted)
                return;

            applyFocusSettingsToCaptureRequestBuilder(mPreviewRequestBuilder);
            mPreviewRequest = mPreviewRequestBuilder.build();

            try {
                mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to set focus mode:" + exception);
            }
        }
    }

    @UsedFromNativeCode
    void setFlashMode(String flashMode)
    {
        synchronized (mSyncedMembers) {

            int flashModeValue = mVideoDeviceManager.stringToControlAEMode(flashMode);
            if (flashModeValue < 0) {
                Log.w("QtCamera2", "Unknown flash mode");
                return;
            }
            mSyncedMembers.mFlashMode = flashModeValue;

            if (!mSyncedMembers.mIsStarted)
                return;

            mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, mSyncedMembers.mFlashMode);
            mPreviewRequest = mPreviewRequestBuilder.build();

            try {
                mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
            } catch (Exception exception) {
                Log.w("QtCamera2", "Failed to set flash mode:" + exception);
            }
        }
    }

    // Sets the focus distance of the camera. Input is the same that accepted by the
    // QCamera public API. Accepts a float in the range 0,1. Where 0 means as close as possible,
    // and 1 means infinity.
    //
    // This should never be called if the device specifies focus-distance as unsupported.
    @UsedFromNativeCode
    public void setFocusDistance(float distanceInput)
    {
        if (distanceInput < 0.f || distanceInput > 1.f) {
            Log.w(
                "QtCamera2",
                "received out-of-bounds value when setting camera focus-distance. " +
                "Likely Qt developer bug. Ignoring.");
            return;
        }

        // TODO: Add error handling to check if current mCameraId supports setting focus-distance.
        // See setFocusMode relevant issue.

        synchronized (mSyncedMembers) {
            mSyncedMembers.mFocusDistance = distanceInput;

            // If the camera is not in the started state yet, we skip applying any camera-controls
            // here. It will get applied once the camera is ready.
            if (!mSyncedMembers.mIsStarted)
                return;

            // If we are currently in QCamera::FocusModeManual, we apply the focus distance
            // immediately. Otherwise, we store the value and apply it during setFocusMode(Manual).
            if (mSyncedMembers.mAFMode == CaptureRequest.CONTROL_AF_MODE_OFF) {
                applyFocusSettingsToCaptureRequestBuilder(mPreviewRequestBuilder);

                mPreviewRequest = mPreviewRequestBuilder.build();

                try {
                    mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
                } catch (Exception exception) {
                    Log.w("QtCamera2", "Failed to set focus distance:" + exception);
                }
            }
        }
    }

    private int getTorchModeValue(boolean mode)
    {
        return mode ? CameraMetadata.FLASH_MODE_TORCH : CameraMetadata.FLASH_MODE_OFF;
    }

    @UsedFromNativeCode
    void setTorchMode(boolean torchMode)
    {
        synchronized (mSyncedMembers) {
            mSyncedMembers.mTorchMode = getTorchModeValue(torchMode);

            if (mSyncedMembers.mIsStarted) {
                mPreviewRequestBuilder.set(CaptureRequest.FLASH_MODE, mSyncedMembers.mTorchMode);
                mPreviewRequest = mPreviewRequestBuilder.build();

                try {
                    mCaptureSession.setRepeatingRequest(mPreviewRequest, mCaptureCallback, mBackgroundHandler);
                } catch (Exception exception) {
                    Log.w("QtCamera2", "Failed to set flash mode:" + exception);
                }
            }
        }
    }

    // This function is, under some circumstances, invoked indirectly on the C++ thread.
    private void applyFocusSettingsToCaptureRequestBuilder(CaptureRequest.Builder requestBuilder)
    {
        synchronized (mSyncedMembers) {
            if (!isAfModeSupported(mSyncedMembers.mAFMode)) {
                // If we don't support our desired AF_MODE, fallback to AF_MODE_OFF if that is
                // available. Otherwise don't set any focus-mode, leave it as default and
                // undefined state.
                //
                // NOTE! Setting CONTROL_AF_MODE to null is illegal and will cause an exception
                // thrown.
                if (isAfModeSupported(CaptureRequest.CONTROL_AF_MODE_OFF)) {
                    requestBuilder.set(
                        CaptureRequest.CONTROL_AF_MODE,
                        CaptureRequest.CONTROL_AF_MODE_OFF);
                }

                requestBuilder.set(CaptureRequest.LENS_FOCUS_DISTANCE, null);
                return;
            }

            requestBuilder.set(CaptureRequest.CONTROL_AF_MODE, mSyncedMembers.mAFMode);


            // Set correct lens focus distance if we are in QCamera::FocusModeManual
            if (mSyncedMembers.mAFMode == CaptureRequest.CONTROL_AF_MODE_OFF) {
                final float lensFocusDistance = calcLensFocusDistanceFromQCameraFocusDistance(
                    mSyncedMembers.mFocusDistance);
                if (lensFocusDistance < 0) {
                    Log.w(
                        "QtCamera2",
                        "Tried to apply FocusModeManual on a camera that doesn't support " +
                        "setting lens distance. Likely Qt developer bug. Ignoring.");
                } else {
                    requestBuilder.set(CaptureRequest.LENS_FOCUS_DISTANCE, lensFocusDistance);
                }
            } else {
                requestBuilder.set(CaptureRequest.LENS_FOCUS_DISTANCE, null);
            }
        }
    }

    // Calculates the CaptureRequest.LENS_FOCUS_DISTANCE equivalent given a QCamera::focusDistance
    // value. Returns -1 on failure, such as if camera does not support setting manual focus
    // distance.
    private float calcLensFocusDistanceFromQCameraFocusDistance(float qCameraFocusDistance) {
        float lensMinimumFocusDistance =
            mVideoDeviceManager.getLensInfoMinimumFocusDistance(mCameraId);
        if (lensMinimumFocusDistance <= 0)
            return -1;

        // Input is 0 to 1, with 0 meaning as close as possible.
        // Android Camera2 expects it to be in the range [0, minimumFocusDistance]
        // where higher values means closer to the camera and 0 means as far away as possible.
        // We need to map to this range.
        return (1.f - qCameraFocusDistance) * lensMinimumFocusDistance;
    }

    // Helper function to check if a given CaptureRequest.CONTROL_AF_MODE is supported on this
    // device, and we have a working implementation for it.
    private boolean isAfModeSupported(int afMode) {
        if (mVideoDeviceManager == null || mCameraId == null || mCameraId.isEmpty())
            return false;
        return mVideoDeviceManager.isAfModeSupported(mCameraId, afMode);
    }
}
