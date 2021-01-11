#####################################################################
## tst_qserialport Binary:
#####################################################################

qt_internal_add_test(tst_qserialport
    SOURCES
        tst_qserialport.cpp
    PUBLIC_LIBRARIES
        Qt::SerialPort
        Qt::Test
)
