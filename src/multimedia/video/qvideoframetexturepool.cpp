// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframetexturepool_p.h"
#include "qvideotexturehelper_p.h"

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

void QVideoFrameTexturePool::setCurrentFrame(QVideoFrame frame) {
    m_texturesDirty = true;
    m_currentFrame = std::move(frame);
}

QVideoFrameTextures* QVideoFrameTexturePool::updateTextures(QRhi &rhi, QRhiResourceUpdateBatch &rub) {
    Q_ASSERT(size_t(rhi.currentFrameSlot()) < MaxSlotsCount);

    m_texturesDirty = false;
    QVideoFrameTexturesUPtr &textures = m_textureSlots[rhi.currentFrameSlot()];
    textures = QVideoTextureHelper::createTextures(m_currentFrame, rhi, &rub, std::move(textures));
    return textures.get();
}

QT_END_NAMESPACE
