all:
	protoc --proto_path=./ --cpp_out=./ ./simple_message.proto
	g++ -g -I../../output/ -I../../output/include/ -L../../output/lib/ simple_message.pb.cc server.cc -std=c++11 -lprotobuf -pthread -lpink -o server
	g++ -g -I../../output/ -I../../output/include/ simple_message.pb.cc client.cc -std=c++11 -lprotobuf -pthread -o client
server:
	protoc --proto_path=./ --cpp_out=./ ./simple_message.proto
	g++ -g -I../../output/ -I../../output/include/ -L../../output/lib/ simple_message.pb.cc server.cc -std=c++11 -lprotobuf -pthread -lpink -o server
client:	
	protoc --proto_path=./ --cpp_out=./ ./simple_message.proto
	g++ -g -I../../output/ -I../../output/include/ simple_message.pb.cc client.cc -std=c++11 -lprotobuf -pthread -o client
clean:
	rm -f server client simple_message.pb.*
