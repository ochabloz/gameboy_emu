CC      := gcc
CFLAGS    := -g -Wall
CFLAGS  += -std=gnu99
# CPPUTest is C++Write, so using g++ To compile the test file
CPP     := g++
CPPFLAGS  := -g -Wall -std=c++11

# Flags included for both cpp and c compilation
CXXFLAGS  += -I GB_emu/includes/
CPPFLAGS_TESTS := -D CPPUTEST
ifeq ($(shell uname -s), MINGW64_NT-10.0)
	LDFLAGS := `sdl2-config --cflags --libs`
else
	LDFLAGS := -lSDL2
endif

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
GB_emu: $(OBJ_FILES) $(OBJ_DIR)argparse.o  $(OBJ_DIR)utils.o
	$(CPP) -o $@ $^ $(LDFLAGS)


$(OBJTST_DIR)%.o: tests/%.c
	mkdir -p $(OBJTST_DIR)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# Additional compiled test program
test: $(TST_OBJ_FILES) $(OBJ_DIR)Cart.o $(OBJ_DIR)Cpu.o $(OBJ_DIR)Memory_map.o $(OBJ_DIR)PPU.o $(OBJ_DIR)rtc.o $(OBJ_DIR)APU.o $(OBJ_DIR)argparse.o $(OBJ_DIR)utils.o
	$(CPP) -o $(OBJ_DIR)$@ $^ -lCppUTest
	$(OBJ_DIR)test

all: GB_emu test
	$(OBJ_DIR)$<

debug: CXXFLAGS += -DDEBUG -g
debug: CFLAGS += -DDEBUG -g
debug: GB_emu

.PHONY: clean
clean:
	@echo "clean..."
	rm -rf $(OBJ_DIR)
