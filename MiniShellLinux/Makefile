build: parser.tab.o parser.yy.o tema1_mini_shell.o
	g++ tema1_mini_shell.o parser.tab.o parser.yy.o -o mini-shell

tema1_mini_shell.o:
	g++ src/tema1_mini_shell.cpp -c -o tema1_mini_shell.o

parser.tab.o: src/parser/parser.tab.c
	gcc src/parser/parser.tab.c -c -o parser.tab.o

parser.yy.o: src/parser/parser.yy.c
	gcc src/parser/parser.yy.c -c -o parser.yy.o

clean:
	rm *.o mini-shell
