SLASH_PATH=../../third/slash
PINK_PATH=../../../

all:
	protoc --proto_path=./ --cpp_out=./ ./message.proto
	g++ -O2 -I$(PINK_PATH) -I$(SLASH_PATH) -L$(SLASH_PATH)/slash/lib -L$(PINK_PATH)/pink/lib message.pb.cc server.cc -std=c++11 -lprotobuf -pthread -lpink -lslash -o server
	g++ -O2 -I$(PINK_PATH) -I$(SLASH_PATH) -L$(SLASH_PATH)/slash/lib -L$(PINK_PATH)/pink/lib message.pb.cc client.cc -std=c++11 -lprotobuf -pthread -lpink -lslash -o client
clean:
	rm -f server client message.pb.*
