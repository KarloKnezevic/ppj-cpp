CC=g++
CFLAGS=-O2 -Wall

SRCS=generator.cpp regex.cpp analizator/automaton.cpp
OBJS=$(SRCS:.cpp=.o)
GEN=generator

LAN=ppjC.lan
OUT=analizator/language.cpp

all: $(OUT)

$(OUT): $(GEN) $(LAN)
	./$(GEN) < $(LAN)

$(GEN): $(SRCS) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(GEN) $(OBJS) $(OUT)

