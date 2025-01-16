// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOCONTEXTMANAGER_P_H
#define QPIPEWIRE_AUDIOCONTEXTMANAGER_P_H

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

#include <pipewire/pipewire.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QPipeWireInstance;

class QAudioContextManager
{
public:
    QAudioContextManager();
    ~QAudioContextManager();

    static QAudioContextManager *instance();
    bool isConnected() const;

    template <typename Closure>
    static auto withEventLoopLock(Closure &&c)
    {
        QAudioContextManager *self = instance();

        pw_thread_loop_lock(self->m_eventLoop.get());
        auto unlock = qScopeGuard([&] {
            pw_thread_loop_unlock(self->m_eventLoop.get());
        });

        return c();
    }

    static bool isInPwThreadLoop();
    static pw_loop *getEventLoop();

private:
    std::shared_ptr<QPipeWireInstance> m_libraryInstance;

    // event loop
    PwThreadLoopHandle m_eventLoop;
    void prepareEventLoop();
    void startEventLoop();
    void stopEventLoop();

    // pipewire context
    PwContextHandle m_context;
    void prepareContext();

    // pw_core connection
    PwCoreConnectionHandle m_coreConnection;
    void connectToPipewireInstance();
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_AUDIOCONTEXTMANAGER_P_H
