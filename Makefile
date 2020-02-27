build: main.c
	$(CC) main.c -o main -Wall -Wextra -pedantic -std=c99
test: main.c main testFile.txt
	$(CC) main.c -o main -Wall -Wextra -pedantic -std=c99 && ./main testFile.txt