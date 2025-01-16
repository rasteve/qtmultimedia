// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiodevices_p.h"

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

bool QAudioDevices::isSupported()
{
    return false;
}

QList<QAudioDevice> QAudioDevices::findAudioInputs() const
{
    return {};
}

QList<QAudioDevice> QAudioDevices::findAudioOutputs() const
{
    return {};
}

QPlatformAudioSource *QAudioDevices::createAudioSource(const QAudioDevice &, QObject *)
{
    return {};
}

QPlatformAudioSink *QAudioDevices::createAudioSink(const QAudioDevice &, QObject *)
{
    return {};
}
} // namespace QtPipeWire

QT_END_NAMESPACE
