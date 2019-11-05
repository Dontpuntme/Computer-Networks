SOURCES := QRServer.cpp Client.cpp
HEADERS := QRServer.h Client.h
LINK := -pthread
OUTPUT := QRServer Client


SERVER_SOURCES := QRServer.cpp
SERVER_HEADERS := QRServer.h
SERVER_OUT := QRServer

CLIENT_SOURCES := Client.cpp
CLIENT_HEADERS := Client.h
CLIENT_OUT := Client

all: $(OUTPUT)

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(CXX) -o $(SERVER_OUT) $(SERVER_SOURCES) $(LINK)
	$(CXX) -o $(CLIENT_OUT) $(CLIENT_SOURCES)

clean:
	$(RM) $(OUTPUT)