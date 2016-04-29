CFLAGS = -Wall
LDFLAGS = -lpthread

all: cme

cme: cme.c
	gcc -g -o cme cme.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.out *.log *.dvi *.aux *.ps *.pdf *.o cme

