cmake_minimum_required(VERSION 3.16)
project(GameCenter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Ядро (логика без интерфейса) - его же позже подключит Qt GUI.
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(gamecenter_core STATIC ${CORE_SOURCES})
target_include_directories(gamecenter_core PUBLIC include)

# Консольное приложение.
add_executable(gamecenter_console main.cpp console/ConsoleApp.cpp)
target_link_libraries(gamecenter_console PRIVATE gamecenter_core)
target_include_directories(gamecenter_console PRIVATE console)

# Графический интерфейс на Qt Widgets (собирается, только если Qt найден).
option(BUILD_GUI "Build Qt GUI" ON)
if(BUILD_GUI)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets QUIET)
    if(QT_FOUND)
        find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
        file(GLOB GUI_SOURCES CONFIGURE_DEPENDS gui/*.cpp gui/*.h)
        add_executable(gamecenter_gui ${GUI_SOURCES})
        set_target_properties(gamecenter_gui PROPERTIES AUTOMOC ON)
        target_include_directories(gamecenter_gui PRIVATE gui)
        target_link_libraries(gamecenter_gui PRIVATE gamecenter_core Qt${QT_VERSION_MAJOR}::Widgets)

        # Windows: автоматически копируем нужные DLL Qt рядом с exe.
        if(WIN32 AND TARGET Qt${QT_VERSION_MAJOR}::qmake)
            get_target_property(QT_QMAKE_EXE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
            get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXE}" DIRECTORY)
            find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
            if(WINDEPLOYQT_EXECUTABLE)
                add_custom_command(TARGET gamecenter_gui POST_BUILD
                    COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:gamecenter_gui>")
            endif()
        endif()
    else()
        message(STATUS "Qt не найден: графический интерфейс не будет собран")
    endif()
endif()

# Простые автотесты.
enable_testing()
add_executable(smoke_tests tests/smoke_test.cpp)
target_link_libraries(smoke_tests PRIVATE gamecenter_core)
add_test(NAME smoke_tests COMMAND smoke_tests)
