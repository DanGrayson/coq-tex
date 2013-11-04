CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -g

COQ = coqtop
COQARGS = -emacs

all: coq-tex-merge coq-tex-process
check: test-coq.tex
	tex test-coq.tex
clean:
	rm -f coq-tex-merge coq-tex-process *.dvi *.log *.tmp *.vo *.old *.glob *-coq.tex *.coq-output *-tmp.v

.PRECIOUS : %-coq.tex %.split %.coq-output %-tmp.v
%-coq.tex : %.tex %.coq-output coq-tex-merge
	./coq-tex-merge $*.tex $*.coq-output >$@.tmp
	mv $@.tmp $@
%.coq-output : %-tmp.v
	@if [ -f $*.coq-output ] && cmp -s $*-tmp.v $*-tmp.old ; \
	 then touch $*.coq-output ; \
	 else echo "running COQ on $*-tmp.v"; \
	   ( set -x ; $(COQ) $(COQARGS) <$*-tmp.v >&$*.coq-output.tmp ) && \
	   mv $*.coq-output.tmp $*.coq-output && \
	   cp $*-tmp.v $*-tmp.old ; \
	 fi
%-tmp.v : %.tex coq-tex-process
	./coq-tex-process $< >$@.tmp
	mv $@.tmp $@

bak:
	(cd .. && scp -r coq-tex u00:tmp)
