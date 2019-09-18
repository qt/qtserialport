# Generated from qserialportinfoprivate.pro.

#####################################################################
## tst_qserialportinfoprivate Binary:
#####################################################################

add_qt_test(tst_qserialportinfoprivate
    SOURCES
        tst_qserialportinfoprivate.cpp
    LIBRARIES
        Qt::SerialPortPrivate
    PUBLIC_LIBRARIES
        Qt::SerialPort
        Qt::Test
)
