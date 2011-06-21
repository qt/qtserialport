/*
    License...
*/

#include "serialport_p_win.h"


/* Public */

SerialPortPrivateWin::SerialPortPrivateWin()
    : SerialPortPrivate()
{
}

SerialPortPrivateWin::~SerialPortPrivateWin()
{
}

bool SerialPortPrivateWin::open(QIODevice::OpenMode mode)
{

}

void SerialPortPrivateWin::close()
{

}

bool SerialPortPrivateWin::dtr() const
{

}

bool SerialPortPrivateWin::rts() const
{

}

SerialPort::Lines SerialPortPrivateWin::lines() const
{

}

bool SerialPortPrivateWin::flush()
{

}

bool SerialPortPrivateWin::reset()
{

}

qint64 SerialPortPrivateWin::bytesAvailable() const
{

}

qint64 SerialPortPrivateWin::read(char *data, qint64 len)
{

}

qint64 SerialPortPrivateWin::write(const char *data, qint64 len)
{

}

bool SerialPortPrivateWin::waitForReadyRead(int msec)
{

}

bool SerialPortPrivateWin::waitForBytesWritten(int msec)
{

}

// protected

bool SerialPortPrivateWin::setNativeRate(qint32 rate, SerialPort::Directions dir)
{

}

bool SerialPortPrivateWin::setNativeDataBits(SerialPort::DataBits dataBits)
{

}

bool SerialPortPrivateWin::setNativeParity(SerialPort::Parity parity)
{

}

bool SerialPortPrivateWin::setNativeStopBits(SerialPort::StopBits stopBits)
{

}

bool SerialPortPrivateWin::setNativeFlowControl(SerialPort::FlowControl flow)
{

}

void SerialPortPrivateWin::setNativeDataInterval(int usecs)
{

}

void SerialPortPrivateWin::setNativeReadTimeout(int msecs)
{

}

void SerialPortPrivateWin::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{

}

void SerialPortPrivateWin::detectDefaultSettings()
{

}

bool SerialPortPrivateWin::saveOldsettings()
{

}

bool SerialPortPrivateWin::restoreOldsettings()
{

}

/* Private */

bool SerialPortPrivateWin::createEvents(bool rx, bool tx)
{

}

bool SerialPortPrivateWin::closeEvents() const
{

}

void SerialPortPrivateWin::recalcTotalReadTimeoutConstant()
{

}

void SerialPortPrivateWin::prepareCommTimeouts(CommTimeouts cto, DWORD msecs)
{

}

bool SerialPortPrivateWin::updateDcb()
{

}

bool SerialPortPrivateWin::updateCommTimeouts()
{

}



