SOURCES := wireview.c
HEADERS := wireview.h
OUTPUT := wireview
COMPILER := gcc

all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(COMPILER) -o $(OUTPUT) $(SOURCES)

clean:
	$(RM) $(OUTPUT)