CC = gcc
CFLAGS=-g 
LIBS=-lpthread 
FLIBS=-lfl

HFILES = 

CCFILES = myshell.c\
	  lex.yy.c

OFILES = $(CCFILES:.c=.o)

all: 

.cc.o:	 $(CCFILES) $(HFILES)
	$(CC) $(CFLAGS) -c $*.cc

link:	$(OFILES)
	$(CC) $(CFLAGS) $(LIBS)  -o run $(OFILES) $(FLIBS)
clean:
	rm -f *.o *.*~ run
