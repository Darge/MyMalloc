# MyMalloc

It's a malloc/realloc/calloc/free implementation in Unix. It keeps a list of free blocks and merges them together, so it doesn't allocate or deallocate memory every time you use malloc() or free().

I used LD_PRELOAD to dynamically link popular Linux program with my implementation instead of the standard one. It works with big programs like Google Chrome, Firefox.
It doesn't work with programs that depend on more functions connected with malloc, like mallopt, malloc_get_state etc. Examples of such programs: gedit, LibreOffice, VLC.
