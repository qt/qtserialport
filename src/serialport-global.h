/*
    License...
*/

#ifndef SERIALPORT_GLOBAL_H
#define SERIALPORT_GLOBAL_H

#include "qglobal.h"

#if defined(QT_ADDON_SERIALPORT_LIB)
#  define Q_ADDON_SERIALPORT_EXPORT Q_DECL_EXPORT
#else
#  define Q_ADDON_SERIALPORT_EXPORT Q_DECL_IMPORT
#endif

#if defined(QT_NAMESPACE)
#  define QT_BEGIN_NAMESPACE_SERIALPORT namespace QT_NAMESPACE { namespace QtAddOn { namespace SerialPort {
#  define QT_END_NAMESPACE_SERIALPORT } } }
#  define QT_USE_NAMESPACE_SERIALPORT using namespace QT_NAMESPACE::QtAddOn::SerialPort;
#  define QT_PREPEND_NAMESPACE_SERIALPORT(name) ::QT_NAMESPACE::QtAddOn::SerialPort::name
#else
#  define QT_BEGIN_NAMESPACE_SERIALPORT namespace QtAddOn { namespace SerialPort {
#  define QT_END_NAMESPACE_SERIALPORT } }
#  define QT_USE_NAMESPACE_SERIALPORT using namespace QtAddOn::SerialPort;
#  define QT_PREPEND_NAMESPACE_SERIALPORT(name) ::QtAddOn::SerialPort::name
#endif

// a workaround for moc - if there is a header file that doesn't use serialport
// namespace, we still force moc to do "using namespace" but the namespace have to
// be defined, so let's define an empty namespace here
QT_BEGIN_NAMESPACE_SERIALPORT
QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORT_GLOBAL_H
