# MyMalloc

It's a malloc/realloc/calloc/free implementation in Unix. It keeps a list of free blocks and merges them together, so it doesn't allocate or deallocate memory every time you use malloc() or free().

I used LD_PRELOAD to dynamically link popular Linux program with my implementation instead of the standard one. It worked flawlessly with single threaded programs like ls, vim, top. In future, I plan on using mutexes to extend it to multithreaded programs and then benchmark it.
