build: main.c
	$(CC) main.c -o puged -Wall -Wextra -pedantic -std=c99
test: main.c puged testFile.txt
	$(CC) main.c -o puged -Wall -Wextra -pedantic -std=c99 && ./puged testFile.txt