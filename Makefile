# SandroFurieri (c) 2009
# Makefile - SpatiaLite amalgamation

SRC = amalgamate.c
OBJ = amalgamate.o
EXE = ./amalgamate

# Define default flags:
CFLAGS = -Wall

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(OBJ) -o $(EXE)
	$(EXE)
	sh ./amalgamation/auto-sh

clean :
	$(RM) $(OBJ) $(EXE)
	
amalgamate.o: amalgamate.c
	$(CC) -c amalgamate.c

