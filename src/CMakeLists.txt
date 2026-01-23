cmake_minimum_required(VERSION 3.12)
if(POLICY CMP0177)
	cmake_policy(SET CMP0177 NEW)
endif()

# 使用 vcpkg 工具链文件 Warning  工具连设置必须是在project 前面
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 获取父目录
get_filename_component(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)  
get_filename_component(PARENT_DIR "${CURRENT_DIR}" DIRECTORY)  
get_filename_component(PARENT_NAME ${PARENT_DIR} NAME)
message(STATUS "Current Directory: ${CURRENT_DIR}")
message(STATUS "Parent Directory: ${PARENT_DIR}")
message(STATUS "Parent Directory Name: ${PARENT_NAME}")

project(${PARENT_NAME})
message("PROJECT_NAME: ${PROJECT_NAME}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(MSVC)
set(CMAKE_GENERATOR_PLATFORM x64)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

# OpenMP
find_package(OpenMP)
message("OpenMP_FOUND = ${OpenMP_FOUND}")

if(OpenMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# #MFC
# find_package(MFC)
# message("MFC_FOUND = ${MFC_FOUND}")

# if(MFC_FOUND)
# #1: static MFC library, 2: shared MFC library
# set(CMAKE_MFC_FLAG 2)
# endif()

# #Python3
# find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
# message("Python3_FOUND = ${Python3_FOUND}")
# message("Python3_VERSION = ${Python3_VERSION}")
# message("Python3_LIBS = ${Python3_LIBRARIES}")
#==================================================================================#
# fmt
find_package(fmt CONFIG REQUIRED)
message("fmt_FOUND = ${OpenCV_FOUND}")
message("fmt_VERSION = ${OpenCV_VERSION}")
message("fmt_LIBS = ${OpenCV_LIBS}")

# replxx
# find_package(replxx CONFIG REQUIRED)
# message("replxx_FOUND = ${replxx_FOUND}")
# message("replxx_VERSION = ${replxx_VERSION}")
# message("replxx_LIBS = ${replxx_LIBS}")

# indicators
# find_package(indicators CONFIG REQUIRED)
# message("indicators_FOUND = ${indicators_FOUND}")
# message("indicators_VERSION = ${indicators_VERSION}")
# message("indicators_LIBS = ${indicators_LIBS}")

# nlohmann_json
# find_package(nlohmann_json CONFIG REQUIRED)
# message("nlohmann_json_FOUND = ${nlohmann_json_FOUND}")
# message("nlohmann_json_VERSION = ${nlohmann_json_VERSION}")
# message("nlohmann_json_LIBS = ${nlohmann_json_LIBS}")

# OpenCV
find_package(OpenCV CONFIG REQUIRED)
message("OpenCV_FOUND = ${OpenCV_FOUND}")
message("OpenCV_VERSION = ${OpenCV_VERSION}")
message("OpenCV_LIBS = ${OpenCV_LIBS}")

# ffmepg
# find_package(FFMPEG REQUIRED)
# message("FFMPEG_FOUND = ${FFMPEG_FOUND}")
# message("FFMPEG_LIBRARIES = ${FFMPEG_LIBRARIES}")
# message("FFMPEG_LIBRARY_DIRS = ${FFMPEG_LIBRARY_DIRS}")
# message("FFMPEG_INCLUDE_DIRS = ${FFMPEG_INCLUDE_DIRS}")

# 查找FFmpeg可执行文件
find_program(FFMPEG_EXECUTABLE ffmpeg)
if(FFMPEG_EXECUTABLE)
    message(STATUS "找到 FFmpeg: ${FFMPEG_EXECUTABLE}")
    # 你可以将路径传递给C++源代码
    add_definitions(-DFFMPEG_PATH="${FFMPEG_EXECUTABLE}")
else()
    message(WARNING "未找到 ffmpeg 可执行文件。请安装或手动指定路径。")
endif()

# 查找FFplay可执行文件
find_program(FFPLAY_EXECUTABLE ffplay)
if(FFPLAY_EXECUTABLE)
    message(STATUS "找到 FFPlay: ${FFPLAY_EXECUTABLE}")
    # 你可以将路径传递给C++源代码
    add_definitions(-DFFPLAY_PATH="${FFPLAY_EXECUTABLE}")
else()
    message(WARNING "未找到 ffplay 可执行文件。请安装或手动指定路径。")
endif()

find_program(FFPROBE_EXECUTABLE ffprobe)
if(FFPROBE_EXECUTABLE)
    message(STATUS "找到 FFprobe: ${FFPROBE_EXECUTABLE}")
    # 你可以将路径传递给C++源代码
    add_definitions(-DFFPROBE_PATH="${FFPROBE_EXECUTABLE}")
else()
    message(WARNING "未找到 ffprobe 可执行文件。请安装或手动指定路径。")
endif()

#==================================================================================#

# 外部依赖项
set(DPS_INCLUDES)
set(DPS_TARGETS)
set(DPS_LIBRARYS)

# 资源文件

# #图标
set(RC_FILE)

# #qrc 文件
set(QRC_SOURCE_FILES)

# 忽略特定库
if(MSVC)
SET(CMAKE_EXE_LINKER_FLAGS /NODEFAULTLIB:"libcmt.lib;libc.lib;msvcrt.lib")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CommonTools)

add_subdirectory(third_party/json)
set_target_properties(nlohmann_json PROPERTIES FOLDER "third_party")

add_subdirectory(third_party/replxx)
set_target_properties(replxx PROPERTIES FOLDER "third_party")

add_subdirectory(third_party/indicators)
set_target_properties(indicators PROPERTIES FOLDER "third_party")

add_subdirectory(MemoryLeakDetector)
add_subdirectory(GuessWord)
add_subdirectory(Card21Game)
add_subdirectory(TicTacToe)
add_subdirectory(Timer)
add_subdirectory(MoveTest)
add_subdirectory(LogicalOpt)
add_subdirectory(EnumTest)
add_subdirectory(Base16test)
add_subdirectory(Base16API)
add_subdirectory(CoutTest) # 控制台的输入输出
add_subdirectory(CinTest)
add_subdirectory(StringStreamTest) # 字符串的输入输出
add_subdirectory(FileStreamOutTest) # 文件的输出
add_subdirectory(FileStreamInTest) # 文件的输入
add_subdirectory(FileLogTest) # 日志测试
add_subdirectory(ProgrammingParadigmTest) # 编程范式
add_subdirectory(LogTest) # 日志示例
add_subdirectory(StringTest) 
add_subdirectory(DataPtr) 
add_subdirectory(UsingFunction) #函数指针
add_subdirectory(MsgServer) 
add_subdirectory(XExecTest) 
add_subdirectory(XUserInput) 
add_subdirectory(XVideoEdit) 

# 拷贝assert目录到输出目录
set(ASSERT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/assert)
file(COPY ${ASSERT_PATH} DESTINATION ${OUT_RUN_PATH})