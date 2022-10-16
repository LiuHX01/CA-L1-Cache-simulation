CC = g++
OPT = -O3 -m32
#OPT = -g -m32
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc files here (source files, excluding header files)
SIM_SRC = main.cc world.cc

# List corresponding compiled object files here (.o files)
SIM_OBJ = main.o world.o
 
#################################

# default rule

all: sim_cache
	@echo "my work is done here..."


# rule for making sim_cache

sim_cache: $(SIM_OBJ)
	$(CC) -o sim_cache $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH SIM_CACHE-----------"


# generic rule for converting any .cc file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


# type "make clean" to remove all .o files plus the sim_cache binary

clean:
	rm -f *.o sim_cache


# type "make clobber" to remove all .o files (leaves sim_cache binary)

clobber:
	rm -f *.o


test:
	make clean
	make all
	./sim_cache 16 16384 1 0 0 gcc_trace>o1
	diff -iw o1 ../validation/ValidationRun1.txt
	./sim_cache 128 2048 8 0 1 go_trace>o2
	diff -iw o2 ../validation/ValidationRun2.txt
	./sim_cache 32 4096 4 0 1 perl_trace>o3
	diff -iw o3 ../validation/ValidationRun3.txt
	./sim_cache 64 8192 2 1 0 gcc_trace>o4
	diff -iw o4 ../validation/ValidationRun4.txt
	./sim_cache 32 1024 4 1 1 go_trace>o5
	diff -iw o5 ../validation/ValidationRun5.txt
