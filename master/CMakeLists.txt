cmake_minimum_required (VERSION 3.2)

# put all .cpp and .h files into the sources variable
set(sources
	src/main.cpp
	src/Stopwatch.cpp
	src/Stopwatch.h
)

#include_directories(
#	${CMAKE_PROJECT_NAME}
#	lib/msgpack-c/include/
#	)

add_executable(
	master
	${sources}
	)

#target_link_libraries(
#	${CMAKE_PROJECT_NAME}
#	tcpserver
#	${LUA_LIB}
#	Threads::Threads
#	mysqlcppconn
#)
