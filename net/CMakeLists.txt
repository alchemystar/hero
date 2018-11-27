# 设置当前代码路径=>DIR_NET_SRC
AUX_SOURCE_DIRECTORY(. DIR_NET_SRCS)
ADD_SUBDIRECTORY(proto)
# 将当前路径的编译结果加入net lib
ADD_LIBRARY (net ${DIR_NET_SRCS})
target_link_libraries(net proto)