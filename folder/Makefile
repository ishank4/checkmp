# ***************************************** Make file code *********************
# Author: Sanket Agarwal & Dhiraj Kudva (agarwal.220196, dhirajkudva)@tamu.edu
#Organisation: Texas A&M University
#Description: Machine problem 1, Compiles server and client source code.



#for compiling server.c
sample : echos.o echo.o


echos.o: server.c client_server.h 
	gcc -I. server.c -o echos


#for compiling client.c

echo.o: client.c client_server.h
	gcc -I. client.c -o echo

#clean to discard previous .o files
clean:
	rm -f sample *.o core



