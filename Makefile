CC=g++
CFLAGS=-Wall
LDFLAGS=
BUILD_DIR=build/test
SOURCES=main.cpp
EXECUTABLE=hello

all: $(EXECUTABLE)
	
$(EXECUTABLE): mkbuilddir
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(BUILD_DIR)/$@

mkbuilddir:
	mkdir -p $(BUILD_DIR)

run:
	$(BUILD_DIR)/$(EXECUTABLE) www.google.com