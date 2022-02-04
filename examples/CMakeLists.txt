qt_examples_build_begin(EXTERNAL_BUILD)

if(NOT QNX)
    add_subdirectory(serialport)
endif()

qt_examples_build_end()
