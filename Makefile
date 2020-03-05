CCFLAGS   := -Wall -Wextra -pedantic -std=c99

SOURCES   := ./source/*.c
OBJS	  := ./objects/*.o

EXEC := pged

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(CCFLAGS)

test: $(OBJS) $(exec) testFile.txt
	$(CC) $(OBJS) -o $(EXEC) $(CCFLAGS) && ./$(EXEC) testFile.txt

clean:
	rm -f $(EXEC) $(OBJS)

install:
	cp $(EXEC) /usr/local/bin/$(EXEC)
uninstall:
	rm -f $(EXEC) /usr/local/bin/$(EXEC)

$(OBJS): $(SOURCES)
	$(CC) $(CCFLAGS) -c $(SOURCES)
	mv *.o ./objects
