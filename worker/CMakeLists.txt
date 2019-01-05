cmake_minimum_required (VERSION 3.2)

# put all .cpp and .h files into the sources variable
set(sources
	src/main.cpp
)

#include_directories(
#	${CMAKE_PROJECT_NAME}
#	lib/msgpack-c/include/
#	)

add_executable(
	worker
	${sources}
	)

target_link_libraries(
	worker
	rt
)
