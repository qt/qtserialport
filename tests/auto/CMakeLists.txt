# Generated from auto.pro.

add_subdirectory(qserialport)
add_subdirectory(qserialportinfo)
# add_subdirectory(cmake) # special case
if(QT_FEATURE_private_tests)
    add_subdirectory(qserialportinfoprivate)
endif()
