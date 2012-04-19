/*
    License...
*/

#include "serialport.h"
#include "serialportinfo.h"
#include "serialport_p.h"
#include "serialportengine_p.h"

#if QT_VERSION >= 0x040700
#  include <QtCore/qelapsedtimer.h>
#else
#  include <QtCore/qdatetime.h>
#endif

#ifndef SERIALPORT_BUFFERSIZE
#  define SERIALPORT_BUFFERSIZE 16384
#endif

#ifndef SERIALPORT_READ_CHUNKSIZE
#  define SERIALPORT_READ_CHUNKSIZE 256
#endif

QT_BEGIN_NAMESPACE_SERIALPORT

//----------------------------------------------------------------

/* Public methods */

/*! \internal

    Constructs a SerialPortPrivate. Initializes all members.
*/
SerialPortPrivate::SerialPortPrivate(SerialPort *parent)
    : readBufferMaxSize(0)
    , readBuffer(SERIALPORT_BUFFERSIZE)
    , writeBuffer(SERIALPORT_BUFFERSIZE)
    , isBuffered(true)
    , readSerialNotifierCalled(false)
    , readSerialNotifierState(false)
    , readSerialNotifierStateSet(false)
    , emittedReadyRead(false)
    , emittedBytesWritten(false)
    , portError(SerialPort::NoError)
    , engine(0)
    , q_ptr(parent)

{
    engine = SerialPortEngine::create(this);
    Q_ASSERT(engine);
}

/*! \internal

    Destructs the SerialPort. Also if the native engine is
    exists, then delete it.
*/
SerialPortPrivate::~SerialPortPrivate()
{
    if (engine)
        delete engine;
}

/*! \internal

    Sets the internal variable system location, by converting
    the serial \a port name. Converting serial port name does
    native engine, this conversion is platform-dependent.
    That is, performed the converting inverse by
    method SerialPortPrivate::port().
*/
void SerialPortPrivate::setPort(const QString &port)
{
    options.systemLocation = engine->toSystemLocation(port);
}

/*! \internal

    Returns the serial port name obtained by converting internal
    the system location variable. Converting system location does
    native engine, this conversion is platform-dependent. That is,
    performed the converting inverse by method
    SerialPortPrivate::setPort().
*/
QString SerialPortPrivate::port() const
{
    return engine->fromSystemLocation(options.systemLocation);
}

/*! \internal

    Opens the serial port with a given internal system location
    variable for the given open \a mode. This operation is
    platform-dependent, and it provides native engine.
*/
bool SerialPortPrivate::open(QIODevice::OpenMode mode)
{
    return engine->open(options.systemLocation, mode);
}

/*! \internal

    Closes the serial port specified in the internal variable
    as system location. This operation is platform-dependent,
    and it provides native engine.
*/
void SerialPortPrivate::close()
{
    engine->close(options.systemLocation);
}

/*! \internal

    Sets the desired speed \a rate for a given direction \a dir
    for the serial port. This operation is platform-dependent,
    and it provides native engine.
*/
bool SerialPortPrivate::setRate(qint32 rate, SerialPort::Directions dir)
{
    if (engine->setRate(rate, dir)) {
        if (dir & SerialPort::Input)
            options.inputRate = rate;
        if (dir & SerialPort::Output)
            options.outputRate = rate;
        return true;
    }
    return false;
}

/*! \internal

    Returns the current value of the speed of a given direction
    \a dir from the corresponding internal variables.
*/
qint32 SerialPortPrivate::rate(SerialPort::Directions dir) const
{
    if (dir == SerialPort::AllDirections)
        return (options.inputRate == options.outputRate) ? (options.inputRate) : SerialPort::UnknownRate;
    return (dir & SerialPort::Input) ? (options.inputRate) : (options.outputRate);
}

/*! \internal

    Sets the desired number of data bits \a dataBits in byte
    for the serial port. This operation is platform-dependent,
    and it provides native engine.
*/
bool SerialPortPrivate::setDataBits(SerialPort::DataBits dataBits)
{
    if (engine->setDataBits(dataBits)) {
        options.dataBits = dataBits;
        return true;
    }
    return false;
}

/*! \internal

    Returns the current number of data bits in byte from the
    corresponding internal variable.
*/
SerialPort::DataBits SerialPortPrivate::dataBits() const
{
    return options.dataBits;
}

/*! \internal

    Sets the desired \a parity control mode for the serial port.
    This operation is platform-dependent, and it provides
    native engine.
*/
bool SerialPortPrivate::setParity(SerialPort::Parity parity)
{
    if (engine->setParity(parity)) {
        options.parity = parity;
        return true;
    }
    return false;
}

/*! \internal

    Returns the current parity control mode from the
    corresponding internal variable.
*/
SerialPort::Parity SerialPortPrivate::parity() const
{
    return options.parity;
}

/*! \internal

    Sets the desired number of stop bits \a stopBits in frame for
    the serial port. This operation is platform-dependent, and
    it provides native engine.
*/
bool SerialPortPrivate::setStopBits(SerialPort::StopBits stopBits)
{
    if (engine->setStopBits(stopBits)) {
        options.stopBits = stopBits;
        return true;
    }
    return false;
}

/*! \internal

    Returns the current number of stop bits in frame from the
    corresponding internal variable.
*/
SerialPort::StopBits SerialPortPrivate::stopBits() const
{
    return options.stopBits;
}

/*! \internal

    Sets the desired \a flow control mode for the serial port.
    This operation is platform-dependent, and it provides
    native engine.
*/
bool SerialPortPrivate::setFlowControl(SerialPort::FlowControl flow)
{
    if (engine->setFlowControl(flow)) {
        options.flow = flow;
        return true;
    }
    return false;
}

