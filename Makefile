SOURCES := QRServer.cpp server.cpp client.cpp
HEADERS := server.h client.h

OUTPUT := QRServer
all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(CXX) -o $(OUTPUT) $(SOURCES)

clean:
	$(RM) $(OUTPUT)