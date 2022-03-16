
.PHONY: all
all:
	bin/progress l < test/make.txt

.PHONY: clean
clean:
	rm -f bin/progress
	cp bin/.auto-progress bin/progress

