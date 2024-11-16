// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGAUDIOENCODERUTILS_P_H
#define QFFMPEGAUDIOENCODERUTILS_P_H

#include "qffmpeg_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

AVSampleFormat adjustSampleFormat(const AVSampleFormat *supportedFormats, AVSampleFormat requested);

int adjustSampleRate(const int *supportedRates, int requested);

ChannelLayoutT adjustChannelLayout(const ChannelLayoutT *supportedLayouts,
                                   const ChannelLayoutT &requested);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGAUDIOENCODERUTILS_P_H
