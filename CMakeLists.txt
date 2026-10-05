cmake_minimum_required(VERSION 3.21)

project(GXCounterTerminal VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets SerialPort)

qt_add_executable(GXCounterTerminal WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/protocol.cpp
    src/protocol.h
    src/widgets/sevensegmentdisplay.cpp
    src/widgets/sevensegmentdisplay.h
    src/widgets/ledbar.cpp
    src/widgets/ledbar.h
)

target_link_libraries(GXCounterTerminal PRIVATE
    Qt6::Widgets
    Qt6::SerialPort
)

target_include_directories(GXCounterTerminal PRIVATE src)

set_target_properties(GXCounterTerminal PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.2 QUIET COMPONENTS Test)
    if(TARGET Qt6::Test)
        qt_add_executable(protocol_test
            tests/protocol_test.cpp
            src/protocol.cpp
            src/protocol.h
        )
        target_include_directories(protocol_test PRIVATE src)
        target_link_libraries(protocol_test PRIVATE Qt6::Test Qt6::Core)
        add_test(NAME protocol_test COMMAND protocol_test)
    endif()
endif()
