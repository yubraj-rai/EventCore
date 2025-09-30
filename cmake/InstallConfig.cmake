# Installation configuration for EventCore

# GNU install directories
include(GNUInstallDirs)

# Install libraries
install(TARGETS eventcore_static
    EXPORT EventCoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_SHARED_LIBS)
    install(TARGETS eventcore
        EXPORT EventCoreTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install executable
install(TARGETS eventcore_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY include/eventcore
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Install export targets
install(EXPORT EventCoreTargets
    FILE EventCoreTargets.cmake
    NAMESPACE EventCore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EventCore
)
