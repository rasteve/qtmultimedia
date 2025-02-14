// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinintegration_p.h"
#include <avfmediaplayer_p.h>
#if !defined(Q_OS_VISIONOS)
#include <avfcameraservice_p.h>
#include <avfcamera_p.h>
#include <avfimagecapture_p.h>
#include <avfmediaencoder_p.h>
#include <qavfcamerabase_p.h>
#endif
#include <qdarwinformatsinfo_p.h>
#include <avfvideosink_p.h>
#include <avfaudiodecoder_p.h>
#include <VideoToolbox/VideoToolbox.h>
#include <qdebug.h>
#include <private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

class QDarwinMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "darwin.json")

public:
    QDarwinMediaPlugin()
        : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == u"darwin")
            return new QDarwinIntegration;
        return nullptr;
    }
};

QDarwinIntegration::QDarwinIntegration() : QPlatformMediaIntegration(QLatin1String("darwin"))
{
#if defined(Q_OS_MACOS) && QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_11_0)
    if (__builtin_available(macOS 11.0, *))
        VTRegisterSupplementalVideoDecoderIfAvailable(kCMVideoCodecType_VP9);
#endif
}

QPlatformMediaFormatInfo *QDarwinIntegration::createFormatInfo()
{
    return new QDarwinFormatInfo();
}

QPlatformVideoDevices *QDarwinIntegration::createVideoDevices()
{
#if defined(Q_OS_VISIONOS)
    return nullptr;
#else
    return new QAVFVideoDevices(this);
#endif
}

QMaybe<QPlatformAudioDecoder *> QDarwinIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new AVFAudioDecoder(decoder);
}

QMaybe<QPlatformMediaCaptureSession *> QDarwinIntegration::createCaptureSession()
{
#if defined(Q_OS_VISIONOS)
    return nullptr;
#else
    return new AVFCameraService;
#endif
}

QMaybe<QPlatformMediaPlayer *> QDarwinIntegration::createPlayer(QMediaPlayer *player)
{
    return new AVFMediaPlayer(player);
}

QMaybe<QPlatformCamera *> QDarwinIntegration::createCamera(QCamera *camera)
{
#if defined(Q_OS_VISIONOS)
    Q_UNUSED(camera);
    return nullptr;
#else
    return new AVFCamera(camera);
#endif
}

QMaybe<QPlatformMediaRecorder *> QDarwinIntegration::createRecorder(QMediaRecorder *recorder)
{
#if defined(Q_OS_VISIONOS)
    Q_UNUSED(recorder);
    return nullptr;
#else
    return new AVFMediaEncoder(recorder);
#endif
}

QMaybe<QPlatformImageCapture *> QDarwinIntegration::createImageCapture(QImageCapture *imageCapture)
{
#if defined(Q_OS_VISIONOS)
    Q_UNUSED(imageCapture);
    return nullptr;
#else
    return new AVFImageCapture(imageCapture);
#endif
}

QMaybe<QPlatformVideoSink *> QDarwinIntegration::createVideoSink(QVideoSink *sink)
{
    return new AVFVideoSink(sink);
}

QT_END_NAMESPACE

#include "qdarwinintegration.moc"
