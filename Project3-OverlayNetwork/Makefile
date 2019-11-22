SOURCES := overlay.cpp ipPacket.cpp udpPacket.cpp
HEADERS := overlay.h ipPacket.h udpPacket.h
LINK := -lpcap
OUTPUT := overlay

all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(CXX) -o $(OUTPUT) $(SOURCES) $(LINK)

clean:
	$(RM) $(OUTPUT)