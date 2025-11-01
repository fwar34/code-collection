cmake_minimum_required(VERSION 3.15)

project(code-collection)

# 使用方法
# 只编译 C++ 目录
#cmake -DBUILD_CXX=ON -DBUILD_JAVA=OFF ..
# 只编译 Java 目录
#cmake -DBUILD_CXX=OFF -DBUILD_JAVA=ON ..
# 同时编译两个目录
#cmake -DBUILD_CXX=ON -DBUILD_JAVA=ON ..

option(BUILD_CXX "Build C_C++" ON)
option(BUILD_OTHER "Build Other" OFF)

if(BUILD_CXX)
    add_subdirectory(C_C++)
endif()

if(BUILD_OTHER)
    add_subdirectory(Other)
endif()