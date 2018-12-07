#设置CMAKE最小版本
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
#设置编译器
SET(CMAKE_CXX_COMPILER "gcc")
#项目名称
project(hero)
# for glibc
include(FindPkgConfig)
pkg_check_modules(GLIB glib-2.0 REQUIRED)
include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
# for tc malloc
find_library(TCMALLOC_LIB NAMES libtcmalloc_minimal.a tcmalloc_minimal)
#代码路径 main路径
aux_source_directory(. source_list)
# 子目录net路径
ADD_SUBDIRECTORY(net)
#debug 模式
set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g ")
#打印详细信息
set(CMAKE_VERBOSE_MAKEFILE ON)
#生成可执行的文件
add_executable(hero ${source_list}) 
#link对应net lib
target_link_libraries(hero net ${GLIB_LIBRARIES} ${TCMALLOC_LIB})