/*! \internal

    Returns the current flow control mode from the
    corresponding internal variable.
*/
SerialPort::FlowControl SerialPortPrivate::flowControl() const
{
    return options.flow;
}

/*! \internal

    Returns DTR signal state. This operation is platform-dependent,
    and it provides native engine.
*/
bool SerialPortPrivate::dtr() const
{
    return engine->lines() & SerialPort::Dtr;
}

/*! \internal

    Returns RTS signal state. This operation is platform-dependent,
    and it provides native engine.
*/
bool SerialPortPrivate::rts() const
{
    return engine->lines() & SerialPort::Rts;
}

/*! \internal

    Set desired DTR signal state \a set. This operation is
    platform-dependent, and it provides native engine.
*/
bool SerialPortPrivate::setDtr(bool set)
{
    return engine->setDtr(set);
}

/*! \internal

    Set desired RTS signal state \a set. This operation is
    platform-dependent, and it provides native engine.
*/
bool SerialPortPrivate::setRts(bool set)
{
    return engine->setRts(set);
}

/*! \internal

    Returns the bitmap signals of RS-232 lines. This operation is
    platform-dependent, and it provides native engine.
*/
SerialPort::Lines SerialPortPrivate::lines() const
{
    return engine->lines();
}

/*! \internal

    Writes pending data in the write buffers to the serial port.
    The function writes as much as it can without blocking.

    It is usually invoked by canWriteNotification after one or more
    calls to write().

    Emits bytesWritten().
*/
bool SerialPortPrivate::flush()
{
    Q_Q(SerialPort);

    if (writeBuffer.isEmpty())
        return false;

    int nextSize = writeBuffer.nextDataBlockSize();
    const char *ptr = writeBuffer.readPointer();

    // Attempt to write it all in one chunk.
    qint64 written = write(ptr, nextSize);
    if (written < 0) {
        writeBuffer.clear();
        return false;
    }

    // Remove what we wrote so far.
    writeBuffer.free(written);
    if (written > 0) {
        // Don't emit bytesWritten() recursively.
        if (!emittedBytesWritten) {
            emittedBytesWritten = true;
            emit q->bytesWritten(written);
            emittedBytesWritten = false;
        }
    }

    if (writeBuffer.isEmpty() && engine->isWriteNotificationEnabled())
        engine->setWriteNotificationEnabled(false);

    return true;
}

/*! \internal

    Resets and clears the receiving and transmitting driver buffers
    (UART). This operation is platform-dependent, and it set
    provides native engine.
*/
bool SerialPortPrivate::reset()
{
    return engine->reset();
}

/*! \internal

    Sends a continuous stream of zero bits during a specified period
    of time \a duration in msec. This operation is platform-dependent,
    and it set provides native engine.
*/
bool SerialPortPrivate::sendBreak(int duration)
{
    return engine->sendBreak(duration);
}

/*! \internal

    Control the signal break, depending on the \a set. This operation
    is platform-dependent, and it set provides native engine.
*/
bool SerialPortPrivate::setBreak(bool set)
{
    return engine->setBreak(set);
}

/*! \internal

    Sets the type of \a policy processing received symbol at parity
    error is detected the corresponding internal variable. The value
    of this variable is further taken into account when processing
    received symbol in the implementation of native methods of reading
    in native engine.
*/
bool SerialPortPrivate::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    const bool ret = (options.policy == policy) || engine->setDataErrorPolicy(policy);
    if (ret)
        options.policy = policy;
    return ret;
}

/*! \internal

    Returns the current value internal policy variable.
*/
SerialPort::DataErrorPolicy SerialPortPrivate::dataErrorPolicy() const
{
    return options.policy;
}

/*! \internal

    Returns the current error code.
*/
SerialPort::PortError SerialPortPrivate::error() const
{
    return portError;
}

/*! \internal

    Clears error code.
*/
void SerialPortPrivate::unsetError()
{
    portError = SerialPort::NoError;
}

/*! \internal

    Set \a error code in the corresponding internal variable.
*/
void SerialPortPrivate::setError(SerialPort::PortError error)
{
    portError = error;
}

/*! \internal

    Returns the number of bytes available for reading, which
    are in the receive buffer driver (UART). This operation
    is platform-dependent, and it set provides native engine.
*/
qint64 SerialPortPrivate::bytesAvailable() const
{
    return engine->bytesAvailable();
}

/*! \internal

    Returns the number of bytes ready to send to that are in
    the transmit buffer driver (UART). This operation
    is platform-dependent, and it set provides native engine.
*/
qint64 SerialPortPrivate::bytesToWrite() const
{
    return engine->bytesToWrite();
}

/*! \internal

    Reads data into the pointer \a data of length \a len,
    which are located in the receive buffer driver (UART).
    This operation is platform-dependent, and it set provides
    native engine.
*/
qint64 SerialPortPrivate::read(char *data, qint64 len)
{
    return engine->read(data, len);
}

/*! \internal

    Write data from the pointer \a data of length \a len,
    to the transmit buffer driver (UART).
    This operation is platform-dependent, and it set provides
    native engine.
*/
qint64 SerialPortPrivate::write(const char *data, qint64 len)
{
    return engine->write(data, len);
}

/*! \internal

    Implements a generic "wait" method, expected within the \a timeout
    in millisecond occurrence of any of the two events:

    - appearance in the reception buffer driver (UART) at least one
    character, if the flag \a checkRead is set on true
    - devastation of the transmit buffer driver (UART) with the transfer
    of the last character, if the flag \a checkWrite is set on true

    Also, all these events can happen simultaneously. The result of
    catch of the corresponding event returns to the variables
    \a selectForRead and \a selectForWrite. This operation is
    platform-dependent, and it set provides native engine.
*/
bool SerialPortPrivate::waitForReadOrWrite(int timeout,
                                           bool checkRead, bool checkWrite,
                                           bool *selectForRead, bool *selectForWrite)
{
    return engine->select(timeout, checkRead, checkWrite, selectForRead, selectForWrite);
}

