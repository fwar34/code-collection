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

# 参数 / 部分	含义说明
# copy_compile_commands	自定义目标的名称（可任意命名，用于标识该目标）。
# ALL	表示该目标会被加入到默认构建目标中（即执行 make 或 cmake --build 时会自动运行，无需手动指定）。
# COMMAND	定义该目标要执行的命令：
# ${CMAKE_COMMAND} 是 CMake 可执行程序的路径（跨平台兼容）；
# -E copy_if_difference 是 CMake 内置的命令行工具，用于条件复制文件（仅当源文件与目标文件内容不同时才复制，避免重复操作）。
# 源文件路径	${CMAKE_BINARY_DIR}/compile_commands.json
# CMAKE_BINARY_DIR 是构建目录（即运行 cmake 时的目录，通常是 build），compile_commands.json 是编译数据库文件（由 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 生成，用于 IDE 代码补全、静态分析等）。
# 目标文件路径	${PROJECT_ROOT_DIR}/compile_commands.json
# PROJECT_SOURCE_DIR 是项目根目录（即 CMakeLists.txt 所在目录），表示要将文件复制到项目根目录。
# DEPENDS	声明该目标的依赖：必须等待 ${CMAKE_BINARY_DIR}/compile_commands.json 生成后，才会执行复制命令（避免源文件不存在时出错）。
# COMMENT	构建时输出的提示信息（如 Copying compile_commands.json to project root...），方便查看进度。
# VERBATIM	确保命令中的参数被原样传递（尤其是路径含空格或特殊字符时，避免解析错误），建议始终添加。
add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
    DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
    COMMENT "Copying compile_commands.json to project root..."
    VERBATIM
)