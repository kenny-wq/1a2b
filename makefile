all:server client
server: server.cpp ./source/*.cpp ./header/*.h
	g++ -pthread server.cpp ./source/*.cpp -o ./build/server
client: client.cpp
	g++ client.cpp -o client