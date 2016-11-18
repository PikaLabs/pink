#ifndef _TEMPLETE_H
#define _TEMPLETE_H

#include <string>

namespace pink {

/*
 * service implement header file template
 */

#define INCLUDES_MARK                           "[INCLUDES]"
#define SERVICE_IMPL_DECLARES_MARK              "[SERVICE_IMPL_DECLARES]" 
#define PROTO_NAME_CAPITAL_MARK                 "[PROTO_NAME_CAPITAL]"
	
#define SERVICE_NAME_MARK                       "[SERVICE_NAME]"
#define SERVICE_FULL_NAME_MARK                  "[SERVICE_FULL_NAME]"
#define SERVICE_IMPL_METHOD_DECLARES_MARK       "[SERVICE_IMPL_METHOD_DECLARES]"

#define METHOD_NAME_MARK                        "[METHOD_NAME]"
#define REQUEST_TYPE_NAME_MARK                  "[REQUEST_TYPE_NAME]"
#define RESPONSE_TYPE_NAME_MARK                 "[RESPONSE_TYPE_NAME]"

#define SERVICE_IMPL_METHOD_IMPLEMENTS_MARK     "[SERVICE_IMPL_METHOD_IMPLEMENTS]"

#define PROTO_NAME_MARK                         "[PROTO_NAME]"
#define SERVER_REGISTER_SERVICES_MARK           "[SERVER_REGISTER_SERVICES]"

/****************************************************************************************************/

#define SERVICE_CLIENT_DECLARES_MARK            "[SERVICE_CLIENT_DECLARES]"
#define CLIENT_METHOD_DECLARES_MARK             "[CLIENT_METHOD_DECLARES]"
#define CLIENT_METHOD_IMPLEMENTS_MARK           "[CLIENT_METHOD_IMPLEMENTS]"

/****************************************************************************************************/

#define PINK_DIR_MARK                           "[PINK_DIR]"


const char *service_impl_header_template = 
	R"pink_delimiter(
#ifndef _[PROTO_NAME_CAPITAL]_IMPL_H
#define _[PROTO_NAME_CAPITAL]_IMPL_H

[INCLUDES]

namespace pink {

[SERVICE_IMPL_DECLARES]

}

#endif
	)pink_delimiter";


const char *service_impl_declare_template = 
	R"pink_delimiter(
class [SERVICE_NAME]Impl : public [SERVICE_FULL_NAME] {
public:
[SERVICE_IMPL_METHOD_DECLARES]
};
		)pink_delimiter";

const char *service_impl_method_declare_template =
	R"pink_delimiter(
	void [METHOD_NAME](google::protobuf::RpcController* controller,
										 const [REQUEST_TYPE_NAME] *request,
										 [RESPONSE_TYPE_NAME] *response,
										 google::protobuf::Closure *done);
	)pink_delimiter";


/*
 * service implement source file template
 */

const char *service_impl_source_template =
	R"pink_delimiter(
[INCLUDES]

namespace pink {

[SERVICE_IMPL_METHOD_IMPLEMENTS]

}
	)pink_delimiter";

const char *service_impl_method_implement_template =
	R"pink_delimiter(
void [SERVICE_NAME]Impl::[METHOD_NAME](google::protobuf::RpcController* controller,
                                   const [REQUEST_TYPE_NAME] *request,
																	 [RESPONSE_TYPE_NAME] *response,
																	 google::protobuf::Closure *done) {
	(void)controller;
	(void)done;
	//TODO: Implement your server service
}
	)pink_delimiter";


/*
 * server main source file
 */

const char *server_main_source_file_template =
	R"pink_delimiter(
#include "rpc_server.h"
#include "rpc_log.h"
[INCLUDES]

pink::RpcServer *g_server;
pink::RpcConf   *g_conf;
pink::RpcLog    *g_log;

void Usage() {
  fprintf(stderr, "Usage\n\t./[PROTO_NAME]_server -c [PROTO_NAME].conf\n");
}

int main(int argc, char *argv[]) {
  if (argc != 3 || strcmp(argv[1], "-c") != 0) {
    Usage();
    return -1;
  }

  g_server = new pink::RpcServer();
  g_conf = new pink::RpcConf();
  g_log = new pink::RpcLog();

  g_conf->Init(argv[2]);
  g_server->Init(g_conf);
	std::vector<google::protobuf::Service *> services;

	[SERVER_REGISTER_SERVICES]
  g_server->Start();


	for (auto iter = services.begin(); iter != services.end(); ++iter) {
		delete *iter;	
	}
  delete g_server;
  delete g_conf;
  delete g_log;
}
	)pink_delimiter";

