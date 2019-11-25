SOURCES := overlay.cpp
HEADERS := overlay.h
LINK := -lpcap
OUTPUT := overlay

all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(CXX) -o $(OUTPUT) $(SOURCES) $(LINK)

clean:
	$(RM) $(OUTPUT)