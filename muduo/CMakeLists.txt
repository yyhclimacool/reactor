aux_source_directory(src muduo_srcs)
#set(muduo_srcs
#    src/timestamp.cc
#)
add_library(muduo STATIC ${muduo_srcs})
set(HEADERS
    src/channel.h
    src/current_thread.h
    src/epoll_poller.h
    src/event_loop.h
    src/poller.h
    src/timestamp.h
    )
install(FILES ${HEADERS} DESTINATION include/)
install(TARGETS muduo DESTINATION lib/)
