// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_propertydict_p.h"

#include <QtCore/qspan.h>

QT_BEGIN_NAMESPACE

#ifndef PW_KEY_DEVICE_SYSFS_PATH
#  define PW_KEY_DEVICE_SYSFS_PATH "device.sysfs.path"
#endif

namespace QtPipeWire {

PwPropertiesHandle makeProperties(QSpan<const spa_dict_item> keyValuePairs)
{
    struct spa_dict info = SPA_DICT_INIT(keyValuePairs.data(), uint32_t(keyValuePairs.size()));

    return PwPropertiesHandle{
        pw_properties_new_dict(&info),
    };
}

PwPropertyDict toPropertyDict(const spa_dict &dict)
{
    QSpan<const spa_dict_item> items{ dict.items, dict.n_items };

    PwPropertyDict map;
    for (const spa_dict_item &item : items)
        map.emplace(item.key, item.value);
    return map;
}

namespace {

std::optional<std::string_view> resolveInDictionary(const PwPropertyDict &dict,
                                                    std::string_view key)
{
    if (auto it = dict.find(key); it != dict.end())
        return it->second;
    return std::nullopt;
}

template <typename Converter>
auto resolveInDictionaryHelper(const PwPropertyDict &dict, std::string_view key,
                               Converter &&convert)
{
    using ResultType = decltype(convert(std::declval<std::string_view>()));

    if (auto it = dict.find(key); it != dict.end())
        return convert(it->second);
    return ResultType{ std::nullopt };
}

template <typename T>
struct Converter
{
};

template <>
struct Converter<uint32_t>
{
    std::optional<uint32_t> operator()(std::string_view sv) const
    {
        bool ok{};
        uint32_t ret = QLatin1StringView(sv).toInt(&ok);
        if (ok)
            return ret;

        return std::nullopt;
    }
};

template <typename T>
std::optional<T> resolveInDictionary(const PwPropertyDict &dict, std::string_view key)
{
    return resolveInDictionaryHelper(dict, key, Converter<T>{});
}

} // namespace

std::optional<std::string_view> getMediaClass(const PwPropertyDict &dict)
{
    return resolveInDictionary(dict, PW_KEY_MEDIA_CLASS);
}

std::optional<std::string_view> getNodeName(const PwPropertyDict &dict)
{
    return resolveInDictionary(dict, PW_KEY_NODE_NAME);
}

std::optional<uint32_t> getDeviceId(const PwPropertyDict &dict)
{
    return resolveInDictionary<uint32_t>(dict, PW_KEY_DEVICE_ID);
}

std::optional<std::string_view> getDeviceSysfsPath(const PwPropertyDict &dict)
{
    return resolveInDictionary(dict, PW_KEY_DEVICE_SYSFS_PATH);
}

std::optional<std::string_view> getDeviceName(const PwPropertyDict &dict)
{
    return resolveInDictionary(dict, PW_KEY_DEVICE_NAME);
}

std::optional<std::string_view> getDeviceDescription(const PwPropertyDict &dict)
{
    return resolveInDictionary(dict, PW_KEY_DEVICE_DESCRIPTION);
}

} // namespace QtPipeWire

QT_END_NAMESPACE
