CC=g++
CFLAGS=-Wall
LDFLAGS=
BUILD_DIR=build
CLIENT_SRC=client.cpp
SERVER_SRC=main.cpp server.cpp
CLIENT_EXE=client
SERVER_EXE=server
CLIENT_ARGS= 192.168.0.254 
SERVER_ARGS=

all: clean $(SERVER_EXE) $(CLIENT_EXE) 
	
$(CLIENT_EXE): mkbuilddir
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLIENT_SRC) -o $(BUILD_DIR)/$@

$(SERVER_EXE): mkbuilddir
	$(CC) $(CFLAGS) $(LDFLAGS) $(SERVER_SRC) -o $(BUILD_DIR)/$@

mkbuilddir:
	mkdir -p $(BUILD_DIR)

runclient:
	$(BUILD_DIR)/$(CLIENT_EXE) $(CLIENT_ARGS)
	
runserver:
	$(BUILD_DIR)/$(SERVER_EXE) $(SERVER_ARGS)

clean:
	rm -rf build/
