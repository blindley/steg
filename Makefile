
FLAGS=/nologo /EHsc /std:c++20
SOURCES=main.cpp args.cpp
HEADERS=args.h
EXE_OUT=steg.exe

$(EXE_OUT): Makefile $(SOURCES) $(HEADERS)
	cl $(FLAGS) /Fe:$(EXE_OUT) $(SOURCES)

clean:
	del *.exe
	del *.obj
