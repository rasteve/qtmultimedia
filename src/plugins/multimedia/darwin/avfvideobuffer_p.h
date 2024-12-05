// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFVIDEOBUFFER_H
#define AVFVIDEOBUFFER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qhwvideobuffer_p.h>
#include <private/qcore_mac_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <avfvideosink_p.h>

#include <CoreVideo/CVImageBuffer.h>

#import "Metal/Metal.h"
#import "MetalKit/MetalKit.h"

QT_BEGIN_NAMESPACE

struct AVFMetalTexture;
class AVFVideoBuffer : public QHwVideoBuffer
{
public:
    AVFVideoBuffer(AVFVideoSinkInterface *sink, CVImageBufferRef buffer);
    ~AVFVideoBuffer();

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    quint64 textureHandle(QRhi *, int plane) override;

    QVideoFrameFormat videoFormat() const { return m_format; }

private:
    AVFVideoSinkInterface *sink = nullptr;

    CVMetalTextureRef cvMetalTexture[3] = {};
    QCFType<CVMetalTextureCacheRef> metalCache;
#if defined(Q_OS_MACOS)
    CVOpenGLTextureRef cvOpenGLTexture = nullptr;
#elif defined(Q_OS_IOS)
    CVOpenGLESTextureRef cvOpenGLESTexture = nullptr;
#endif

    CVImageBufferRef m_buffer = nullptr;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    QVideoFrameFormat m_format;
};

QT_END_NAMESPACE

#endif
