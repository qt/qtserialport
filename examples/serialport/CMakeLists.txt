# Generated from serialport.pro.

add_subdirectory(cenumerator)
add_subdirectory(creaderasync)
add_subdirectory(creadersync)
add_subdirectory(cwriterasync)
add_subdirectory(cwritersync)
if (TARGET Qt6::Widgets) # special case
    add_subdirectory(enumerator)
    add_subdirectory(terminal)
    add_subdirectory(blockingmaster)
    add_subdirectory(blockingslave)
    add_subdirectory(master)
    add_subdirectory(slave)
endif()
