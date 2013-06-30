L = lexer/analizator/analizator
S = syntax/analizator/analizator
C = codegen/codegen

DIRS = lexer lexer/analizator syntax syntax/analizator codegen

all: $(DIRS) $(L) $(S) $(C)

$(L):
	$(MAKE) -C lexer
	$(MAKE) -C lexer/analizator

$(S):
	$(MAKE) -C syntax
	$(MAKE) -C syntax/analizator

$(C):
	$(MAKE) -C codegen

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done

