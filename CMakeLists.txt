#设置CMAKE最小版本
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
#设置编译器
SET(CMAKE_CXX_COMPILER "gcc")
#项目名称
project(hero)
#代码路径 main路径
aux_source_directory(. source_list)
# 子目录net路径
ADD_SUBDIRECTORY(net)
#debug 模式
set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g ")
#生成可执行的文件
add_executable(hero ${source_list}) 
#link对应net lib
target_link_libraries(hero net)