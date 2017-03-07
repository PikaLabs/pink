all:
	protoc --proto_path=./ --cpp_out=./ ./message.proto
	g++ -g -I../../output/ -I../../third/ -L../../third/slash/output/lib/ -L../../output/lib/ message.pb.cc server.cc -std=c++11 -lprotobuf -pthread -lpink -lslash -o server
	g++ -g -I../../output/ -I../../third/ -L../../third/slash/output/lib/ -L../../output/lib/ message.pb.cc client.cc -std=c++11 -lprotobuf -pthread -lpink -lslash -o client
clean:
	rm -f server client message.pb.*
