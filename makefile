gcc -Wall -g -c main.c xmlparser.c element.c helper.c
gcc -Wall -g main.o xmlparser.o element.o helper.o -o xmlparser.exe