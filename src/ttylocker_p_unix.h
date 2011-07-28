/*
    License...
*/

#ifndef TTYLOCKER_P_UNIX_H
#define TTYLOCKER_P_UNIX_H

#include <QtCore/QStringList>

class TTYLocker
{
public:
    TTYLocker();
    ~TTYLocker();

    void setDeviceName(const QString &fullName) { this->m_name = fullName; }
    bool lock() const;
    bool unlock() const;
    bool locked(bool *lockedByCurrentPid) const;

private:
    QString m_name;
    int m_descriptor;

    QString shortName() const;

#if !(defined (HAVE_BAUDBOY_H) || defined (HAVE_LOCKDEV_H))

    QString getLockFileInNumericForm() const;
    QString getLockFileInNamedForm() const;
    //QString getLockFileInPidForm() const;
    QString getFirstSharedLockDir() const;

    int checkPid(int pid) const;
    bool m_locked(bool *lockedByCurrentPid) const;
    bool m_unlock() const;
    bool m_lock() const;

    QStringList lockDirList;
#endif

};

#endif // TTYLOCKER_P_UNIX_H
