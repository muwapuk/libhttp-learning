add_library(libhttp STATIC 
	include/client.h src/client.cpp
	include/server.h src/server.cpp
	include/networkfuncs.h src/networkfuncs.cpp
	include/http.h src/http.cpp	
    include/logger.h src/logger.cpp
)


target_include_directories(libhttp 
	PUBLIC
		include
)

