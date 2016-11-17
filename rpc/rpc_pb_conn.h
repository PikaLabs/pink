#ifndef _RPC_PB_CONN_H
#define _RPC_PB_CONN_H

#include "pb_conn.h"
#include "pink_thread.h"
#include "google/protobuf/descriptor.h"

namespace pink {

class RpcPbConn : public PbConn {
public:
	RpcPbConn(const int fd, const std::string &ip_port, pink::Thread *thread);
	~RpcPbConn();
	ReadStatus GetRequest() override;

private:
	int DealMessage() override;
	ReadStatus ParseMethodFullName();
	google::protobuf::ServiceDescriptor* GetServiceDescriptor(std::string service_name);

	std::string method_fname_;
	int32_t method_fname_len_;
	int32_t msg_len_;
	google::protobuf::Message *msg_;
	google::protobuf::Message *resp_;

	Thread *thread_;
};


}
#endif
