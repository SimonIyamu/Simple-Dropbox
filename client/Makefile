dropbox_client : main.o client_implementation.o buffer_implementation.o list_implementation.o client_interface.h buffer_interface.h bool.h
	gcc main.o client_implementation.o list_implementation.o buffer_implementation.o -o dropbox_client -lm -pthread

main.o : main.c
	gcc -c main.c -g 

client_implementation.o : client_implementation.c
	gcc -c client_implementation.c -g 

list_implementation.o: list_implementation.c list_interface.h list_types.h
	gcc -c list_implementation.c

buffer_implementation.o: buffer_implementation.c buffer_interface.h buffer_types.h
	gcc -c buffer_implementation.c -g

clean:
	rm dropbox_client main.o client_implementation.o buffer_implementation.o list_implementation.o
