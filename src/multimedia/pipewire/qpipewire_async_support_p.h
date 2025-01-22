// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_ASYNC_SUPPORT_P_H
#define QPIPEWIRE_ASYNC_SUPPORT_P_H

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

#include <QtCore/qglobal.h>

#include "qpipewire_support_p.h"

#include <spa/utils/hook.h>

#include <functional>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

struct SpaListenerBase
{
    explicit SpaListenerBase();

    int sequenceNumber() const { return m_sequenceNumber; }

protected:
    void removeHooks();

    int m_sequenceNumber;
    spa_hook m_listenerHook{};
};

struct NodeEventListener final : SpaListenerBase
{
    using InfoHandler = std::function<void(const struct pw_node_info *)>;
    using ParamHandler = std::function<void(int /*seq*/, uint32_t /*id*/, uint32_t /*index*/,
                                            uint32_t /*next*/, const struct spa_pod * /*param*/)>;

    struct NodeHandler
    {
        InfoHandler infoHandler;
        ParamHandler paramHandler;
    };

    explicit NodeEventListener(PwNodeHandle, NodeHandler);
    ~NodeEventListener();

    void enumParams(spa_param_type);

private:
    PwNodeHandle m_node;
    NodeHandler m_handler;

    static void onInfo(void *userData, const struct pw_node_info *);
    static void onParam(void *userData, int seq, uint32_t id, uint32_t index, uint32_t next,
                        const struct spa_pod *);
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_ASYNC_SUPPORT_P_H
