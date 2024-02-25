CC = gcc
CFLAGS = -Wall -g

all: TCP_Sender TCP_Receiver

TCP_Receiver: TCP_Receiver.o
	$(CC) $(CFLAGS) $< -o $@

TCP_Sender: TCP_Sender.o
	$(CC) $(CFLAGS) $< -o $@

TCP_Receiver.o: TCP_Receiver.c sockconst.h
	$(CC) $(CFLAGS) -c $<

TCP_Sender.o: TCP_Sender.c sockconst.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.gch TCP_Sender TCP_Receiver