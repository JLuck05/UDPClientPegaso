#
#Makefile per client UPD 
#
standalone: Standalone clear
#
all: clientAcme clear
#
esempioIni: dictionary.o iniparser.o config_parser.o esempioIni.o
	echo linking config_parse, ecc...
	gcc -Wall -o esempioIni esempioIni.o config_parser.o iniparser.o dictionary.o
	echo done
#
prova: UDPClientNetduino.c config_parser.c iniparser.c dictionary.c
	echo linking config_parse...
	arm-linux-gnueabi-gcc -m32 -Wall -g -o ClientAcme -L/usr/local/lib -lzlog -lpthread UDPClientNetduino.c config_parser.c iniparser.c dictionary.c
#
clientAcme: dictionary.o iniparser.o ClientACME1.o
	echo linking config_parse...
	gcc -m32 -Wall -ggdb -o ClientAcme UDPClientNetduino.o -lzlog -lpthread config_parser.c iniparser.o dictionary.o -L/usr/local/lib
#
pegaso: dictionary.o iniparser.o Pegaso.o
	echo linking config_parse...
	gcc -m32 -Wall -ggdb -o ClientPegaso UDPClientPegaso.o -lzlog -lpthread config_parser.c iniparser.o dictionary.o -L/usr/local/lib
#
Standalone: dictionary.o iniparser.o StandAlone.o
	echo linking library...
	gcc -m32 -Wall -ggdb -o Standalone UDPClientStandalone.o -lzlog -lpthread config_parser.c iniparser.o dictionary.o -L/usr/local/lib
#
StandAlone.o: UDPClientStandalone.c
	gcc -m32 -c UDPClientStandalone.c -I/usr/local/include -ggdb
#
ClientACME1.o: UDPClientNetduino.c
	gcc -m32 -c UDPClientNetduino.c -I/usr/local/include -ggdb

Pegaso.o: UDPClientPegaso.c
	gcc -m32 -c UDPClientPegaso.c -I/usr/local/include -ggdb
#
config_parser.o: config_parser.c
	gcc -m 32 -c config_parser.c -ggdb

iniparser.o: iniparser.c
	gcc -m32 -c iniparser.c -ggdb

dictionary.o: dictionary.c
	gcc -m32 -c dictionary.c -ggdb

clear: 
	rm *.o
