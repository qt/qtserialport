/*
    License...
*/

#ifndef TTYLOCKER_P_UNIX_H
#define TTYLOCKER_P_UNIX_H

class QString;

class TTYLocker
{
public:
    static bool lock(const QString &location);
    static bool unlock(const QString &location);
    static bool isLocked(const QString &location, bool *currentPid);
};

#endif // TTYLOCKER_P_UNIX_H
