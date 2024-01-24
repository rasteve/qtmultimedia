// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegavaudioformat_p.h"
#include "qaudioformat.h"
#include "qffmpegmediaformatinfo_p.h"

extern "C" {
#include <libavutil/opt.h>
}

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

AVAudioFormat::AVAudioFormat(const AVCodecParameters *codecPar)
    : sampleFormat(AVSampleFormat(codecPar->format)), sampleRate(codecPar->sample_rate)
{
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    if (codecPar->channel_layout) {
        channelLayoutMask = codecPar->channel_layout;
    } else {
        const auto channelConfig =
                QAudioFormat::defaultChannelConfigForChannelCount(codecPar->channels);
        channelLayoutMask = QFFmpegMediaFormatInfo::avChannelLayout(channelConfig);
    }
#else
    channelLayout = codecPar->ch_layout;
#endif
}

AVAudioFormat::AVAudioFormat(const QAudioFormat &audioFormat)
    : sampleFormat(QFFmpegMediaFormatInfo::avSampleFormat(audioFormat.sampleFormat())),
      sampleRate(audioFormat.sampleRate())
{
    const auto channelConfig = audioFormat.channelConfig() == QAudioFormat::ChannelConfigUnknown
            ? QAudioFormat::defaultChannelConfigForChannelCount(audioFormat.channelCount())
            : audioFormat.channelConfig();

    const auto mask = QFFmpegMediaFormatInfo::avChannelLayout(channelConfig);

#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    channelLayoutMask = mask;
#else
    av_channel_layout_from_mask(&channelLayout, mask);
#endif
}

} // namespace QFFmpeg

QT_END_NAMESPACE
