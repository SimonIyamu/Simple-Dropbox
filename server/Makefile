dropbox_server : main.o server_implementation.o list_implementation.o server_interface.h
	gcc main.o server_implementation.o list_implementation.o -o dropbox_server

main.o : main.c
	gcc -c main.c -g 

server_implementation.o : server_implementation.c
	gcc -c server_implementation.c -g -fpic

list_implementation.o: list_implementation.c list_interface.h list_types.h
	gcc -c list_implementation.c

clean:
	rm dropbox_server main.o server_implementation.o list_implementation.o
