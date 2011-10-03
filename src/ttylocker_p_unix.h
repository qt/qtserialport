/*
    License...
*/

#ifndef TTYLOCKER_P_UNIX_H
#define TTYLOCKER_P_UNIX_H

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QString;

class TTYLocker
{
public:
    static bool lock(const QString &location);
    static bool unlock(const QString &location);
    static bool isLocked(const QString &location, bool *currentPid);
};

QT_END_NAMESPACE

#endif // TTYLOCKER_P_UNIX_H
