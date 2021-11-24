CC = gcc

all:
	$(CC) -g -lopus -lm -o has_opus_fec has_opus_fec.c
	$(CC) -g -lopus -lm -lpcap -o iterate_opus_fec iterate_opus_fec.c

clean:
	rm -rf *.dSYM has_opus_fec iterate_opus_fec
