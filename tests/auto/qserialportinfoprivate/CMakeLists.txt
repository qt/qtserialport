#####################################################################
## tst_qserialportinfoprivate Binary:
#####################################################################

qt_internal_add_test(tst_qserialportinfoprivate
    SOURCES
        tst_qserialportinfoprivate.cpp
    PUBLIC_LIBRARIES
        Qt::SerialPortPrivate
        Qt::Test
)
