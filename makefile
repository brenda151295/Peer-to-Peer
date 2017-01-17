make all:
	g++ --std=c++11 -pthread client.cpp -o client
	g++ --std=c++11 -pthread server.cpp -o server
clean:
	rm client server