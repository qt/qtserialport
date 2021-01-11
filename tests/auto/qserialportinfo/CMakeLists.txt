#####################################################################
## tst_qserialportinfo Binary:
#####################################################################

qt_internal_add_test(tst_qserialportinfo
    SOURCES
        tst_qserialportinfo.cpp
    PUBLIC_LIBRARIES
        Qt::SerialPort
        Qt::Test
)
