
.PHONY: all
all: bin/progress.host

bin/progress.host: src/progress.c src/progress.h
	cc -o bin/progress.host -Ibin src/progress.c
	bash -x bin/.auto-progress < test/make.txt

.PHONY: clean
clean:
	rm -f bin/progress
	rm -f bin/progress.host
	cp bin/.auto-progress bin/progress

