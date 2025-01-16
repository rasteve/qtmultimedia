// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiodevices_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_instance_p.h"

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

bool QAudioDevices::isSupported()
{
    QByteArray requestedBackend = qgetenv("QT_AUDIO_BACKEND");
    if (requestedBackend == "pipewire") {
        bool pipewireAudioAvailable =
                QPipeWireInstance::isLoaded() && QAudioContextManager::instance()->isConnected();

        if (!pipewireAudioAvailable) {
            qDebug() << "PipeWire audio backend requested. not available. Using default backend";
            return false;
        }
    }

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
