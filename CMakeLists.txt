cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(reactor LANGUAGES C CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "*** CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

message(STATUS "*** CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
#set(CMAKE_MODULE_PATH "/usr/share/cmake-3.18/Modules/")

message(STATUS "*** CMAKE_C_FLAGS_DEBUG: " ${CMAKE_C_FLAGS_DEBUG})
message(STATUS "*** CMAKE_C_FLAGS_RELEASE: " ${CMAKE_C_FLAGS_RELEASE})
message(STATUS "*** CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
message(STATUS "*** CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})

set(CXXFLAGS
  -g
  -Wall
  -Wextra
)
string(REPLACE ";" " " CXXFLAGS "${CXXFLAGS}")
string(APPEND CMAKE_CXX_FLAGS " ")
string(APPEND CMAKE_CXX_FLAGS ${CXXFLAGS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

list(APPEND LINK_LIBS
  libgflags.a
  libglog.a
  pthread
  unwind
)

add_subdirectory(muduo)

##list(APPEND LINK_LIBS )
#
#foreach(lib IN LISTS LIBS)
#  message(STATUS "search for lib:[" ${lib} "]")
#  find_library(thelib
#    ${lib}
#    #PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64 /usr/lib/x86_64-linux-gnu/
#  )
#  message(STATUS "lib found by cmake[" ${thelib} "]")
#  list(APPEND LINK_LIBS ${thelib})
#endforeach()

#foreach(lib IN LISTS LINK_LIBS)
#  message(STATUS "link a lib[" ${lib} "]")
#  find_library
#  link_libraries(${lib})
#endforeach()
#link_libraries(${LINK_LIBS})
link_libraries(libglog.a  libgflags.a pthread unwind)

#find_package(Glog REQUIRED)
#include_directories(BEFORE ${GLOG_INCLUDE_DIRS})

add_subdirectory(io_multiplexing)

install(FILES README.md DESTINATION doc COMPONENT document RENAME "README.md")
