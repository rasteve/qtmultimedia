// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiodevicemonitor_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_registry_support_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/private/qflatmap_p.h>

#include <q20vector.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

void QAudioDeviceMonitor::objectAdded(uint32_t /*id*/, uint32_t /*permissions*/, const char *type,
                                      uint32_t /*version*/, const spa_dict * /*props*/)
{
    Q_ASSERT(QAudioContextManager::isInPwThreadLoop());

    auto typeInRegistry = parsePipewireRegistryType(type);
    if (!typeInRegistry)
        return;

    switch (*typeInRegistry) {
    case PipewireRegistryType::Factory:
    case PipewireRegistryType::Registry:
    case PipewireRegistryType::Module:
        return; // ignore, we're not interested in these

    default:
        return; // added device
    }
}

void QAudioDeviceMonitor::objectRemoved(uint32_t /*id*/)
{
    Q_ASSERT(QAudioContextManager::isInPwThreadLoop());
}

} // namespace QtPipeWire

QT_END_NAMESPACE
