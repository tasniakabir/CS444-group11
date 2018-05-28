LATEXINSTALL=/usr/local/apps/tex_live/current/bin/x86_64-linux/

LATEX	= $(LATEXINSTALL)latex -shell-escape
DVIPS	= $(LATEXINSTALL)dvips
DVIPDF  = $(LATEXINSTALL)dvipdft
XDVI	= $(LATEXINSTALL)xdvi -gamma 4
GH		= $(LATEXINSTALL)gv

EXAMPLES = $(wildcard *.c)
SRC	:= $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG	= $(SRC:%.tex=%.dvi)
PSF	= $(SRC:%.tex=%.ps)
PDF	= $(SRC:%.tex=%.pdf)

default: all

pdf: $(PDF)

ps: $(PSF)

$(TRG): %.dvi: %.tex $(EXAMPLES)
	#one way of including source code is to use pygments
	#pygmentize -f latex -o __${EXAMPLES}.tex ${EXAMPLES}
	#requires that you \include{pygments.tex} in your preamble

	$(LATEX) $<
	$(LATEX) $<
	$(LATEX) $<
	#remove the pygmentized output to avoid cluttering up the directory
	#rm __${SRC}.tex


$(PSF):%.ps: %.dvi
	$(DVIPS) -R -Poutline -t letter $< -o $@

$(PDF): %.pdf: %.ps
	ps2pdf $<

show: $(TRG)
	@for i in $(TRG) ; do $(XDVI) $$i & done

showps: $(PSF)
	@for i in $(PSF) ; do $(GH) $$i & done

all: pdf


clean:
	rm -f *.pdf *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg *.fls *.fdb_latexmk *.syentex.gz *.toc

.PHONY: all show clean ps pdf showps
