zad2: zad2.c
	reset && gcc zad2.c -std=gnu99 && ./a.out

library:
	gcc -DFOR_LIBRARY_USAGE zad2.c -o libmyownmalloc.so -fPIC -shared

mrlibrary: reset library run_library

crash_library:
	 gcc -DCRASH_THIS_LIB  -DFOR_LIBRARY_USAGE zad2.c -o libmyownmalloc.so -fPIC -shared

run_library: library
	LD_PRELOAD=$(shell pwd)/libmyownmalloc.so firefox
	
build:
	gcc zad2.c -std=gnu99

reset:
	bash -c reset