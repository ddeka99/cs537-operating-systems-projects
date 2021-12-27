CC   = gcc
OPTS = -Wall -g

#all: libmfs server
all: libmfs server

# this generates the target executables
server: server.o udp.o
	$(CC) -o server server.o udp.o 

libmfs: udp.o mfs.o
	$(CC) -shared -g -Wall -o libmfs.so udp.o mfs.o
udp.o:
	$(CC) $(OPTS) -c -fpic udp.c -o udp.o
mfs.o:
	$(CC) $(OPTS) -c -fpic mfs.c -o mfs.o

# this is a generic rule for .o files 
%.o: %.c 
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f server.o udp.o server libmfs.so disk mfs.o