const char *server_register_service_template =
	R"pink_delimiter(
	{
		pink::[SERVICE_NAME]Impl *service = new pink::[SERVICE_NAME]Impl();
		g_server->RegisterService(service);
		services.push_back(service);
	}
	)pink_delimiter";

/*
 * server conf file
 */

const char *server_conf_file_template = 
	R"pink_delimiter(
#############server##############
	daemon = false
#############server##############

#############transfer############
	workers_num = 8

## use space to split the local ips your want to bind, for example: 
## local_ips = 1.1.1.1 2.2.2.2 3.3.3.3
	local_ips = 127.0.0.1
	local_port = 8221
#############transfer############

#############transfer############
	log_file = ./log
	log_level = DEBUG
#############transfer############
	)pink_delimiter";

/****************************************************************************************************/

/*
 * client header file 
 */

const char *client_header_file_template =
	R"pink_delimiter(
#ifndef _[PROTO_NAME_CAPITAL]_CLIENT_H
#define _[PROTO_NAME_CAPITAL]_CLIENT_H

#include "rpc_pb_client.h"
[INCLUDES]

namespace pink {

[SERVICE_CLIENT_DECLARES]

}

#endif
	)pink_delimiter";

const char *service_client_declare_template =
	R"pink_delimiter(
class [SERVICE_NAME]Client {
public:

	[SERVICE_NAME]Client() {
		rpc_pb_cli_ = new RpcPbCli();
	}
	~[SERVICE_NAME]Client() {
		delete rpc_pb_cli_;
	}

	Status Connect(const std::string &remote_ip, const int &remote_port) {
		return rpc_pb_cli_->Connect(remote_ip, remote_port);	
	}

[CLIENT_METHOD_DECLARES]

private:
	pink::RpcPbCli* rpc_pb_cli_;
};
	)pink_delimiter";

const char *client_method_declare_template =
	R"pink_delimiter(
	Status [METHOD_NAME]([REQUEST_TYPE_NAME] *req, [RESPONSE_TYPE_NAME] *response);
	)pink_delimiter";


/*
 * client source file
 */

const char *client_source_file_template =
	R"pink_delimiter(
[INCLUDES]
namespace pink {

[CLIENT_METHOD_IMPLEMENTS]

}
	)pink_delimiter";

const char *client_method_implement_template =
	R"pink_delimiter(
Status [SERVICE_NAME]Client::[METHOD_NAME]([REQUEST_TYPE_NAME] *request, [RESPONSE_TYPE_NAME] *response) {
	Status s;
	if ((s = rpc_pb_cli_->Send(request, "[SERVICE_NAME].[METHOD_NAME]"), !s.ok())
			|| (s = rpc_pb_cli_->Recv(response), s.ok())) {
		return s;
	}
	return Status();
}
	)pink_delimiter";
	


/*
 * a client main sample
 */
	
	const char *client_main_file_template =
	R"pink_delimiter(
#include "[PROTO_NAME]_client.h"
#include "[PROTO_NAME].pb.h"
#include <iostream>

int main(int argc, char *argv[]) {

  pink::[SERVICE_NAME]Client client;
	if (!client.Connect("127.0.0.1", 8221).ok()) {
		std::cerr << "Connection failed" << std::endl;
		return -1;
	}
  [REQUEST_TYPE_NAME] request;
  [RESPONSE_TYPE_NAME] response;
  //set your own request, for example: request.set_member("your member");
  if (client.[METHOD_NAME](&request, &response).ok()) {
    std::cerr << "[METHOD_NAME] method execution success!" << std::endl;
  } else {
    std::cerr << "[METHOD_NAME] method execution failed!" << std::endl;
  }

  return 0;
}	
	)pink_delimiter";


/*
 * Makefile
 */

const char *makefile_template =
	R"pink_delimiter(
PINK_DIR = [PINK_DIR]

PROTOBUF_DIR = $(PINK_DIR)/deps/protobuf

CC = g++
CC_FLAGS = -g -std=c++11
LD_FLAGS =
INCLUDE_PATH = -I$(PROTOBUF_DIR)/include/ \
               -I/usr/local/include/ \
               -I$(PINK_DIR)/output/include/ \
               -I$(PINK_DIR)/output/ \
               -I./

