CC		  := clang
CCFLAGS   := -Wall -Werror -Wextra -pedantic -std=c11

SOURCES   := ./source/*.c
OBJS	  := ./objects/*.o

EXEC := pged

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) -g3 $(CCFLAGS)

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
