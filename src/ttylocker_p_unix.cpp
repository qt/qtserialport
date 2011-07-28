/*
    License...
*/

#include "ttylocker_p_unix.h"

/* this directive by default will not work because when creating Makefile no checks on the existence of:
1. header: <baudboy.h>
ie using qmake to do this is impossible (difficult).
it is possible to solve the transition to CMake*/
#if defined (HAVE_BAUDBOY_H)
#  include <baudboy.h>
#  include <cstdlib>

/* this directive by default will not work because when creating Makefile no checks on the existence of:
1. header: <lockdev.h>
2. library:-llockdev
ie using qmake to do this is impossible (difficult).
it is possible to solve the transition to CMake*/
#elif defined (HAVE_LOCKDEV_H)
#  include <lockdev.h>
#  include <unistd.h>

#else
#  include <signal.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/stat.h>

#  include <QtCore/QFile>
#  include <QtCore/QDir>
#  include <QtCore/QStringList>

#endif

#include <QtCore/QString>


// Truncate the full name of the device to short.
// For example the long name "/dev/ttyS0" will cut up to "ttyS0".
static QString short_name_from_location(const QString &location)
{
    return QDir::cleanPath(location).section(QDir::separator() , -1);
}


#if !(defined (HAVE_BAUDBOY_H) || defined (HAVE_LOCKDEV_H))

enum {
    LOCK_DIRS_COUNT = 5,
    LOCK_FILE_FORMS_COUNT = 2
};

static const char *lock_dir_list[LOCK_DIRS_COUNT] = {
    "/var/lock",
    "/etc/locks",
    "/var/spool/locks",
    "/var/spool/uucp",
    "/tmp"
};

// Returns the full path first found in the directory where you can create a lock file
// (ie a directory with access to the read/write).
// Verification of directories is of the order in accordance with the order
// of records in the variable lockDirList.
static QString get_first_shared_lock_dir()
{
    for (int i = 0; i < LOCK_DIRS_COUNT; ++i) {
        if (::access(lock_dir_list[i], (R_OK | W_OK)) == 0)
            return QString(lock_dir_list[i]);
    }
    return QString();
}

