# France Foot 2D — make RAYLIB=chemin/vers/raylib
RAYLIB ?= C:/raylib/raylib
CXX    ?= g++
WINDRES ?= windres
CXXFLAGS = -std=c++17 -O2 -Wall -I$(RAYLIB)/src
SRC  = $(wildcard src/*.cpp)
OBJ  = $(SRC:src/%.cpp=build/%.o)

ifeq ($(OS),Windows_NT)
  TARGET = foot2d.exe
  RES    = build/foot.res.o
  LDLIBS = -L$(RAYLIB)/src -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows -static
else
  TARGET = foot2d
  RES    =
  LDLIBS = -L$(RAYLIB)/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

all: $(TARGET)

$(TARGET): $(OBJ) $(RES)
	$(CXX) $^ -o $@ $(LDLIBS)

build/%.o: src/%.cpp $(wildcard src/*.h src/*.inc) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/foot.res.o: res/foot.rc res/foot.ico | build
	$(WINDRES) -I res $< -O coff -o $@

build:
	mkdir build

clean:
	rm -rf build $(TARGET)

.PHONY: all clean
