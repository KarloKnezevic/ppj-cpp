CC = g++
CFLAGS = -O2 -Wall

SRCS = codegen.cpp ast.cpp types.cpp parse.cpp symtable.cpp
HEADERS = ast.h types.h parse.h symtable.h function.h
OBJS = $(SRCS:.cpp=.o)
EXE = codegen

all: $(HEADERS) $(SRCS) $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJS)
