WRITEUP_NAME=writeup

all: writeup

writeup:
	latex $(WRITEUP_NAME).tex
	latex $(WRITEUP_NAME).tex
	dvips $(WRITEUP_NAME).dvi
	ps2pdf $(WRITEUP_NAME).ps

clean:
	rm -f *.out *.log *.dvi *.aux *.ps *.pdf

