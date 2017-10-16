
# CPPUTest is C++Write, so using g++ To compile the test file
CPP     := g++
CPPFLAGS  := -g -Wall -std=c++11

# Flags included for both cpp and c compilation
CXXFLAGS  += -I GB_emu/includes/
LDFLAGS := -lSDL2

SRC_DIR := GB_emu/src/

OBJ_DIR := objs/
SRC_FILES := $(wildcard $(SRC_DIR)*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRC_FILES))


$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	mkdir -p $(OBJ_DIR)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


# Additional compiled test program
GB_emu: $(OBJ_FILES)
	$(CPP) -o $(OBJ_DIR)$@ $^ $(LDFLAGS)


objs/tests/%.o: tests/%.c
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


all: GB_emu
	$(OBJ_DIR)$<

.PHONY: clean
clean:
	@echo "clean..."
	rm -rf $(OBJ_DIR)
