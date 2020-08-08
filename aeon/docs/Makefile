DOC = mace_HOWTO

PP = FirstPing.mac Ping.mac firstping.cc ping.cc PingServiceClass.mh PingDataHandler.mh

SRCS = $(wildcard *.tex) $(wildcard *.eps)

%all: clean pp $(DOC).ps $(DOC).pdf
all: clean pp $(DOC).pdf

%.pdf: %.tex
	pdflatex $<
	pdflatex $<
	pdflatex $<

%.ps: %.dvi
	dvips $< -Ppdf -o $@

$(DOC).dvi: pp $(SRCS)
	latex $(DOC).tex
	latex $(DOC).tex

pp: $(PP)
	for i in $^; do lgrind -d lgrindef-mace -lmace -i $$i > $$i.tex; done
	./grindtex.pl $(DOC).tex

cleantmp:
	rm -f *.aux *.toc *.out *.log $(DOC).dvi

cleanlg:
	for i in $(PP); do rm -f $$i.tex; done
	rm -f *-lg.tex

clean: cleantmp cleanlg
	rm -f $(DOC).ps $(DOC).pdf $(DOC).xml $(DOC).html

ps: $(DOC).ps

view:
	acroread $(DOC).pdf &

xml: $(SRCS)
	./latextodocbook $(DOC).tex > $(DOC).xml

html: xml
	docbook2html -d ldp.dsl#html -u $(DOC).xml
