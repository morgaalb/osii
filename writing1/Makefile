WRITEUP_NAME=writing1

all: writeup

writeup:
	latex $(WRITEUP_NAME).tex
	bibtex $(WRITEUP_NAME)
	latex $(WRITEUP_NAME).tex
	dvips $(WRITEUP_NAME).dvi
	ps2pdf $(WRITEUP_NAME).ps
	cp -f $(WRITEUP_NAME).pdf ~/public_html
	chmod a+r ~/public_html/$(WRITEUP_NAME).pdf

clean:
	rm -f *.out *.log *.dvi *.aux *.ps *.pdf *.o *.blg *.bbl

