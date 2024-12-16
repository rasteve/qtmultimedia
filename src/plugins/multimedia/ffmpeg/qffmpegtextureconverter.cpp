// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegtextureconverter_p.h"
#include "qffmpeghwaccel_p.h"
#include <rhi/qrhi.h>

#if QT_CONFIG(vaapi)
#  include "qffmpeghwaccel_vaapi_p.h"
#endif

#ifdef Q_OS_DARWIN
#  include "qffmpeghwaccel_videotoolbox_p.h"
#endif

#if QT_CONFIG(wmf)
#  include "qffmpeghwaccel_d3d11_p.h"
#endif

#ifdef Q_OS_ANDROID
#  include "qffmpeghwaccel_mediacodec_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

TextureConverterBackend::~TextureConverterBackend() = default;

TextureConverter::TextureConverter(QRhi &rhi) : m_rhi(rhi) { }

void TextureConverter::init(AVFrame &hwFrame)
{
    Q_ASSERT(hwFrame.hw_frames_ctx);
    AVPixelFormat fmt = AVPixelFormat(hwFrame.format);
    if (fmt != m_format)
        updateBackend(fmt);
}

QVideoFrameTexturesUPtr TextureConverter::createTextures(AVFrame &hwFrame,
                                                         QVideoFrameTexturesUPtr &oldTextures)
{
    if (isNull())
        return nullptr;

    Q_ASSERT(hwFrame.format == m_format);
    return m_backend->createTextures(&hwFrame, oldTextures);
}

QVideoFrameTexturesHandlesUPtr
TextureConverter::createTextureHandles(AVFrame &hwFrame, QVideoFrameTexturesHandlesUPtr oldHandles)
{
    if (isNull())
        return nullptr;

    Q_ASSERT(hwFrame.format == m_format);
    return m_backend->createTextureHandles(&hwFrame, std::move(oldHandles));
}

void TextureConverter::updateBackend(AVPixelFormat fmt)
{
    m_backend = nullptr;
    m_format = fmt; // init on the top; should be saved even if m_backend is not created

    if (!hwTextureConversionEnabled())
        return;

    switch (fmt) {
#if QT_CONFIG(vaapi)
    case AV_PIX_FMT_VAAPI:
        m_backend = std::make_shared<VAAPITextureConverter>(&m_rhi);
        break;
#endif
#ifdef Q_OS_DARWIN
    case AV_PIX_FMT_VIDEOTOOLBOX:
        m_backend = std::make_shared<VideoToolBoxTextureConverter>(&m_rhi);
        break;
#endif
#if QT_CONFIG(wmf)
    case AV_PIX_FMT_D3D11:
        m_backend = std::make_shared<D3D11TextureConverter>(&m_rhi);
        break;
#endif
#ifdef Q_OS_ANDROID
    case AV_PIX_FMT_MEDIACODEC:
        m_backend = std::make_shared<MediaCodecTextureConverter>(&m_rhi);
        break;
#endif
    default:
        break;
    }
}

bool TextureConverter::hwTextureConversionEnabled()
{
    // HW texture conversions are not stable in specific cases, dependent on the hardware and OS.
    // We need the env var for testing with no texture conversion on the user's side.
    static const int disableHwConversion =
            qEnvironmentVariableIntValue("QT_DISABLE_HW_TEXTURES_CONVERSION");

    return !disableHwConversion;
}

void TextureConverter::applyDecoderPreset(const AVPixelFormat format, AVCodecContext &codecContext)
{
    if (!hwTextureConversionEnabled())
        return;

    Q_ASSERT(codecContext.codec && Codec(codecContext.codec).isDecoder());

#if QT_CONFIG(wmf)
    if (format == AV_PIX_FMT_D3D11)
        D3D11TextureConverter::SetupDecoderTextures(&codecContext);
#elif defined Q_OS_ANDROID
    if (format == AV_PIX_FMT_MEDIACODEC)
        MediaCodecTextureConverter::setupDecoderSurface(&codecContext);
#else
    Q_UNUSED(codecContext);
    Q_UNUSED(format);
#endif
}

QT_END_NAMESPACE
