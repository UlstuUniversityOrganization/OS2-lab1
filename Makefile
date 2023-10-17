run: build
	./shell

build:
	gcc -g -O2 -Wall *.c *.h -o shell
