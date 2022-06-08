// Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTUDEV_P_H
#define QTUDEV_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifdef LINK_LIBUDEV
extern "C"
{
#include <libudev.h>
}
#else
#include <QtCore/qlibrary.h>
#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/private/qglobal_p.h>

#define GENERATE_SYMBOL_VARIABLE(returnType, symbolName, ...) \
    typedef returnType (*fp_##symbolName)(__VA_ARGS__); \
    static fp_##symbolName symbolName;

#define RESOLVE_SYMBOL(symbolName) \
    symbolName = (fp_##symbolName)resolveSymbol(udevLibrary, #symbolName); \
    if (!symbolName) \
        return false;

struct udev;

#define udev_list_entry_foreach(list_entry, first_entry) \
        for (list_entry = first_entry; \
             list_entry != nullptr; \
             list_entry = udev_list_entry_get_next(list_entry))

struct udev_device;
struct udev_enumerate;
struct udev_list_entry;

GENERATE_SYMBOL_VARIABLE(struct ::udev *, udev_new);
GENERATE_SYMBOL_VARIABLE(struct ::udev_enumerate *, udev_enumerate_new, struct ::udev *)
GENERATE_SYMBOL_VARIABLE(int, udev_enumerate_add_match_subsystem, struct udev_enumerate *, const char *)
GENERATE_SYMBOL_VARIABLE(int, udev_enumerate_scan_devices, struct udev_enumerate *)
GENERATE_SYMBOL_VARIABLE(struct udev_list_entry *, udev_enumerate_get_list_entry, struct udev_enumerate *)
GENERATE_SYMBOL_VARIABLE(struct udev_list_entry *, udev_list_entry_get_next, struct udev_list_entry *)
GENERATE_SYMBOL_VARIABLE(struct udev_device *, udev_device_new_from_syspath, struct udev *udev, const char *syspath)
GENERATE_SYMBOL_VARIABLE(const char *, udev_list_entry_get_name, struct udev_list_entry *)
GENERATE_SYMBOL_VARIABLE(const char *, udev_device_get_devnode, struct udev_device *)
GENERATE_SYMBOL_VARIABLE(const char *, udev_device_get_sysname, struct udev_device *)
GENERATE_SYMBOL_VARIABLE(const char *, udev_device_get_driver, struct udev_device *)
GENERATE_SYMBOL_VARIABLE(struct udev_device *, udev_device_get_parent, struct udev_device *)
GENERATE_SYMBOL_VARIABLE(const char *, udev_device_get_subsystem, struct udev_device *)
GENERATE_SYMBOL_VARIABLE(const char *, udev_device_get_property_value, struct udev_device *, const char *)
GENERATE_SYMBOL_VARIABLE(void, udev_device_unref, struct udev_device *)
GENERATE_SYMBOL_VARIABLE(void, udev_enumerate_unref, struct udev_enumerate *)
GENERATE_SYMBOL_VARIABLE(void, udev_unref, struct udev *)

inline QFunctionPointer resolveSymbol(QLibrary *udevLibrary, const char *symbolName)
{
    QFunctionPointer symbolFunctionPointer = udevLibrary->resolve(symbolName);
    if (!symbolFunctionPointer)
        qWarning("Failed to resolve the udev symbol: %s", symbolName);

    return symbolFunctionPointer;
}

inline bool resolveSymbols(QLibrary *udevLibrary)
{
    if (!udevLibrary->isLoaded()) {
        udevLibrary->setFileNameAndVersion(QStringLiteral("udev"), 1);
        if (!udevLibrary->load()) {
            udevLibrary->setFileNameAndVersion(QStringLiteral("udev"), 0);
            if (!udevLibrary->load()) {
                qWarning("Failed to load the library: %s, supported version(s): %i and %i", qPrintable(udevLibrary->fileName()), 1, 0);
                return false;
            }
        }
    }

    RESOLVE_SYMBOL(udev_new)
    RESOLVE_SYMBOL(udev_enumerate_new)
    RESOLVE_SYMBOL(udev_enumerate_add_match_subsystem)
    RESOLVE_SYMBOL(udev_enumerate_scan_devices)
    RESOLVE_SYMBOL(udev_enumerate_get_list_entry)
    RESOLVE_SYMBOL(udev_list_entry_get_next)
    RESOLVE_SYMBOL(udev_device_new_from_syspath)
    RESOLVE_SYMBOL(udev_list_entry_get_name)
    RESOLVE_SYMBOL(udev_device_get_devnode)
    RESOLVE_SYMBOL(udev_device_get_sysname)
    RESOLVE_SYMBOL(udev_device_get_driver)
    RESOLVE_SYMBOL(udev_device_get_parent)
    RESOLVE_SYMBOL(udev_device_get_subsystem)
    RESOLVE_SYMBOL(udev_device_get_property_value)
    RESOLVE_SYMBOL(udev_device_unref)
    RESOLVE_SYMBOL(udev_enumerate_unref)
    RESOLVE_SYMBOL(udev_unref)

    return true;
}

#endif

#endif
