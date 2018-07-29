#This is the makefile for the client application

myftp: myftp.o stream.o tokenise.o
	gcc -Wall myftp.o stream.o tokenise.o -o myftp
myftp.o: myftp.c
	gcc -c -Wall myftp.c -o myftp.o
stream.o: stream.c stream.h
	gcc -c -Wall stream.c -o stream.o
tokenise.o: tokenise.c tokenise.h
	gcc -c -Wall tokenise.c -o tokenise.o
clean:
	rm -f *.o myftp test
