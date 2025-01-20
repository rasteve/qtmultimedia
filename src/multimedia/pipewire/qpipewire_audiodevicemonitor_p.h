// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIODEVICEMONITOR_P_H
#define QPIPEWIRE_AUDIODEVICEMONITOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qfiledevice.h>

#include <pipewire/pipewire.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

enum class PipewireRegistryType : uint8_t
{
    Client,
    Core,
    Device,
    Factory,
    Link,
    Metadata,
    Module,
    Node,
    Port,
    Profiler,
    Registry,
    SecurityContext,
};

class QAudioDeviceMonitor : public QObject
{
public:
    void objectAdded(uint32_t id, uint32_t permissions, const char *type, uint32_t version,
                     const spa_dict *props);

    void objectRemoved(uint32_t id);
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_AUDIODEVICEMONITOR_P_H
