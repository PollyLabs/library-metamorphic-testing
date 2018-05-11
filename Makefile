CC = g++
IPATH = -I./include
LPATH = -L./libs
LIB = -lisl
CFLAGS = $(IPATH) $(LPATH) $(LIB) -g
OUTDIR = ./bin
MKDIR = mkdir -p

OBJ = \
	set_fuzzer.o \
	isl_tester.o

all: dirs isl_tester

dirs:
	${MKDIR} ${OUTDIR}

%.o: %.cpp %.hpp
	$(CC) -c -o $@ $< $(CFLAGS)

isl_tester: $(OBJ)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)

clean:
	rm -rf *.o ${OUTDIR}
