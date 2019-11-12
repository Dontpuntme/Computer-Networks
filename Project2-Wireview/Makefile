SOURCES := wireview.c
HEADERS := wireview.h
LINK := -lpcap
OUTPUT := wireview
COMPILER := gcc

all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(COMPILER) -o $(OUTPUT) $(SOURCES) $(LINK)

clean:
	$(RM) $(OUTPUT)