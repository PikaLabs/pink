all:
	protoc --proto_path=./ --cpp_out=./ ./message.proto
	g++ -g -I../../output/ -I../../output/include/ -L../../output/lib/ message.pb.cc server.cc -std=c++11 -lprotobuf -pthread -lpink -o server
	g++ -g -I../../output/ -I../../output/include/ -L../../output/lib/ message.pb.cc client.cc -std=c++11 -lprotobuf -pthread -lpink -o client
clean:
	rm -f server client message.pb.*
