# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries



#### Tests

# ntddmodm
qt_config_compile_test(ntddmodm
    LABEL "ntddmodm"
    CODE
"
#include <windows.h>
#include <ntddmodm.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
GUID guid = GUID_DEVINTERFACE_MODEM;
    /* END TEST: */
    return 0;
}
")



#### Features

qt_feature("ntddmodm" PRIVATE
    LABEL "ntddmodm"
    CONDITION TEST_ntddmodm
    DISABLE INPUT_ntddmodm STREQUAL 'no'
)
qt_feature_definition("ntddmodm" "QT_NO_REDEFINE_GUID_DEVINTERFACE_MODEM")
qt_configure_add_summary_section(NAME "Serial Port")
qt_configure_add_summary_entry(ARGS "ntddmodm")
qt_configure_end_summary_section() # end of "Serial Port" section