/*! \internal

    Clears internal read and write buffers of the class.
*/
void SerialPortPrivate::clearBuffers()
{
    writeBuffer.clear();
    readBuffer.clear();
}

/*! \internal

    Reads data from the serial port into the read buffer. Returns
    true on success; otherwise returns false. This operation takes place
    automatically when the driver (UART) in the input buffer have
    of at least one byte, i.e. by event.
*/
bool SerialPortPrivate::readFromPort()
{
    qint64 bytesToRead = (options.policy == SerialPort::IgnorePolicy) ?
                (SERIALPORT_READ_CHUNKSIZE) : 1;

    if (bytesToRead <= 0)
        return false;

    if (readBufferMaxSize
            && (bytesToRead > (readBufferMaxSize - readBuffer.size()))) {

        bytesToRead = readBufferMaxSize - readBuffer.size();
    }

    char *ptr = readBuffer.reserve(bytesToRead);
    qint64 readBytes = read(ptr, bytesToRead);

    if (readBytes <= 0) {
        readBuffer.chop(bytesToRead);
        return false;
    }
    readBuffer.chop(int(bytesToRead - ((readBytes < 0) ? qint64(0) : readBytes)));
    return true;
}

/*! \internal

    This method is called from native engine when new data is
    available for reading, i.e. is the handler read notification.
    Handles recursive calls.
*/
bool SerialPortPrivate::canReadNotification()
{
    Q_Q(SerialPort);

#if defined (Q_OS_WINCE)
    engine->lockNotification(SerialPortEngine::CanReadLocker, true);
#endif
    // Prevent recursive calls.
    if (readSerialNotifierCalled) {
        if (!readSerialNotifierStateSet) {
            readSerialNotifierStateSet = true;
            readSerialNotifierState = engine->isReadNotificationEnabled();
            engine->setReadNotificationEnabled(false);
        }
    }
    readSerialNotifierCalled = true;

    //if (!isBuffered)
    //    this->serialEngine->setReadNotificationEnabled(false);

    // If buffered, read data from the serial into the read buffer.
    qint64 newBytes = 0;
    if (isBuffered) {
        // Return if there is no space in the buffer.
        if (readBufferMaxSize
                && (readBuffer.size() >= readBufferMaxSize)) {

            readSerialNotifierCalled = false;
            return false;
        }

        newBytes = readBuffer.size();

        if (!readFromPort()) {
            readSerialNotifierCalled = false;
            return false;
        }
        newBytes = readBuffer.size() - newBytes;

        // If read buffer is full, disable the read serial notifier.
        if (readBufferMaxSize
                && (readBuffer.size() == readBufferMaxSize)) {

            engine->setReadNotificationEnabled(false);
        }
    }

    // Only emit readyRead() when not recursing,
    // and only if there is data available.
    const bool hasData = (isBuffered) ? (newBytes > 0) : true;

    if ((!emittedReadyRead) && hasData) {
        emittedReadyRead = true;
        emit q->readyRead();
        emittedReadyRead = false;
    }

    if ((!hasData) && engine->isReadNotificationEnabled())
        engine->setReadNotificationEnabled(true);

    // Reset the read serial notifier state if we reentered inside the
    // readyRead() connected slot.
    if (readSerialNotifierStateSet &&
            (readSerialNotifierState != engine->isReadNotificationEnabled())) {

        engine->setReadNotificationEnabled(readSerialNotifierState);
        readSerialNotifierStateSet = false;
    }
    readSerialNotifierCalled = false;
    return true;
}

/*! \internal

    This method is called from native engine when the serial
    port ready for writing, i.e. is the handler write notification.
    Handles recursive calls.
*/
bool SerialPortPrivate::canWriteNotification()
{
#if defined (Q_OS_WINCE)
    engine->lockNotification(SerialPortEngine::CanWriteLocker, true);
#endif

#if defined (Q_OS_WIN)
    if (engine->isWriteNotificationEnabled())
        engine->setWriteNotificationEnabled(false);
#endif

    int tmp = writeBuffer.size();
    flush();

#if defined (Q_OS_WIN)
    if (!writeBuffer.isEmpty())
        engine->setWriteNotificationEnabled(true);
#else
    if (writeBuffer.isEmpty())
        engine->setWriteNotificationEnabled(false);
#endif
    return (writeBuffer.size() < tmp);
}

/*! \internal

    This method is called from native engine when the serial
    port hardware detect an I/O error. For example, the error
    parity, frame, etc., i.e. is the handler error notification.
    Handles recursive calls.
*/
bool SerialPortPrivate::canErrorNotification()
{
#if defined (Q_OS_WINCE)
    engine->lockNotification(SerialPortEngine::CanErrorLocker, true);
#endif
    return engine->processIOErrors();
}

//----------------------------------------------------------------
//************* SerialPort

