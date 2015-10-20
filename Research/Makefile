all:main.o circuit.o aging.o
	g++ -o research main.o circuit.o aging.o
main.o:main.cpp circuit.h
	g++ -c main.cpp
circuit.o:circuit.h circuit.cpp typedef.h aging.h
	g++ -c circuit.cpp
aging.o:aging.h aging.cpp typedef.h
	g++ -c aging.cpp
clean:
	rm research main.o circuit.o aging.o

