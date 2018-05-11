CC = g++
IPATH = -I./include
LPATH = -L./libs
LIB = -lisl
CFLAGS = $(IPATH) $(LPATH) $(LIB) -g
OUTDIR = ./bin
MKDIR = mkdir -p

SAN = -fsanitize=address -fsanitize=undefined

OBJ = \
	set_fuzzer.o \
	set_tester.o \
	isl_tester.o

all: dirs isl_tester

san: dirs isl_tester_san

dirs:
	${MKDIR} ${OUTDIR}

%.o: %.cpp %.hpp
	$(CC) -c -o $@ $< $(CFLAGS)

isl_tester: $(OBJ)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)

isl_tester_san: $(OBJ)
	$(CC) $(SAN) -o $(OUTDIR)/$@ $^ $(CFLAGS)

clean:
	rm -rf *.o ${OUTDIR}
