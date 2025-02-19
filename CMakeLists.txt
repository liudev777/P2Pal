cmake_minimum_required(VERSION 3.19)
project(P2Pal_Devin_Liu LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

qt_add_executable(P2Pal_Devin_Liu
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    udphandler.h udphandler.cpp
)

target_link_libraries(P2Pal_Devin_Liu
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

include(GNUInstallDirs)

install(TARGETS P2Pal_Devin_Liu
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET P2Pal_Devin_Liu
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
