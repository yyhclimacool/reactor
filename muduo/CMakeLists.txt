aux_source_directory(src muduo_srcs)
#set(muduo_srcs
#    src/timestamp.cc
#)
add_library(muduo STATIC ${muduo_srcs})
