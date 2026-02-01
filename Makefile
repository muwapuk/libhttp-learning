all: test test2

test: 
	mkdir -p build && g++ -Wall main.cpp -o build/test
test2:
	echo hello