/*!
    \class SerialPort

    \brief The SerialPort class provides functionality to access
    serial ports.

    \reentrant
    \ingroup serialport-main
    \inmodule QtAddOnSerialPort
    \since 5.0

    The class QAbstractSocket was chosen as base of SerialPort,
    so their functionality and behavior is similar in many cases.
    For example, in terms of I/O operations, the implementation
    of the wait methods, the internal architecture etc.
    Especially, the implementation of some methods of SerialPort
    take directly from QAbstractSocket with minimal changes.

    Features of the implementation and conduct of the class are
    listed below:

    \list
    \o Provides only common functionality which includes
    configuring, I/O data stream, get and set control signals
    RS-232 lines.
    \o Not supported terminal features as echo, control CR/LF,
    etc.
    \o Always works in binary mode.
    \o Do not support the native ability for configuring timeouts
    and delays during the reading.
    \o Does not provide tracking and notification when the state
    of RS-232 lines was changed.
    \endlist

    To get started with the SerialPort, first create an object of
    that class.

    Next, call setPort() to assign the object with the name of the
    desired serial port (which has to be present in the system).
    The name must follow a certain format, which is platform dependent.

    The helper class SerialPortInfo allows to enumerate all
    serial ports in the system. This is useful to obtain the
    correct serial port name.

    You can also use SerialPortInfo as an input parameter to the
    method setPort() (to retrieve the currently assigned name,
    use portName()).

    Now the serial port can be opened with open(). This is possible
    in read-only (r/o), write-only (w/o) or read-write (r/w) mode.

    Note: The serial port is always opened with exclusive access
    (i. e. no other process or thread can access an already opened
    serial port).

    With the successful opening, the SerialPort determines
    it's current configuration and initializes itself to this
    configuration. To access the current configuration use rate(),
    dataBits(), parity(), stopBits(), and flowControl().

    If you are satisfied with these settings, you can proceed with I/O
    operation. Otherwise you can reconfigure the port to the desired
    setting using setRate(), setDataBits(), setParity(), setStopBits(),
    and setFlowControl().

    Read or write data by calling read() or write(), or use the
    convenience functions readLine() and readAll(). SerialPort also
    inherits getChar(), putChar(), and ungetChar() from QIODevice,
    which work on single bytes. The bytesWritten() signal is
    emitted when data has been written to the serial port. Note
    that Qt does not limit the write buffer size. You can monitor
    it's size by listening to this signal.

    The readyRead() signal is emitted every time a new chunk of data
    has arrived. bytesAvailable() then returns the number of bytes
    that are available for reading. Typically, you would connect the
    readyRead() signal to a slot and read all available data there.

    If you don't read all the data at once, the remaining data will
    still be available later, and any new incoming data will be
    appended to SerialPort's internal read buffer. To limit the size
    of the read buffer, call setReadBufferSize().

    The status of the control lines is determined with dtr(), rts()
    and lines(). To change the control line status, use setDtr() and
    setRts().

    To close the serial port, call close(). After all pending data
    has been written to the serial port, SerialPort actually
    closes the descriptor.

    SerialPort provides a set of functions that suspend the calling
    thread until certain signals are emitted. These functions can be
    used to implement blocking serial ports:

    \list
    \o waitForReadyRead() blocks until new data is available for
    reading.

    \o waitForBytesWritten() blocks until one payload of data has been
    written to the serial port.
    \endlist

    We show an example:

    \code
     int numRead = 0, numReadTotal = 0;
     char buffer[50];

     forever {
         numRead  = serial.read(buffer, 50);

         // do whatever with array

         numReadTotal += numRead;
         if (numRead == 0 && !serial.waitForReadyRead())
             break;
     }
    \endcode

    If \l{QIODevice::}{waitForReadyRead()} returns false, the
    connection has been closed or an error has occurred.

    Programming with a blocking serial port is radically different from
    programming with a non-blocking serial port. A blocking serial port
    doesn't require an event loop and typically leads to simpler code.
    However, in a GUI application, blocking serial port should only be
    used in non-GUI threads, to avoid freezing the user interface.

    See the \l examples/terminal and \l examples/blockingterminal
    examples for an overview of both approaches.

    We discourage the use of the blocking functions together
    with signals. One of the two possibilities should be used.

    SerialPort can be used with QTextStream and QDataStream's stream
    operators (operator<<() and operator>>()). There is one issue to
    be aware of, though: You must make sure that enough data is
    available before attempting to read it using operator>>().

    \sa SerialPortInfo
*/

/*!
    \enum SerialPort::Direction

    This enum describes the possibly direction of data transmission.
    Note: It is used to set the speed of the device separately for
    each direction in some operating systems (e. g. POSIX-like).

    \value Input Input direction.
    \value Output Output direction
    \value AllDirections Simultaneously in two directions.
*/

/*!
    \enum SerialPort::Rate

    This enum describes the baud rate at which the communications
    device operates. Note: only the most common standard rates
    are listed in this enum.

    \value Rate1200 1200 baud.
    \value Rate2400 2400 baud.
    \value Rate4800 4800 baud.
    \value Rate9600 9600 baud.
    \value Rate19200 19200 baud.
    \value Rate38400 38400 baud.
    \value Rate57600 57600 baud.
    \value Rate115200 115200 baud.
    \value UnknownRate Unknown baud rate.

    \sa setRate(), rate()
*/

/*!
    \enum SerialPort::DataBits

    This enum describes possibly the number of bits in the bytes
    transmitted and received.

    \value Data5 Five bits.
    \value Data6 Six bits
    \value Data7 Seven bits
    \value Data8 Eight bits.
    \value UnknownDataBits Unknown number of bits.

    \sa setDataBits(), dataBits()
*/

/*!
    \enum SerialPort::Parity

    This enum describes the used parity scheme.

    \value NoParity No parity.
    \value EvenParity Even parity.
    \value OddParity Odd parity.
    \value SpaceParity Space parity.
    \value MarkParity Mark parity.
    \value UnknownParity Unknown parity.

    \sa setParity(), parity()
*/

/*!
    \enum SerialPort::StopBits

    This enum describes the used number of stop bits.

    \value OneStop 1 stop bit.
    \value OneAndHalfStop 1.5 stop bits.
    \value TwoStop 2 stop bits.
    \value UnknownStopBits Unknown number of stop bit.

    \sa setStopBits(), stopBits()
*/

