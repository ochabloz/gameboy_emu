
CC= g++
CPPFLAGS=-std=c++11 -lSDL2

all: emulator

emulator: APU.o Cpu.o Cart.o main.o Memory_map.o PPU.o
	$(CC) -o emulator $^ $(CPPFLAGS)

main.o: main.cpp
	$(CC) -o $@ -c $< $(CPPFLAGS)

Cpu.o: Cpu.cpp
	$(CC) -o $@ -c $< $(CPPFLAGS)

APU.o: APU.cpp
	$(CC) -o $@ -c $< $(CPPFLAGS)

Cart.o: Cart.cpp
	$(CC) -o $@ -c $< $(CPPFLAGS)

Memory_map.o: Memory_map.cpp
	$(CC) -o $@ -c $< $(CPPFLAGS)

PPU.o: PPU.cpp
	$(CC) -o $@ -c $< $(CPPFLAGS)

clean:
	rm -rf *.o
