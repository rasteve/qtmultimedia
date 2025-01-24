// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiocontextmanager_p.h"

#include "qpipewire_instance_p.h"
#include "qpipewire_propertydict_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>

#include <system_error>

#if !PW_CHECK_VERSION(0, 3, 75)
extern "C" {
bool pw_check_library_version(int major, int minor, int micro);
}
#endif

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_GLOBAL_STATIC(QAudioContextManager, s_audioContextInstance);

QAudioContextManager::QAudioContextManager()
    : m_libraryInstance{
          QPipeWireInstance::instance(),
      }
{
    prepareEventLoop();
    prepareContext();
    connectToPipewireInstance();
    if (isConnected())
        startEventLoop();
}

QAudioContextManager::~QAudioContextManager()
{
    if (isConnected())
        stopEventLoop();

    m_coreConnection.reset();
    m_context.reset();
    m_eventLoop.reset();
}

bool QAudioContextManager::minimumRequirementMet()
{
    return pw_check_library_version(0, 3, 44); // we require PW_KEY_OBJECT_SERIAL
}

QAudioContextManager *QAudioContextManager::instance()
{
    return s_audioContextInstance;
}

bool QAudioContextManager::isConnected() const
{
    return bool(m_coreConnection);
}

bool QAudioContextManager::isInPwThreadLoop()
{
    return pw_thread_loop_in_thread(instance()->m_eventLoop.get());
}

pw_loop *QAudioContextManager::getEventLoop()
{
    return pw_thread_loop_get_loop(instance()->m_eventLoop.get());
}

void QAudioContextManager::prepareEventLoop()
{
    m_eventLoop = PwThreadLoopHandle{
        pw_thread_loop_new("QAudioContext", /*props=*/nullptr),
    };
    if (!m_eventLoop) {
        qFatal() << "Failed to create pipewire main loop" << make_error_code().message();
        return;
    }
}

void QAudioContextManager::startEventLoop()
{
    int status = pw_thread_loop_start(m_eventLoop.get());
    if (status < 0)
        qFatal() << "Failed to start event loop" << make_error_code(-status).message();
}

void QAudioContextManager::stopEventLoop()
{
    pw_thread_loop_stop(m_eventLoop.get());
}

void QAudioContextManager::prepareContext()
{
    PwPropertiesHandle props = makeProperties({
            { PW_KEY_APP_NAME, qApp->applicationName().toUtf8().data() },
    });

    Q_ASSERT(m_eventLoop);
    m_context = PwContextHandle{
        pw_context_new(pw_thread_loop_get_loop(m_eventLoop.get()), props.release(),
                       /*user_data_size=*/0),
    };
    if (!m_context)
        qFatal() << "Failed to create pipewire context" << make_error_code().message();
}

void QAudioContextManager::connectToPipewireInstance()
{
    Q_ASSERT(m_eventLoop && m_context);
    m_coreConnection = PwCoreConnectionHandle{
        pw_context_connect(m_context.get(), /*props=*/nullptr,
                           /*user_data_size=*/0),
    };

    if (!m_coreConnection)
        qInfo() << "Failed to connect to pipewire instance" << make_error_code().message();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
