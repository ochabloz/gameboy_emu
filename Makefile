CC      := gcc
CFLAGS    := -g -Wall
CFLAGS  += -std=gnu99
# CPPUTest is C++Write, so using g++ To compile the test file
CPP     := g++
CPPFLAGS  := -g -Wall -std=c++11

# Flags included for both cpp and c compilation
CXXFLAGS  += -I GB_emu/includes/
CPPFLAGS_TESTS := -D CPPUTEST
LDFLAGS := -lSDL2

SRC_DIR := GB_emu/src/

OBJ_DIR := objs/
SRC_FILES := $(wildcard $(SRC_DIR)*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRC_FILES))

OBJTST_DIR := objs/tests/
TST_FILES := $(wildcard tests/*.c)
TST_OBJ_FILES := $(patsubst tests/%.c, $(OBJTST_DIR)%.o,$(TST_FILES))

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	mkdir -p $(OBJ_DIR)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)%.o: $(SRC_DIR)%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

# Additional compiled test program
GB_emu: $(OBJ_FILES) objs/argparse.o  objs/utils.o
	$(CPP) -o $(OBJ_DIR)$@ $^ $(LDFLAGS)


$(OBJTST_DIR)%.o: tests/%.c
	mkdir -p $(OBJTST_DIR)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# Additional compiled test program
test: $(TST_OBJ_FILES) objs/Cart.o objs/Cpu.o objs/Memory_map.o objs/PPU.o objs/rtc.o objs/APU.o objs/argparse.o objs/utils.o
	$(CPP) -o $(OBJ_DIR)$@ $^ -lCppUTest
	objs/test

all: GB_emu test
	$(OBJ_DIR)$<

.PHONY: clean
clean:
	@echo "clean..."
	rm -rf $(OBJ_DIR)
