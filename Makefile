
.PHONY: all
all: bin/progress.host

bin/progress.host: src/progress.c
	echo "##+build bin/progress.host"
	cc -o bin/progress.host -Ibin src/progress.c
	echo "##-ok"

.PHONY: clean test
clean:
	rm -f bin/progress
	rm -f bin/progress.host
	cp bin/.auto-progress bin/progress

current_dir = $(shell pwd)
export PROGRESS := ${current_dir}/bin/progress.host

test: bin/progress.host
	bash -x bin/.auto-progress s < test/make.in
	echo ${PROGRESS}
	bash test/run.bash
