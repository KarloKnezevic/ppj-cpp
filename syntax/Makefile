CC=g++
CFLAGS=-O2 -Wall

SRCS=generator.cpp
OBJS=$(SRCS:.cpp=.o)
GEN=generator
SAN=ppjC.san
OUT=analizator/gen.h

all: $(OUT)

$(OUT): $(GEN) $(SAN)
	./$(GEN) < $(SAN)

$(GEN): $(SRCS) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(GEN) $(OBJS) $(OUT)

