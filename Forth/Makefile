ifdef USENCURSES
MAIN_OBJ = main_test_curses.o
LDFLAGS += -lncurses
else
MAIN_OBJ = main_test_stdio.o
endif

# On a 64 bit Linux installations you need to compile in 32bit mode. Uncomment the line that has -m32 in it and comment out the next one.
# CC = gcc -m32

CC = gcc

CFLAGS += -O3

default: forth

forth: $(MAIN_OBJ) forth.o forth_dict.o forth_file_access_stdio.o forth_memory_malloc.o forth_posix.o
	$(CC) $(CFLAGS) $^ -o forth $(LDFLAGS)

forth_file_access_stdo.o: forth_file_access_stdio.c forth_internal.h forth.h forth_config.h forth_features.h 

forth_memory_malloc.o: forth_memory_malloc.c forth_internal.h forth.h forth_features.h forth_config.h

forth.o: forth.c forth_internal.h forth.h forth_config.h forth_dict.h forth_features.h forth_internal.h 

main_test_curses.o: main_test_curses.c forth.h forth_features.h forth_config.h forth_dict.h

main_test_stdio.o: main_test_stdio.c forth.h forth_features.h forth_config.h forth_dict.h

forth_posix.o:	forth_posix.c forth.h forth_config.h forth_features.h forth_internal.h

forth_dict.o:	forth_dict.c forth_dict.h forth.h forth_features.h forth_config.h

gen_dict: gen_dict.c forth_internal.h forth.h forth_config.h forth_features.h 
	$(CC) $(CFLAGS) gen_dict.c -o gen_dict

forth_dict.c: gen_dict
	./gen_dict

forth_dict.h: gen_dict
	./gen_dict

clean:
	$(RM) -f forth *.o gen_dict.o gen_dict x.x x.txt forth_dict.[ch]



