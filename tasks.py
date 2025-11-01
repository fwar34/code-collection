# 导入系统模块
import subprocess  # 用于创建子进程和执行外部命令
import argparse    # 用于解析命令行参数
import os          # 用于与操作系统交互，如文件路径操作
import shutil      # 用于高级文件操作，如复制、删除目录树等
import sys         # 用于访问Python解释器相关的变量和函数
import threading   # 用于实现多线程并发处理

# 定义CMake可执行文件名，可在不同系统中修改
CMAKE_EXE='cmake'

# 定义颜色代码类，用于在终端中输出带颜色的文本
class ColorsPrint:
    # ANSI颜色代码定义
    RED = '\033[91m'      # 红色 - 用于错误信息
    GREEN = '\033[92m'    # 绿色 - 用于成功信息
    YELLOW = '\033[93m'   # 黄色 - 用于警告信息
    BLUE = '\033[94m'     # 蓝色 - 用于普通信息
    PURPLE = '\033[95m'   # 紫色 - 用于命令显示
    CYAN = '\033[96m'     # 青色 - 用于调试信息
    WHITE = '\033[97m'    # 白色 - 用于普通文本
    BOLD = '\033[1m'      # 粗体文本
    UNDERLINE = '\033[4m' # 下划线文本
    END = '\033[0m'       # 结束颜色/样式 - 重置为默认样式
    
    # 红色打印函数，用于显示错误信息
    def red_print(msg):
        print(ColorsPrint.RED + msg + ColorsPrint.END)
    
    # 绿色打印函数，用于显示成功信息
    def green_print(msg):
        print(ColorsPrint.GREEN + msg + ColorsPrint.END)

    # 紫色打印函数，用于显示执行的命令
    def purple_print(msg):
        print(ColorsPrint.PURPLE + msg + ColorsPrint.END)

# 任务处理主类，包含所有CMake相关操作
class Tasks():
    # CMake配置任务方法，用于生成构建系统文件
    def cmake_config(self, command):
        try:
            # 执行传入的命令
            self.run_command(command)
        except Exception as e:
            # 捕获异常并以红色显示错误信息
            ColorsPrint.red_print(f'cmake config error[{e}]')
    
    # CMake构建任务方法，用于编译项目
    def cmake_build(self, command, buildpath):
        # 调用run_command方法执行构建命令
        self.run_command(command)

    # CMake删除构建目录任务方法，用于清理构建文件
    def cmake_delete(self, build_path):
        # 检查指定的构建路径是否存在
        if os.path.exists(build_path):
            try:
                # 递归删除整个构建目录及其内容
                shutil.rmtree(build_path)
                # 显示删除成功的绿色信息
                ColorsPrint.green_print(f'delete directory [{build_path}] success')
            except Exception as e:
                # 捕获删除过程中的异常并显示错误信息
                ColorsPrint.red_print(f'delete directory [{build_path}] error[{e}]')
        else:
            # 如果目录不存在，显示提示信息
            ColorsPrint.green_print(f'directory [{build_path}] is empty, do not need delete')
        ColorsPrint.purple_print('-------------------------------------------------')

    # 运行命令并返回结果的基础方法
    def run_command(self, command):
        # 以紫色显示即将执行的命令
        ColorsPrint.purple_print(f"[{command}]")
        try:
            # 使用subprocess.run执行命令，shell=True允许执行shell命令
            # 在不指定 stdout 和 stderr 参数时，子进程的输出会直接传递到父进程的标准输出和标准错误流
            # 这意味着命令的输出会在终端中直接显示，与父进程共享标准输出
            return subprocess.run(command, shell=True)
        except Exception as e:
            # 抛出异常供上层处理
            raise
        finally:
            ColorsPrint.purple_print('-------------------------------------------------')

    # 使用Popen运行命令并实时输出的高级方法（当前未被使用）
    def popen_command(self, command):
        # 读取并打印流数据的内部函数
        def read_and_print(stream, is_stderr=False):
            while True:
                # 从流中读取数据，每次读取1024字节
                data = stream.read(1024)
                # 如果没有更多数据则退出循环
                if not data:
                    break
                # 根据流类型将数据写入相应的标准输出流
                if is_stderr:
                    # 写入标准错误流
                    sys.stderr.buffer.write(data)
                    sys.stderr.buffer.flush()
                else:
                    # 写入标准输出流
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
            # 关闭流
            stream.close()
        
        try:
            # 创建子进程，捕获stdout和stderr
            process = subprocess.Popen(command, 
                                       shell=True,           # 通过shell执行命令
                                       stdout=subprocess.PIPE, # 捕获标准输出
                                       stderr=subprocess.PIPE, # 捕获标准错误
                                       bufsize=0)            # 无缓冲模式
            
            # 创建两个线程分别处理stdout和stderr的实时输出
            stdout_thread = threading.Thread(target=read_and_print, args=(process.stdout, False))
            stderr_thread = threading.Thread(target=read_and_print, args=(process.stderr, True))
            
            # 启动两个线程
            stdout_thread.start()
            stderr_thread.start()

            # 等待两个线程执行完毕
            stdout_thread.join()
            stderr_thread.join()
            
            # 等待子进程结束并返回退出码
            return process.wait()
        except Exception as e:
            # 抛出异常供上层处理
            raise