/*!
    \enum SerialPort::FlowControl

    This enum describes the used flow control.

    \value NoFlowControl No flow control.
    \value HardwareControl Hardware flow control (RTS/CTS).
    \value SoftwareControl Software flow control (XON/XOFF).
    \value UnknownFlowControl Unknown flow control.

    \sa setFlowControl(), flowControl()
*/

/*!
    \enum SerialPort::Line

    This enum describes possibly RS-232 pinout signals.

    \value Le DSR (data set ready/line enable).
    \value Dtr DTR (data terminal ready).
    \value Rts RTS (request to send).
    \value St Secondary TXD (transmit).
    \value Sr Secondary RXD (receive).
    \value Cts CTS (clear to send).
    \value Dcd DCD (data carrier detect).
    \value Ri RNG (ring).
    \value Dsr DSR (data set ready).

    \sa lines(), setDtr(), setRts(), dtr(), rts()
*/

/*!
    \enum SerialPort::DataErrorPolicy

    This enum describes policies manipulate for received symbols
    with detected parity errors.

    \value SkipPolicy Skip the bad character.
    \value PassZeroPolicy Replaces bad character to zero.
    \value IgnorePolicy Ignore the error for a bad character.
    \value StopReceivingPolicy Stopping data reception on error.
    \value UnknownPolicy Unknown policy.

    \sa setDataErrorPolicy(), dataErrorPolicy()
*/

/*!
    \enum SerialPort::PortError

    This enum describes the errors that may be returned by the error()
    function.

    \value NoError No error occurred.
    \value NoSuchDeviceError An error occurred when it attempts to
           open no existing device.
    \value PermissionDeniedError An error occurred when it attempts to
           open is already opened device by another process or user do
           not have permission to open.
    \value DeviceAlreadyOpenedError An error occurred when it attempts to
           reused open already opened device in this object.
    \value DeviceIsNotOpenedError An error occurred when it attempts to
           control still closed device.
    \value ParityError The hardware detected a parity error
           when reading data.
    \value FramingError The hardware detected a framing error
           when reading data.
    \value BreakConditionError The hardware detected a break condition on
           the input line.
    \value IoError I/O error occurred when read/write data.
    \value UnsupportedPortOperationError The requested device operation is
           not supported or prohibited by the local operating system.
    \value UnknownPortError An unidentified error occurred.

    \sa error(), unsetError()
*/



/* Public methods */

/*!
    Constructs a new serial port object with the given \a parent.
*/
SerialPort::SerialPort(QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate(this))
{}

/*!
    Constructs a new serial port object with the given \a parent
    to represent the serial port with the specified \a name.

    The name should have a specific format, see setPort().
*/
SerialPort::SerialPort(const QString &name, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate(this))
{
    setPort(name);
}

/*!
    Constructs a new serial port object with the given \a parent
    to represent the serial port with the specified a helper
    class \a info.
*/
SerialPort::SerialPort(const SerialPortInfo &info, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate(this))
{
    setPort(info);
}

/*!
    Destroys the serial port object, closing it if necessary.
*/
SerialPort::~SerialPort()
{
    /**/
    close();
    delete d_ptr;
}

/*!
    Sets the \a name of the port. The name may be in any format:
    either short, or also as system location (with all the prefixes and
    postfixed). As a result, this name will automatically be written
    and converted into an internal variable as system location.

    \sa portName(), SerialPortInfo
*/
void SerialPort::setPort(const QString &name)
{
    Q_D(SerialPort);
    d->setPort(name);
}

/*!
    Sets the port that are stored in instance \a info.

    \sa portName(), SerialPortInfo
*/
void SerialPort::setPort(const SerialPortInfo &info)
{
    Q_D(SerialPort);
    d->setPort(info.systemLocation());
}

/*!
    Returns the name set by setPort() or to the SerialPort constructors.
    This name is short, i.e. it extract and convert out from the internal
    variable system location of the device. Conversion algorithm is
    platform specific:
    \table
    \header
        \o Platform
        \o Brief Description
    \row
        \o Windows
        \o Removes the prefix "\\\\.\\" from the system location
           and returns the remainder of the string.
    \row
        \o Windows CE
        \o Removes the postfix ":" from the system location
           and returns the remainder of the string.
    \row
        \o Symbian
        \o Returns the system location as it is,
           as it is equivalent to the port name.
    \row
        \o GNU/Linux
        \o Removes the prefix "/dev/" from the system location
           and returns the remainder of the string.
    \row
        \o Mac OSX
        \o Removes the prefix "/dev/cu." and "/dev/tty." from the
           system location and returns the remainder of the string.
    \row
        \o Other *nix
        \o The same as for GNU/Linux.
    \endtable

    \sa setPort(), SerialPortInfo::portName()
*/
QString SerialPort::portName() const
{
    Q_D(const SerialPort);
    return d->port();
}

