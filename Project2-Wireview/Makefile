SOURCES := wireview.cpp
HEADERS := wireview.h
LINK := -lpcap
OUTPUT := wireview
COMPILER := gcc

all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(CXX) -o $(OUTPUT) $(SOURCES) $(LINK)

clean:
	$(RM) $(OUTPUT)