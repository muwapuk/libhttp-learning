cmake_minimum_required(VERSION 3.12)
project(libhttp-learning)

add_subdirectory(libhttp)

add_executable(runclient
	runclient.cpp
)
add_executable(runserver
	runserver.cpp
)
target_link_libraries(runserver
	PRIVATE
		libhttp
)
target_link_libraries(runclient
	PRIVATE
		libhttp
)