LIB_PATH = -L$(PROTOBUF_DIR)/lib/ \
           -L$(PINK_DIR)/output/lib/ \
           -L./

LIBS = -lpthread \
       -Wl,-Bstatic \
       -lpink \
       -lprotobuf \
       -Wl,-Bdynamic \

ALL_SOURCES = $(wildcard ./*.cc)
MAIN_SOURCES = $(wildcard ./*main.cc)
CLIENT_MAIN_SOURCE = $(wildcard ./*client_main.cc)
SERVER_MAIN_SOURCE = $(filter-out $(CLIENT_MAIN_SOURCE), $(MAIN_SOURCES))
PROTO_SOURCE = [PROTO_NAME].pb.cc

BASE_SOURCES = $(filter-out $(MAIN_SOURCES) $(PROTO_SOURCE), $(ALL_SOURCES))
CLIENT_BASE_SOURCES = $(wildcard ./*client.cc)
SERVER_BASE_SOURCES = $(filter-out $(CLIENT_BASE_SOURCES), $(BASE_SOURCES))

#SERVER_BASE_SOURCES CLIENT_BASE_SOURCES PROTO_SOURCE SERVER_MAIN_SOURCE CLIENT_MAIN_SOURCE is needed
SERVER_BASE_OBJS = $(patsubst %.cc, %.o, $(SERVER_BASE_SOURCES))
CLIENT_BASE_OBJS = $(patsubst %.cc, %.o, $(CLIENT_BASE_SOURCES))
PROTO_OBJ       =  $(patsubst %.cc, %.o, $(PROTO_SOURCE))
SERVER_MAIN_OBJ = $(patsubst %.cc, %.o, $(SERVER_MAIN_SOURCE))
CLIENT_MAIN_OBJ = $(patsubst %.cc, %.o, $(CLIENT_MAIN_SOURCE))


SERVER_MAIN = [PROTO_NAME]_server_main
CLIENT_LIB = lib[PROTO_NAME]_client.a
CLIENT_MAIN = [PROTO_NAME]_client_main

#######################################################################################################

.PHONY: all server_main client_lib client_main clean

all: server_main client_lib client_main
	mkdir -p ./output
	cp $(SERVER_MAIN) ./output
	cp $(CLIENT_LIB) ./output
	cp $(CLIENT_MAIN) ./output
	cp [PROTO_NAME]_client.h ./output
	cp [PROTO_NAME].conf ./output
	@echo "All Building succeeds"

server_main: $(SERVER_MAIN)
	@echo "Building $(SERVER_MAIN) succeeds"
$(SERVER_MAIN): $(PROTO_OBJ) $(SERVER_MAIN_OBJ) $(SERVER_BASE_OBJS)
	$(CC) $(LD_FLAGS) -o $@ $^ $(LIB_PATH) $(LIBS)

client_main: $(CLIENT_MAIN)
	@echo "Building $(CLIENT_MAIN) succeeds"
$(CLIENT_MAIN): $(PROTO_OBJ) $(CLIENT_MAIN_OBJ) $(CLIENT_LIB)
	$(CC) $(LD_FLAGS) -o $@ $^ $(LIB_PATH) $(LIBS)

client_lib: $(CLIENT_LIB)
	@echo "Building $(CLIENT_LIB) succeeds"
$(CLIENT_LIB): $(CLIENT_BASE_OBJS)
	rm -f $@
	ar -rcs $@ $^

$(SERVER_BASE_OBJS) : %.o : %.cc
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c -o $@ $<
$(CLIENT_BASE_OBJS) : %.o : %.cc
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c -o $@ $<
$(PROTO_OBJ) : %.o : %.cc
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c -o $@ $<
$(SERVER_MAIN_OBJ) : %.o : %.cc
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c -o $@ $<
$(CLIENT_MAIN_OBJ) : %.o : %.cc
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c -o $@ $<

$(PROTO_SOURCE) : %.pb.cc : %.proto
	$(PROTOBUF_DIR)/bin/protoc --proto_path ./ --cpp_out=./ $<

clean:
	rm *.pb.* *.o $(SERVER_MAIN) $(CLIENT_LIB) $(CLIENT_MAIN)	
	)pink_delimiter";
	
	
}
#endif
