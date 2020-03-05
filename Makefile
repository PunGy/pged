CCFLAGS  := -Wall -Wextra -pedantic -std=c99

MAINS    := main.c
TARGETS  := pged

$(TARGETS): $(MAINS)
	$(CC) $(MAINS) -o $(TARGETS) $(CCFLAGS)

test: $(MAINS) $(TARGETS) testFile.txt
	$(CC) $(MAINS) -o $(TARGETS) $(CCFLAGS) && ./$(TARGETS) testFile.txt

clean:
	rm -f $(TARGETS)

install: $(TARGETS)
	cp $(TARGETS) /usr/local/bin/$(TARGETS)
uninstall:
	rm -f $(TARGETS) /usr/local/bin/$(TARGETS)