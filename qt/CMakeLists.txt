cmake_minimum_required(VERSION 3.21)
project(GdipuActivityHelper VERSION 2.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Test)

option(BUILD_APP "Build the Qt Quick application" ON)

# ---- Pure C++ logic (no UI, no WebEngine) -------------------------------
add_library(campus_core STATIC
    src/core/Activity.cpp
    src/core/Timetable.cpp
    src/core/Storage.cpp
    src/core/XlsxExport.cpp
)
target_include_directories(campus_core PUBLIC src)
target_link_libraries(campus_core PUBLIC Qt6::Core PRIVATE Qt6::CorePrivate)

# ---- Tests ----------------------------------------------------------------
enable_testing()
add_executable(core_tests tests/tst_core.cpp)
target_link_libraries(core_tests PRIVATE campus_core Qt6::Test)
target_compile_definitions(core_tests PRIVATE
    REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")
add_test(NAME core_tests COMMAND core_tests)
add_executable(schedule_tests tests/tst_schedule.cpp app/ScheduleModel.cpp app/ActivityModel.cpp)
target_include_directories(schedule_tests PRIVATE app)
target_link_libraries(schedule_tests PRIVATE campus_core Qt6::Test Qt6::Gui)
add_test(NAME schedule_tests COMMAND schedule_tests)

if(BUILD_APP)
    add_subdirectory(app)
endif()