/*! \reimp
    Opens the serial port using OpenMode \a mode, returning true if
    successful; otherwise returns false with saved error code which can be
    obtained by calling error().

    \warning The \a mode must be QIODevice::ReadOnly, QIODevice::WriteOnly,
    or QIODevice::ReadWrite. It may also have additional flags, such as
    QIODevice::Unbuffered. Other modes unsupported.

    \sa QIODevice::OpenMode, setPort()
*/
bool SerialPort::open(OpenMode mode)
{
    Q_D(SerialPort);

    if (isOpen()) {
        d->setError(SerialPort::DeviceAlreadyOpenedError);
        return false;
    }

    // Define while not supported modes.
    static OpenMode unsupportedModes = (Append | Truncate | Text);
    if ((mode & unsupportedModes) || (mode == NotOpen)) {
        d->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    unsetError();
    if (d->open(mode)) {
        QIODevice::open(mode);
        d->clearBuffers();

        if (mode & ReadOnly)
            d->engine->setReadNotificationEnabled(true);
        if (mode & WriteOnly)
            d->engine->setWriteNotificationEnabled(true);

        d->engine->setErrorNotificationEnabled(true);

        d->isBuffered = !(mode & Unbuffered);
        return true;
    }
    return false;
}

/*! \reimp
    Calls SerialPort::flush() and closes the serial port.
    Errors from flush are ignored.

    \sa QIODevice::close()
*/
void SerialPort::close()
{
    Q_D(SerialPort);
    if (!isOpen()) {
        d->setError(SerialPort::DeviceIsNotOpenedError);
        return;
    }

    flush();
    QIODevice::close();
    d->engine->setReadNotificationEnabled(false);
    d->engine->setWriteNotificationEnabled(false);
    d->engine->setErrorNotificationEnabled(false);
    d->clearBuffers();
    d->close();
}

/*!
    Sets or clear the flag \a restore, which allows to restore the
    previous settings on closing the serial port. If it flag
    is true that the settings will be restored; otherwise not.
    The default SerialPort is configured to restore the settings.

    \sa restoreSettingsOnClose()
*/
void SerialPort::setRestoreSettingsOnClose(bool restore)
{
    Q_D( SerialPort);
    d->options.restoreSettingsOnClose = restore;
}

/*!
    Returns the current status of the restore flag settings on
    closing the port. The default SerialPort is configured to
    restore the settings.

    \sa setRestoreSettingsOnClose()
*/
bool SerialPort::restoreSettingsOnClose() const
{
    Q_D(const SerialPort);
    return d->options.restoreSettingsOnClose;
}

/*!
    Sets the desired data rate \a rate for a given direction \a dir.
    If successful, returns true; otherwise returns false with saved error
    code which can be obtained by calling error(). To set the speed
    can use enumeration SerialPort::Rate or any positive qint32 value.

    \warning For OS Windows, Windows CE, Symbian supported only
    AllDirections flag.

    \sa rate()
*/
bool SerialPort::setRate(qint32 rate, Directions dir)
{
    Q_D(SerialPort);
    return d->setRate(rate, dir);
}

/*!
    Returns the current baud rate of the chosen direction \a dir.

    \warning For OS Windows, Windows CE, Symbian return
    equal rate in any direction.

    \sa setRate()
*/
qint32 SerialPort::rate(Directions dir) const
{
    Q_D(const SerialPort);
    return d->rate(dir);
}

/*!
    Sets the desired number of data bits \a dataBits in byte.
    If successful, returns true; otherwise returns false with saved error
    code which can be obtained by calling error().

    \sa dataBits()
*/
bool SerialPort::setDataBits(DataBits dataBits)
{
    Q_D(SerialPort);
    return d->setDataBits(dataBits);
}

/*!
    Returns the current number of data bits in byte.

    \sa setDataBits()
*/
SerialPort::DataBits SerialPort::dataBits() const
{
    Q_D(const SerialPort);
    return d->dataBits();
}

/*!
    Sets the desired parity \a parity checking mode.
    If successful, returns true; otherwise returns false with saved error
    code which can be obtained by calling error().

    \sa parity()
*/
bool SerialPort::setParity(Parity parity)
{
    Q_D(SerialPort);
    return d->setParity(parity);
}

/*!
    Returns the current parity checking mode.

    \sa setParity()
*/
SerialPort::Parity SerialPort::parity() const
{
    Q_D(const SerialPort);
    return d->parity();
}

/*!
    Sets the desired number of stop bits \a stopBits in frame.
    If successful, returns true; otherwise returns false with saved error
    code which can be obtained by calling error().

    \sa stopBits()
*/
bool SerialPort::setStopBits(StopBits stopBits)
{
    Q_D(SerialPort);
    return d->setStopBits(stopBits);
}

/*!
    Returns the current number of stop bits.

    \sa setStopBits()
*/
SerialPort::StopBits SerialPort::stopBits() const
{
    Q_D(const SerialPort);
    return d->stopBits();
}

/*!
    Sets the desired number flow control mode \a flow.
    If successful, returns true; otherwise returns false with saved error
    code which can be obtained by calling error().

    \sa flowControl()
*/
bool SerialPort::setFlowControl(FlowControl flow)
{
    Q_D(SerialPort);
    return d->setFlowControl(flow);
}

/*!
    Returns the current flow control mode.

    \sa setFlowControl()
*/
SerialPort::FlowControl SerialPort::flowControl() const
{
    Q_D(const SerialPort);
    return d->flowControl();
}

/*!
    Returns the current state of the line signal DTR.
    If the signal state high, the return true;
    otherwise returns false;

    \sa lines()
*/
bool SerialPort::dtr() const
{
    Q_D(const SerialPort);
    return d->dtr();
}

/*!
    Returns the current state of the line signal RTS.
    If the signal state high, the return true;
    otherwise returns false;

    \sa lines()
*/
bool SerialPort::rts() const
{
    Q_D(const SerialPort);
    return d->rts();
}

/*!
    Returns the bitmap states of line signals.
    From this result it is possible to allocate the state of the
    desired signal by applying a mask "AND", where the mask is
    the desired enum from SerialPort::Lines.

    \sa dtr(), rts(), setDtr(), setRts()
*/
SerialPort::Lines SerialPort::lines() const
{
    Q_D(const SerialPort);
    return d->lines();
}

/*!
    This function writes as much as possible from the internal write
    buffer to the underlying serial port, without blocking. If any data
    was written, this function returns true; otherwise returns false is returned.

    Call this function if you need SerialPort to start sending buffered
    data immediately. The number of bytes successfully written depends on
    the operating system. In most cases, you do not need to call this
    function, because SerialPort will start sending data automatically
    once control goes back to the event loop. In the absence of an event
    loop, call waitForBytesWritten() instead.

    \sa write(), waitForBytesWritten()
*/
bool SerialPort::flush()
{
    Q_D(SerialPort);
    return d->flush() || d->engine->flush();
}

/*! \reimp
    Resets and clears all buffers of the serial port, including an
    internal class buffer and the UART (driver) buffer. If successful,
    returns true; otherwise returns false.
*/
bool SerialPort::reset()
{
    Q_D(SerialPort);
    d->clearBuffers();
    return d->reset();
}

/*! \reimp

    Returns true if no more data is currently
    available for reading; otherwise returns false.

    This function is most commonly used when reading data from the
    serial port in a loop. For example:

    \code
    // This slot is connected to SerialPort::readyRead()
    void SerialPortClass::readyReadSlot()
    {
        while (!port.atEnd()) {
            QByteArray data = port.read(100);
            ....
        }
    }
    \endcode

     \sa bytesAvailable(), readyRead()
 */
bool SerialPort::atEnd() const
{
    Q_D(const SerialPort);
    return QIODevice::atEnd() && (!isOpen() || d->readBuffer.isEmpty());
}

/*!
    Sets the error policy \a policy process received character in
    the case of parity error detection. If successful, returns
    true; otherwise returns false. By default is set policy IgnorePolicy.

    \sa dataErrorPolicy()
*/
bool SerialPort::setDataErrorPolicy(DataErrorPolicy policy)
{
    Q_D(SerialPort);
    return d->setDataErrorPolicy(policy);
}

/*!
    Returns current error policy.

    \sa setDataErrorPolicy()
*/
SerialPort::DataErrorPolicy SerialPort::dataErrorPolicy() const
{
    Q_D(const SerialPort);
    return d->dataErrorPolicy();
}

/*!
    Returns the serial port error status.

    The I/O device status returns an error code. For example, if open()
    returns false, or a read/write operation returns -1, this function can
    be called to find out the reason why the operation failed.

    \sa unsetError()
*/
SerialPort::PortError SerialPort::error() const
{
    Q_D(const SerialPort);
    return d->error();
}

/*!
    Sets the serial port's error to SerialPort::NoError.

    \sa error()
*/
void SerialPort::unsetError()
{
    Q_D(SerialPort);
    d->unsetError();
}

/*!
    Returns the size of the internal read buffer. This limits the
    amount of data that the client can receive before you call read()
    or readAll().

    A read buffer size of 0 (the default) means that the buffer has
    no size limit, ensuring that no data is lost.

    \sa setReadBufferSize(), read()
*/
qint64 SerialPort::readBufferSize() const
{
    Q_D(const SerialPort);
    return d->readBufferMaxSize;
}

/*!
    Sets the size of SerialPort's internal read buffer to be \a
    size bytes.

    If the buffer size is limited to a certain size, SerialPort
    won't buffer more than this size of data. Exceptionally, a buffer
    size of 0 means that the read buffer is unlimited and all
    incoming data is buffered. This is the default.

    This option is useful if you only read the data at certain points
    in time (e.g., in a real-time streaming application) or if you
    want to protect your serial port against receiving too much data,
    which may eventually cause your application to run out of memory.

    \sa readBufferSize(), read()
*/
void SerialPort::setReadBufferSize(qint64 size)
{
    Q_D(SerialPort);

    if (d->readBufferMaxSize == size)
        return;
    d->readBufferMaxSize = size;
    if (!d->readSerialNotifierCalled && d->engine) {
        // Ensure that the read notification is enabled if we've now got
        // room in the read buffer.
        // But only if we're not inside canReadNotification --
        // that will take care on its own.
        if (size == 0 || d->readBuffer.size() < size)
            d->engine->setReadNotificationEnabled(true);
    }
}

/*! \reimp
    Always returns true. Serial port is a sequential device.
*/
bool SerialPort::isSequential() const
{
    return true;
}

/*! \reimp
    Returns the number of incoming bytes that are waiting to be read.

    \sa bytesToWrite(), read()
*/
qint64 SerialPort::bytesAvailable() const
{
    Q_D(const SerialPort);
    qint64 ret;
    if (d->isBuffered)
        ret = qint64(d->readBuffer.size());
    else
        ret = d->bytesAvailable();
    return ret + QIODevice::bytesAvailable();
}

/*! \reimp
    Returns the number of bytes that are waiting to be written. The
    bytes are written when control goes back to the event loop or
    when flush() is called.

    \sa bytesAvailable(), flush()
*/
qint64 SerialPort::bytesToWrite() const
{
    Q_D(const SerialPort);
    qint64 ret;
    if (d->isBuffered)
        ret = qint64(d->writeBuffer.size());
    else
        ret = d->bytesToWrite();
    return ret + QIODevice::bytesToWrite();
}

/*! \reimp
    Returns true if a line of data can be read from the serial port;
    otherwise returns false.

    \sa readLine()
*/
bool SerialPort::canReadLine() const
{
    Q_D(const SerialPort);
    bool hasLine = d->readBuffer.canReadLine();
    return (hasLine || QIODevice::canReadLine());
}

// Returns the difference between msecs and elapsed. If msecs is -1,
// however, -1 is returned.
static int qt_timeout_value(int msecs, int elapsed)
{
    if (msecs == -1) { return msecs; }
    msecs -= elapsed;
    return (msecs < 0) ? 0 : msecs;
}

/*! \reimp
    This function blocks until new data is available for reading and the
    \l{QIODevice::}{readyRead()} signal has been emitted. The function
    will timeout after \a msecs milliseconds.

    The function returns true if the readyRead() signal is emitted and
    there is new data available for reading; otherwise it returns false
    (if an error occurred or the operation timed out).

    \sa waitForBytesWritten()
*/
bool SerialPort::waitForReadyRead(int msecs)
{
    Q_D(SerialPort);

    if (d->isBuffered && (!d->readBuffer.isEmpty()))
        return true;

    if (d->engine->isReadNotificationEnabled())
        d->engine->setReadNotificationEnabled(false);

#if QT_VERSION >= 0x040700
    QElapsedTimer stopWatch;
#else
    QTime stopWatch;
#endif

    stopWatch.start();

    forever {
        bool readyToRead = false;
        bool readyToWrite = false;
        if (!d->waitForReadOrWrite(qt_timeout_value(msecs, stopWatch.elapsed()),
                                   true, (!d->writeBuffer.isEmpty()),
                                   &readyToRead, &readyToWrite)) {
            return false;
        }
        if (readyToRead) {
            if (d->canReadNotification())
                return true;
        }
        if (readyToWrite)
            d->canWriteNotification();
    }
    return false;
}

/*! \reimp
*/
bool SerialPort::waitForBytesWritten(int msecs)
{
    Q_D(SerialPort);

    if (d->isBuffered && d->writeBuffer.isEmpty())
        return false;

#if QT_VERSION >= 0x040700
    QElapsedTimer stopWatch;
#else
    QTime stopWatch;
#endif

    stopWatch.start();

    forever {
        bool readyToRead = false;
        bool readyToWrite = false;
        if (!d->waitForReadOrWrite(qt_timeout_value(msecs, stopWatch.elapsed()),
                                   true, (!d->writeBuffer.isEmpty()),
                                   &readyToRead, &readyToWrite)) {
            return false;
        }
        if (readyToRead) {
            if(!d->canReadNotification())
                return false;
        }
        if (readyToWrite) {
            if (d->canWriteNotification()) {
                return true;
            }
        }
    }
    return false;
}

/* Public slots */

/*!
    Sets the desired state of the line signal DTR,
    depending on the flag \a set. If successful, returns true;
    otherwise returns false.

    If the flag is true then DTR signal is established in the high;
    otherwise low.

    \sa lines(), dtr()
*/
bool SerialPort::setDtr(bool set)
{
    Q_D(SerialPort);
    return d->setDtr(set);
}

/*!
    Sets the desired state of the line signal RTS,
    depending on the flag \a set. If successful, returns true;
    otherwise returns false.

    If the flag is true then RTS signal is established in the high;
    otherwise low.

    \sa lines(), rts()
*/
bool SerialPort::setRts(bool set)
{
    Q_D(SerialPort);
    return d->setRts(set);
}

/*!
    Sends a continuous stream of zero bits during a specified period
    of time \a duration in msec if the terminal is using asynchronous
    serial data. If successful, returns true; otherwise returns false.

    If duration is zero then zero bits are transmitted by at least
    0.25 seconds, but no more than 0.5 seconds.

    If duration is non zero then zero bits are transmitted within a certain
    period of time depending on implementation.

    \sa setBreak(), clearBreak()
*/
bool SerialPort::sendBreak(int duration)
{
    Q_D(SerialPort);
    return d->sendBreak(duration);
}

/*!
    Control the signal break, depending on the flag \a set.
    If successful, returns true; otherwise returns false.

    If \a set is true then enable break transmission; otherwise disable.

    \sa clearBreak(), sendBreak()
*/
bool SerialPort::setBreak(bool set)
{
    Q_D(SerialPort);
    return d->setBreak(set);
}

/* Protected methods */

/*! \reimp
*/
qint64 SerialPort::readData(char *data, qint64 maxSize)
{
    Q_D(SerialPort);

    if (d->engine->isReadNotificationEnabled() && d->isBuffered)
        d->engine->setReadNotificationEnabled(true);

    if (!d->isBuffered) {
        qint64 readBytes = d->read(data, maxSize);
        return readBytes;
    }

    if (d->readBuffer.isEmpty())
        return qint64(0);

    // If readFromSerial() read data, copy it to its destination.
    if (maxSize == 1) {
        *data = d->readBuffer.getChar();
        return 1;
    }

    qint64 bytesToRead = qMin(qint64(d->readBuffer.size()), maxSize);
    qint64 readSoFar = 0;
    while (readSoFar < bytesToRead) {
        const char *ptr = d->readBuffer.readPointer();
        int bytesToReadFromThisBlock = qMin(int(bytesToRead - readSoFar),
                                            d->readBuffer.nextDataBlockSize());
        memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
        readSoFar += bytesToReadFromThisBlock;
        d->readBuffer.free(bytesToReadFromThisBlock);
    }
    return readSoFar;
}

/*! \reimp
*/
qint64 SerialPort::readLineData(char *data, qint64 maxSize)
{
    return QIODevice::readLineData(data, maxSize);
}

/*! \reimp
*/
qint64 SerialPort::writeData(const char *data, qint64 maxSize)
{
    Q_D(SerialPort);

    if (!d->isBuffered) {
        qint64 written = d->write(data, maxSize);
        if (written < 0) {
            //Error
        } else if (!d->writeBuffer.isEmpty()) {
            d->engine->setWriteNotificationEnabled(true);
        }

        if (written >= 0)
            emit bytesWritten(written);

        return written;
    }

    char *ptr = d->writeBuffer.reserve(maxSize);
    if (maxSize == 1)
        *ptr = *data;
    else
        memcpy(ptr, data, maxSize);

    if (!d->writeBuffer.isEmpty())
        d->engine->setWriteNotificationEnabled(true);

    return maxSize;
}

/*!
    \fn bool SerialPort::clearBreak(bool clear)

    Control the signal break, depending on the flag \a clear.
    If successful, returns true; otherwise returns false.

    If clear is false then enable break transmission; otherwise disable.

    \sa setBreak(), sendBreak()
*/

#include "moc_serialport.cpp"

QT_END_NAMESPACE_SERIALPORT
