// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_async_support_p.h"

#include "qpipewire_audiocontextmanager_p.h"

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

// SpaListenerBase

static std::atomic_int s_sequenceNumberAllocator;

SpaListenerBase::SpaListenerBase()
    : m_sequenceNumber(s_sequenceNumberAllocator.fetch_add(1, std::memory_order_relaxed))
{
}

void SpaListenerBase::removeHooks()
{
    spa_hook_remove(&m_listenerHook);
}

// NodeEventListener

NodeEventListener::NodeEventListener(PwNodeHandle node, NodeHandler handler)
    : m_node(std::move(node)), m_handler(std::move(handler))
{
    static constexpr struct pw_node_events nodeEvents{
        .version = PW_VERSION_NODE_EVENTS,
        .info = NodeEventListener::onInfo,
        .param = NodeEventListener::onParam,
    };

    int status = pw_node_add_listener(m_node.get(), &m_listenerHook, &nodeEvents, this);
    if (status < 0)
        qFatal() << "Failed to add listener: " << make_error_code(-status).message();
}

NodeEventListener::~NodeEventListener()
{
    removeHooks();
    QAudioContextManager::withEventLoopLock([&] {
        m_node = {};
    });
}

void NodeEventListener::enumParams(spa_param_type type)
{
    int status = pw_node_enum_params(m_node.get(), m_sequenceNumber, type, 0, 0, nullptr);
    if (status < 0)
        qCritical() << "pw_node_enum_params failed:" << make_error_code(-status).message();
}

void NodeEventListener::onInfo(void *data, const pw_node_info *info)
{
    NodeEventListener *self = reinterpret_cast<NodeEventListener *>(data);
    if (self->m_handler.infoHandler)
        self->m_handler.infoHandler(info);
}

void NodeEventListener::onParam(void *data, int seq, uint32_t id, uint32_t index, uint32_t next,
                                const spa_pod *param)
{
    NodeEventListener *self = reinterpret_cast<NodeEventListener *>(data);
    if (self->m_handler.paramHandler)
        self->m_handler.paramHandler(seq, id, index, next, param);
}

} // namespace QtPipeWire

QT_END_NAMESPACE
