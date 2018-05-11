CC = g++
IPATH = -I./include
LPATH = -L./libs
LIB = -lisl
CFLAGS = $(IPATH) $(LPATH) $(LIB) -g
OBJ = set_fuzzer.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

set_fuzzer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -r *.o
