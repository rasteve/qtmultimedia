// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOREAUDIOUTILS_P_H
#define QCOREAUDIOUTILS_P_H

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

#include <CoreAudio/CoreAudioTypes.h>

#include <QtMultimedia/QAudioFormat>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QCoreAudioUtils
{

Q_MULTIMEDIA_EXPORT QAudioFormat toQAudioFormat(const AudioStreamBasicDescription& streamFormat);
AudioStreamBasicDescription toAudioStreamBasicDescription(QAudioFormat const& audioFormat);

Q_MULTIMEDIA_EXPORT std::unique_ptr<AudioChannelLayout> toAudioChannelLayout(const QAudioFormat &format, UInt32 *size);
QAudioFormat::ChannelConfig fromAudioChannelLayout(const AudioChannelLayout *layout);

} // namespace QCoreAudioUtils

QT_END_NAMESPACE

#endif // QCOREAUDIOUTILS_P_H
