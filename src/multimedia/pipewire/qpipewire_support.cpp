// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_support_p.h"

#include <QtMultimedia/private/qmultimedia_enum_to_string_converter_p.h>

#include <QtCore/qspan.h>

QT_BEGIN_NAMESPACE

// debug support
QDebug operator<<(QDebug dbg, const spa_dict &dict)
{
    QSpan<const spa_dict_item> items{ dict.items, dict.n_items };

    QDebugStateSaver saver(dbg);
    dbg.nospace();

    for (const spa_dict_item &item : items)
        dbg << item.key << "=" << item.value << ", ";
    return dbg;
}

// clang-format off
QT_MM_MAKE_STRING_RESOLVER( pw_stream_state, QtMultimediaPrivate::EnumName,
                           (PW_STREAM_STATE_ERROR, "error")
                           (PW_STREAM_STATE_UNCONNECTED, "unconnected")
                           (PW_STREAM_STATE_CONNECTING, "connecting")
                           (PW_STREAM_STATE_PAUSED, "paused")
                           (PW_STREAM_STATE_STREAMING, "streaming")
                          );
// clang-format on

QDebug operator<<(QDebug dbg, pw_stream_state state)
{
    std::optional<QString> resolved =
            QtMultimediaPrivate::StringResolver<pw_stream_state>::toQString(state);
    if (resolved)
        return dbg << *resolved;

    return dbg << "unknown pw_stream_state";
}

QT_END_NAMESPACE
