cmake_minimum_required(VERSION 3.20)
project(cooling_telemetry_monitor VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(ctm_core STATIC
    src/model_sensor.cpp
    src/sampler.cpp
    src/filters.cpp
    src/acquisition_pipeline.cpp
)
target_include_directories(ctm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(ctm_acq src/main.cpp)
target_link_libraries(ctm_acq PRIVATE ctm_core)

enable_testing()
add_executable(ctm_tests tests/test_main.cpp)
target_link_libraries(ctm_tests PRIVATE ctm_core)
add_test(NAME ctm_unit_tests COMMAND ctm_tests)
