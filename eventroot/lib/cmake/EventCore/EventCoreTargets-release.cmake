#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "EventCore::eventcore_static" for configuration "Release"
set_property(TARGET EventCore::eventcore_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(EventCore::eventcore_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libeventcore_static.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS EventCore::eventcore_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_EventCore::eventcore_static "${_IMPORT_PREFIX}/lib/libeventcore_static.a" )

# Import target "EventCore::eventcore" for configuration "Release"
set_property(TARGET EventCore::eventcore APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(EventCore::eventcore PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libeventcore.so.1.0.0"
  IMPORTED_SONAME_RELEASE "libeventcore.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS EventCore::eventcore )
list(APPEND _IMPORT_CHECK_FILES_FOR_EventCore::eventcore "${_IMPORT_PREFIX}/lib/libeventcore.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
