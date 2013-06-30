L = lexer/analizator/analizator
S = syntax/analizator/analizator

DIRS = lexer lexer/analizator syntax syntax/analizator

all: $(DIRS) $(L)

$(L):
	$(MAKE) -C lexer
	$(MAKE) -C lexer/analizator

$(S):
	$(MAKE) -C syntax
	$(MAKE) -C syntax/analizator

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
