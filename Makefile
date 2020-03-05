CCFLAGS  := -Wall -Wextra -pedantic -std=c99

MAINS    := ./source/*.c
OBJS	 := ./objects/*.o

pged: $(OBJS)
	$(CC) $(OBJS) -o pged $(CCFLAGS)

test: $(OBJS) pged testFile.txt
	$(CC) $(OBJS) -o pged $(CCFLAGS) && ./pged testFile.txt

clean:
	rm -f pged $(OBJS)

install:
	cp pged /usr/local/bin/pged
uninstall:
	rm -f pged /usr/local/bin/pged

$(OBJS): $(MAINS)
	$(CC) $(CCFLAGS) -c $(MAINS)
	mv *.o ./objects
