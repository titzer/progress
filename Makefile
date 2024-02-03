
.PHONY: all
all: bin/progress.host

bin/progress.host: src/progress.c
	cc -o bin/progress.host -Ibin src/progress.c
	bash -x bin/.auto-progress < test/make.in

.PHONY: clean
clean:
	rm -f bin/progress
	rm -f bin/progress.host
	cp bin/.auto-progress bin/progress

