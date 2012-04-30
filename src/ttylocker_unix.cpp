/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <scapig@yandex.ru>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "ttylocker_unix_p.h"

#if defined (HAVE_BAUDBOY_H)
#  include <baudboy.h>
#  include <cstdlib>
#elif defined (HAVE_LOCKDEV_H)
#  include <lockdev.h>
#  include <unistd.h>
#else
#  include <signal.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <QtCore/qfile.h>
#  include <QtCore/qdir.h>
#  include <QtCore/qstringlist.h>
#endif // defined (HAVE_BAUDBOY_H)

// Truncate the full name of the device to short.
// For example the long name "/dev/ttyS0" will cut up to "ttyS0".
static
QString shortNameFromLocation(const QString &location)
{
    return QDir::cleanPath(location).section(QDir::separator() , -1);
}


#if !(defined (HAVE_BAUDBOY_H) || defined (HAVE_LOCKDEV_H))

static
const char *entryLockDirectoryList[] = {
    "/var/lock",
    "/etc/locks",
    "/var/spool/locks",
    "/var/spool/uucp",
    "/tmp",
    0
};

// Returns the full path first found in the directory where you can create a lock file
// (ie a directory with access to the read/write).
// Verification of directories is of the order in accordance with the order
// of records in the variable lockDirList.
static
QString getFirstSharedLockDir()
{
    for (int i = 0; entryLockDirectoryList[i] != 0; ++i) {
        if (::access(entryLockDirectoryList[i], (R_OK | W_OK)) == 0)
            return QLatin1String(entryLockDirectoryList[i]);
    }
    return QString();
}

/*
// Returns the name of the lock file, which is tied to the
// major and minor device number, eg "LCK.30.50" etc.
static QString get_lock_file_in_numeric_form(const QString &location)
{
    QString result = getFirstSharedLockDir();
    if (!result.isEmpty()) {
        struct stat buf;
        if (::stat(location.toLocal8Bit().constData(), &buf))
            result.clear();
        else {
            result.append("/LCK.%1.%2");
            result = result.arg(major(buf.st_rdev)).arg(minor(buf.st_rdev));
        }
    }
    return result;
}
*/

// Returns the name of the lock file which is tied to the
// device name, eg "LCK..ttyS0", etc.
static
QString getLockFileInNamedForm(const QString &location)
{
    QString result(getFirstSharedLockDir());
    if (!result.isEmpty()) {
        result.append(QLatin1String("/LCK..%1"));
        result = result.arg(shortNameFromLocation(location));
    }
    return result;
}

/*
// Returns the name of the lock file, which is tied to the number of
// the process which uses a device, such as "LCK...999", etc.
static QString get_lock_file_in_pid_form(const QString &location)
{
    QString result = getFirstSharedLockDir();
    if (!result.isEmpty()) {
        result.append("/LCK...%1");
        result = result.arg(::getpid());
    }
    return result;
}
*/

#endif //!(defined (HAVE_BAUDBOY_H) || defined (HAVE_LOCKDEV_H))


QT_BEGIN_NAMESPACE_SERIALPORT

// Try lock serial device. However, other processes can not access it.
bool TTYLocker::lock(const QString &location)
{
    bool result = false;
#if defined (HAVE_BAUDBOY_H)
    if (::ttylock(shortNameFromLocation(location).toLocal8Bit().constData()))
        ::ttywait(shortNameFromLocation(location).toLocal8Bit().constData());
    result = (::ttylock(shortNameFromLocation(location).toLocal8Bit().constData()) != -1);
#elif defined (HAVE_LOCKDEV_H)
    result = (::dev_lock(shortNameFromLocation(location).toLocal8Bit().constData()) != -1);
#else
    QFile f(getLockFileInNamedForm(location));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QString content(QLatin1String("     %1 %2\x0A"));
        content = content.arg(::getpid()).arg(::getuid());

        if (f.write(content.toLocal8Bit()) > 0)
            result = true;
        f.close();
    }
#endif
    return result;
}

// Try unlock serial device. However, other processes can access it.
bool TTYLocker::unlock(const QString &location)
{
    bool result = true;
#if defined (HAVE_BAUDBOY_H)
    result = (::ttyunlock(shortNameFromLocation(location).toLocal8Bit().constData()) != -1);
#elif defined (HAVE_LOCKDEV_H)
    result = (::dev_unlock(shortNameFromLocation(location).toLocal8Bit().constData(), ::getpid()) != -1);
#else
    QFile f(getLockFileInNamedForm(location));
    f.remove();
#endif
    return result;
}

// Verifies the device is locked or not.
// If returned currentPid = true - this means that the device is locked the current process.
bool TTYLocker::isLocked(const QString &location, bool *currentPid)
{
    bool result = false;
#if defined (HAVE_BAUDBOY_H)
    result = (::ttylocked(shortNameFromLocation(location).toLocal8Bit().constData()) != -1);
    *currentPid = false;
#elif defined (HAVE_LOCKDEV_H)
    result = (::dev_testlock(shortNameFromLocation(location).toLocal8Bit().constData()) != -1);
    *currentPid = false;
#else

    enum CheckPidResult {
        CHK_PID_PROCESS_NOT_EXISTS,     /* process does not exist */
        CHK_PID_PROCESS_EXISTS_FOREIGN, /* process exists and it is "foreign" (ie not current) */
        CHK_PID_PROCESS_EXISTS_CURRENT, /* process exists and it is "their" (ie, current) */
        CHK_PID_UNKNOWN_ERROR           /* another error */
    };

    *currentPid = false;

    QFile f(getLockFileInNamedForm(location));
    if (f.exists()) {
        if (!f.open(QIODevice::ReadOnly)) {
            result = true;
        } else {
            QString content(QLatin1String(f.readAll()));
            f.close();
            bool ok = false;
            int pid = content.section(' ', 0, 0, QString::SectionSkipEmpty).toInt(&ok);
            if (ok) {

                // Checks the validity of the process number that was obtained from the Lock file.
                enum CheckPidResult pidResult = CHK_PID_UNKNOWN_ERROR;

                if (::kill(pid_t(pid), 0) == -1) {
                    pidResult = (errno == ESRCH) ?
                                (CHK_PID_PROCESS_NOT_EXISTS) : (CHK_PID_UNKNOWN_ERROR);
                } else {
                    pidResult = (::getpid() == pid) ?
                                (CHK_PID_PROCESS_EXISTS_CURRENT) : (CHK_PID_PROCESS_EXISTS_FOREIGN);
                }

                switch (pidResult) {
                case CHK_PID_PROCESS_NOT_EXISTS:
                    break;
                case CHK_PID_PROCESS_EXISTS_FOREIGN:
                    result = true;
                    break;
                case CHK_PID_PROCESS_EXISTS_CURRENT:
                    result = true;
                    *currentPid = true;
                    break;
                default:
                    result = true;
                }
            }
        }
    }
#endif
    return result;
}

QT_END_NAMESPACE_SERIALPORT
