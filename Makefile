CC=gcc
CFLAGS += -g -Os -static -Wall -Wno-maybe-uninitialized -I/usr/local/include/ -Isrc 
LDFLAGS += -lm 

SRC=./src/

OBJS=rv32gen.o 
 
all:rv32gen.exe

rv32gen.exe: $(OBJS)
	$(CC) -o rv32gen $(OBJS) $(LDFLAGS)	

rv32gen.o: $(SRC)rv32gen.c 
	$(CC) -c $(CFLAGS) $(SRC)rv32gen.c
		
clean:
	del rv32gen.exe 
	del *.o
