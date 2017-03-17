all:
	protoc --proto_path=./ --cpp_out=./ ./message.proto
	g++ -O2 -I../../ -I../../third/slash/ -L../../third/slash/slash/lib/ -L../../lib/ message.pb.cc server.cc -std=c++11 -lprotobuf -pthread -lpink -lslash -o server
	g++ -O2 -I../../ -I../../third/slash/ -L../../third/slash/slash/lib/ -L../../lib/ message.pb.cc client.cc -std=c++11 -lprotobuf -pthread -lpink -lslash -o client
clean:
	rm -f server client message.pb.*
