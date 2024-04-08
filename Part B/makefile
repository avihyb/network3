CC=gcc
CFLAGS=-Wall -Wextra -std=c99


all: RUDP_Sender RUDP_Receiver

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o RUDP_Sender RUDP_Sender.o RUDP_API.o

RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o RUDP_Receiver RUDP_Receiver.o RUDP_API.o

RUDP_Sender.o: RUDP_Sender.c
	$(CC) $(CFLAGS) -c RUDP_Sender.c

RUDP_Receiver.o: RUDP_Receiver.c
	$(CC) $(CFLAGS) -c RUDP_Receiver.c

RUDP_API.o: RUDP_API.c
	$(CC) $(CFLAGS) -c RUDP_API.c

clean:
	rm -f *.txt *.o RUDP_Sender RUDP_Receiver
