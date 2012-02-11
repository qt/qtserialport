/*
    License...
*/

#ifndef TTYLOCKER_UNIX_P_H
#define TTYLOCKER_UNIX_P_H

#include "serialport-global.h"

QT_BEGIN_NAMESPACE_SERIALPORT

class TTYLocker
{
public:
    static bool lock(const QString &location);
    static bool unlock(const QString &location);
    static bool isLocked(const QString &location, bool *currentPid);
};

QT_END_NAMESPACE_SERIALPORT

#endif // TTYLOCKER_UNIX_P_H
