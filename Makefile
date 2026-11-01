all: server client

GENERAL = general.h general.cpp
FLAGS = --std=c++20 -Wfatal-errors

server: server.cpp $(GENERAL)
	g++ $(FLAGS) -o server server.cpp $(GENERAL)

client: client.cpp $(GENERAL)
	g++ $(FLAGS) -o client client.cpp $(GENERAL)
