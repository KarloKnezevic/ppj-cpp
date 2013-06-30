CC=g++
CFLAGS=-O2 -Wall

DEPS=gen.h
SRCS=analizator.cpp
OBJS=$(SRCS:.cpp=.o)
EXE=analizator

all: $(SRCS) $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJS)
