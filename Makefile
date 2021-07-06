all: hello exe lib.so

hello: hello.c
	gcc hello.c -o hello
exe: main.cpp
	g++ main.cpp -g -lpthread -O3 -o exe
lib.so: lib.c
	gcc -fPIC -shared lib.c -o lib.so
test: lib.so hello exe
	./exe ./hello 0 1 2 3
clean:
	rm -f lib.so hello exe