// Returns the name of the lock file, which is tied to the
// major and minor device number, eg "LCK.30.50" etc.
static QString get_lock_file_in_numeric_form(const QString &location)
{
    QString result = get_first_shared_lock_dir();
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

// Returns the name of the lock file which is tied to the
// device name, eg "LCK..ttyS0", etc.
static QString get_lock_file_in_named_form(const QString &location)
{
    QString result = get_first_shared_lock_dir();
    if (!result.isEmpty()) {
        result.append("/LCK..%1");
        result = result.arg(short_name_from_location(location));
    }
    return result;
}

// Returns the name of the lock file, which is tied to the number of
// the process which uses a device, such as "LCK...999", etc.
//static QString get_lock_file_in_pid_form(const QString &location)
//{
//    QString result = get_first_shared_lock_dir();
//    if (!result.isEmpty()) {
//        result.append("/LCK...%1");
//        result = result.arg(::getpid());
//    }
//    return result;
//}

enum {
    CHK_PID_PROCESS_NOT_EXISTS,
    CHK_PID_PROCESS_EXISTS_FOREIGN,
    CHK_PID_PROCESS_EXISTS_CURRENT,
    CHK_PID_UNKNOWN_ERROR
};

// Checks the validity of the process number that was obtained from the Lock file.
// Returns the following values:
//  CHK_PID_PROCESS_NOT_EXISTS     - process does not exist
//  CHK_PID_PROCESS_EXISTS_FOREIGN - process exists and it is "foreign" (ie not current)
//  CHK_PID_PROCESS_EXISTS_CURRENT - process exists and it is "their" (ie, current)
//  CHK_PID_UNKNOWN_ERROR          - another error
static int check_pid(int pid)
{
    if (::kill(pid_t(pid), 0) == -1) {
        return (errno == ESRCH) ?
                    (CHK_PID_PROCESS_NOT_EXISTS) : (CHK_PID_UNKNOWN_ERROR);
    }
    return (::getpid() == pid) ?
                (CHK_PID_PROCESS_EXISTS_CURRENT) : (CHK_PID_PROCESS_EXISTS_FOREIGN);
}

static bool m_islocked(const QString &location, bool *current_pid)
{
    QFile f; //<- lock file
    bool result = false;
    *current_pid = false;

    for (int i = 0; i < LOCK_FILE_FORMS_COUNT; ++i) {
        switch (i) {
        case 0:
            f.setFileName(get_lock_file_in_numeric_form(location));
            break;
        case 1:
            f.setFileName(get_lock_file_in_named_form(location));
            break;
            //case 2:
            //    f.setFileName(get_lock_file_in_pid_form(location));
            //    break;
        default: ;
        }

        if (!f.exists())
            continue;
        if (!f.open(QIODevice::ReadOnly)) {
            result = true;
            break;
        }

        QString content(f.readAll());
        f.close();

        int pid = content.section(' ', 0, 0, QString::SectionSkipEmpty).toInt();

        switch (check_pid(pid)) {
        case CHK_PID_PROCESS_NOT_EXISTS:
            break;
        case CHK_PID_PROCESS_EXISTS_FOREIGN:
            result = true;
            break;
        case CHK_PID_PROCESS_EXISTS_CURRENT:
            result = true;
            *current_pid = true;
            break;
        default:
            result = true;
        }

        if (result)
            break;
    }
    return result;
}

static bool m_unlock(const QString &location) {
    QFile f;
    for (int i = 0; i < LOCK_FILE_FORMS_COUNT; ++i) {
        switch (i) {
        case 0:
            f.setFileName(get_lock_file_in_numeric_form(location));
            break;
        case 1:
            f.setFileName(get_lock_file_in_named_form(location));
            break;
        //case 2:
        //    f.setFileName(get_lock_file_in_pid_form(location));
        default:
            continue;
        }
        f.remove();
    }
    return true;
}

bool m_lock(const QString &location)
{
    bool result = true;
    int flags = (O_WRONLY | O_CREAT | O_EXCL);
    int mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    for (int i = 0; i < LOCK_FILE_FORMS_COUNT; ++i) {
        QString f;
        switch (i) {
        case 0:
            f = get_lock_file_in_numeric_form(location);
            break;
        case 1:
            f = get_lock_file_in_named_form(location);
            break;
        //case 2:
        //    f = get_lock_file_in_pid_form(location);
        //    break;
        default: ;
        }

        int fd = ::open(f.toLocal8Bit().constData(), flags, mode);

        if (fd == -1) {
            result = false;
            break;
        }

        // Make lock file content.
        f = "     %1 %2\x0A";
        f = f.arg(::getpid()).arg(::getuid());

        if (::write(fd, f.toLocal8Bit().constData(), f.length()) == -1)
            result = false;

        ::close(fd);
        if (!result)
            break;
    }
    if (!result)
        m_unlock(location);
    return result;
}

#endif//HAVE_BAUDBOY_H, HAVE_LOCKDEV_H


// Try lock serial device. However, other processes can not access it.
bool TTYLocker::lock(const QString &location)
{
    bool result = false;
#if defined (HAVE_BAUDBOY_H)
    if (::ttylock(short_name_from_location(location).toLocal8Bit().constData()))
        ::ttywait(short_name_from_location(location).toLocal8Bit().constData());
    result = (::ttylock(short_name_from_location(location).toLocal8Bit().constData()) != -1);
#elif defined (HAVE_LOCKDEV_H)
    result = (::dev_lock(short_name_from_location(location).toLocal8Bit().constData()) != -1);
#else
    result = m_lock(location);
#endif
    return result;
}

// Try unlock serial device. However, other processes can access it.
bool TTYLocker::unlock(const QString &location)
{
    bool result = false;
#if defined (HAVE_BAUDBOY_H)
    result = (::ttyunlock(short_name_from_location(location).toLocal8Bit().constData()) != -1);
#elif defined (HAVE_LOCKDEV_H)
    result = (::dev_unlock(short_name_from_location(location).toLocal8Bit().constData(), ::getpid()) != -1);
#else
    result = m_unlock(location);
#endif
    return result;
}

// Verifies the device is locked or not.
// If returned currentPid = true - this means that the device is locked the current process.
bool TTYLocker::isLocked(const QString &location, bool *currentPid)
{
    bool result = false;
#if defined (HAVE_BAUDBOY_H)
    result = (::ttylocked(short_name_from_location(location).toLocal8Bit().constData()) != -1);
    *currentPid = false;
#elif defined (HAVE_LOCKDEV_H)
    result = (::dev_testlock(short_name_from_location(location).toLocal8Bit().constData()) != -1);
    *currentPid = false;
#else
    result = m_islocked(location, currentPid);
#endif
    return result;
}
