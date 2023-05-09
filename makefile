all:server client stnc

server:server.c
	gcc server.c -o server

client:client.c
	gcc client.c -o client

stnc:stnc.c
	gcc stnc.c -o stnc

clean:
	rm -f client server stnc