CC      := gcc
CPP     := g++


CFLAGS    := -Wall
CFLAGS  += -std=gnu99

# Flags included for c++ compilation
CPPFLAGS  := -Wall -std=c++11

# Flags included for both cpp and c compilation
CXXFLAGS  += -I GB_emu/includes/
CPPFLAGS_TESTS := -D CPPUTEST

ifeq ($(shell uname -s), MINGW64_NT-10.0)
	LDFLAGS := `sdl2-config --cflags --libs`
else
	LDFLAGS := -lSDL2
endif

EXE := GB

SRC_DIR := GB_emu/src/

OBJ_DIR := objs/
SRC_FILES := $(wildcard $(SRC_DIR)*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRC_FILES))

OBJTST_DIR := objs/tests/
TST_FILES := $(wildcard tests/*.c)
TST_OBJ_FILES := $(patsubst tests/%.c, $(OBJTST_DIR)%.o,$(TST_FILES))

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

# Additional compiled test program
$(EXE): $(OBJ_FILES) $(OBJ_DIR)argparse.o  $(OBJ_DIR)utils.o $(OBJ_DIR)iniparse.o
	$(CPP) -o $@ $^ $(LDFLAGS)


$(OBJTST_DIR)%.o: tests/%.c
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# Additional compiled test program
test_target: $(TST_OBJ_FILES) $(OBJ_DIR)Cart.o $(OBJ_DIR)Cpu.o $(OBJ_DIR)Memory_map.o $(OBJ_DIR)PPU.o $(OBJ_DIR)rtc.o $(OBJ_DIR)APU.o $(OBJ_DIR)argparse.o $(OBJ_DIR)utils.o $(OBJ_DIR)iniparse.o
	$(CPP) -o $(OBJ_DIR)$@ $^ -lCppUTest
	$(OBJ_DIR)$@

all: release test

debug: CXXFLAGS += -DDEBUG -g
debug: mk $(EXE)

release: CXXFLAGS += -O1
release: LDFLAGS += -static-libgcc -static-libstdc++
release: mk $(EXE)

test: mktst test_target

mk: 
	mkdir -p $(OBJ_DIR)

mktst: 
	mkdir -p $(OBJTST_DIR)

.PHONY: clean
clean:
	@echo "clean..."
	rm -rf $(OBJ_DIR) $(EXE) $(EXE).exe
