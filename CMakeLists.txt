cmake_minimum_required(VERSION 3.29)
project(b-Sketch)

IF(CMAKE_COMPILER_IS_GNUCXX)
    ADD_COMPILE_OPTIONS(-O3)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenMP)

add_executable(
        LifeSketch main.cpp
        tool/BOBHash32.h
        tool/loadData.h
        tool/inc.h
        report/report_memory.h
        tool/geData.h
        sketch/separate.h
        sketch/CM_sample.h
        sketch/separate_super.h
        sketch/separate_sample_super.h
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(LifeSketch PUBLIC OpenMP::OpenMP_CXX)
endif()
