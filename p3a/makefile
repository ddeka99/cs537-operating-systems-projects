# specify all source files here
SRCS = pzip.c
# specify target here (name of executable)
TARG = pzip
# specify compiler, compile flags, and needed libs
CC = gcc
OPTS = -Wall -O3
LIBS = -lm -lpthread -lrt
# this translates .c files in src list to .o’s
OBJS = $(SRCS:.c=.o)
# all is not really needed, but is used to generate the target
all: $(TARG)
# this generates the target executable
$(TARG): $(OBJS)
	$(CC) -o $(TARG) $(OBJS) $(LIBS)
# this is a generic rule for .o files
%.o: %.c
	$(CC) $(OPTS) -c $< -o $@
# and finally, a clean line
clean:
	rm -f $(OBJS) $(TARG)