# 程序主入口函数
def main():
    # 创建Tasks类的实例
    tasks = Tasks()

    # 创建命令行参数解析器
    parser = argparse.ArgumentParser(description='CMake tasks helper script')
    # 添加子命令解析器，dest='cmake'表示子命令名称将存储在args.cmake中
    subparsers = parser.add_subparsers(dest='cmake')

    # 添加config子命令及其参数
    cmake_config = subparsers.add_parser('config', help='cmake config - generate build system files')
    # 添加-B/--buildpath参数，用于指定构建目录路径
    cmake_config.add_argument('-B', '--buildpath', 
                              dest='buildpath', 
                              default='build', 
                              help='config cmake build directory')
    # 添加-G/--generator参数，用于指定CMake生成器类型
    cmake_config.add_argument('-G', '--generator', 
                              dest='generator', 
                              default='MinGW Makefiles', 
                              help='config cmake generator')

    # 添加build子命令及其参数
    cmake_build = subparsers.add_parser('build', help='cmake build - compile the project')
    # 添加-B/--buildpath参数，用于指定构建目录路径
    cmake_build.add_argument('-B', '--buildpath', 
                             dest='buildpath', 
                             default='build', 
                             help='cmake build directory')
    # 添加-G/--generator参数（构建时通常不需要，但为了参数一致性保留）
    cmake_build.add_argument('-G', '--generator', 
                             dest='generator', 
                             default='MinGW Makefiles', 
                             help='config cmake generator')

    # 添加delete子命令及其参数
    cmake_delete = subparsers.add_parser('delete', help='delete cmake build directory')
    # 添加-B/--buildpath参数，用于指定要删除的构建目录路径
    cmake_delete.add_argument('-B', '--buildpath', 
                              dest='buildpath', 
                              default='build', 
                              help='cmake build directory')

    # 解析命令行参数
    args = parser.parse_args()
    
    # 使用模式匹配根据子命令执行相应操作
    match args.cmake:
        case 'config':
            # 构造CMake配置命令字符串
            command = f'{CMAKE_EXE} -B{args.buildpath} -G"{args.generator}"'
            # 调用cmake_config方法执行配置
            tasks.cmake_config(command)
        case 'build':
            # 构造CMake构建命令字符串
            command = f'{CMAKE_EXE} --build {args.buildpath}'
            # 调用cmake_build方法执行构建
            tasks.cmake_build(command, args.buildpath)
        case 'delete':
            # 调用cmake_delete方法删除构建目录
            tasks.cmake_delete(args.buildpath)
        case '_':
            # 如果没有匹配的子命令，显示错误信息
            ColorsPrint.red_print('args error!')
            return

# 程序入口点，只有直接运行此脚本时才会执行main()函数
if __name__ == '__main__':
    main()