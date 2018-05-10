CC = g++
IPATH = -I./include
LPATH = -L./libs
LIB = -lisl
CFLAGS = $(IPATH) $(LPATH) $(LIB) -g
OBJ = small.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

small: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -r *.o
