CC=g++
CFLAGS=-O2 -Wall

SRCS=analizator.cpp automaton.cpp language.cpp
OBJS=$(SRCS:.cpp=.o)
EXE=analizator

all: $(SRCS) $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJS)
