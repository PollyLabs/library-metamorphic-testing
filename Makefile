CC = g++
IPATH = -I./include
LPATH = -L./libs
LIB = -lisl
CFLAGS = $(IPATH) $(LPATH) $(LIB) -g
OBJ = set_fuzzer.o
OUTDIR = ./bin
MKDIR = mkdir -p

all: dirs set_fuzzer

dirs:
	${MKDIR} ${OUTDIR}

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

set_fuzzer: $(OBJ)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)

clean:
	rm -rf *.o ${OUTDIR}
