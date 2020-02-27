build: main.c
	$(CC) main.c -o pged -Wall -Wextra -pedantic -std=c99
test: main.c pged testFile.txt
	$(CC) main.c -o pged -Wall -Wextra -pedantic -std=c99 && ./pged testFile.txt