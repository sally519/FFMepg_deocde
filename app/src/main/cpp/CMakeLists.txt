cmake_minimum_required(VERSION 3.10.2)

# 创建一个变量 source_file 它的值就是src/main/cpp/ 所有的.cpp文件
file(GLOB source_file *.cpp)

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ${source_file})

include_directories(include)

#把libs里面的库链接到.cpp文件
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -LD:/AndroidStudioProject/FirstFFmpeg/app/libs/armeabi-v7a")

target_link_libraries( # Specifies the target library.
                       native-lib
                       avfilter avformat avcodec avutil swresample swscale
                       # Links the target library to the log library
                       # included in the NDK.
                       log z)