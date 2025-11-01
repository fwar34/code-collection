set(CMAKE_BUILD_TYPE Debug)
# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(bitmap bitmap.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加编译选项以更好地支持 C++20 特性
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "GNU G++ add compile options -std=c++20")
    target_compile_options(bitmap PRIVATE -std=c++20) # std::format在gnu++下面必须添加此编译选项
endif()