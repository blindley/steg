
FLAGS=/nologo /EHsc /std:c++20
DEBUG_FLAGS=/Zi /MDd /DDEBUG
SOURCES=src/main.cpp src/args.cpp src/image.cpp src/stb/stb.cpp
HEADERS=src/args.h src/image.h src/utility.h
EXE_OUT=steg.exe
DEBUG_EXE=steg_d.exe

all: release debug

release: $(EXE_OUT)

debug: $(DEBUG_EXE)

$(EXE_OUT): Makefile $(SOURCES) $(HEADERS)
	cl $(FLAGS) /Fe:$(EXE_OUT) $(SOURCES)

$(DEBUG_EXE): Makefile $(SOURCES) $(HEADERS)
	cl $(FLAGS) $(DEBUG_FLAGS) /Fe:$(DEBUG_EXE) $(SOURCES)

clean:
	del *.exe
	del *.obj
	del *.ilk
	del *.pdb
