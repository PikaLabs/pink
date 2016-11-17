#include "rpc_pb_conn.h"
#include "rpc_define.h"
#include "rpc_server.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/service.h"
#include "google/protobuf/message.h"

extern pink::RpcServer *g_server;

namespace pink {

RpcPbConn::RpcPbConn(const int fd, const std::string &ip_port, pink::Thread *thread) :
	PbConn(fd, ip_port),
	msg_(NULL),
	resp_(NULL),
	thread_(thread) {
}

RpcPbConn::~RpcPbConn() {
	if (msg_) {
		delete msg_;
	}

	if (resp_) {
		delete resp_;
	}
}

ReadStatus RpcPbConn::GetRequest() {

  while (true) {
    switch (connStatus_) {
      case kHeader: {
        ssize_t nread = read(fd(), rbuf_ + rbuf_len_, RPC_PB_COMMAND_HEADER_LENGTH - rbuf_len_);
        if (nread == -1) {
          if (errno == EAGAIN) {
            return kReadHalf;
          } else {
            return kReadError;
          }
        } else if (nread == 0) {
          return kReadClose;
        } else {
          rbuf_len_ += nread;
          if (rbuf_len_ - cur_pos_ == RPC_PB_COMMAND_HEADER_LENGTH) {
            uint32_t integer = 0;

            memcpy((char *)(&integer), rbuf_ + cur_pos_, sizeof(uint32_t));
						method_fname_len_ = ntohl(integer);

						memcpy((char *)(&integer), rbuf_ + cur_pos_ + sizeof(uint32_t), sizeof(uint32_t));
						msg_len_ = ntohl(integer);

						header_len_ = method_fname_len_ + msg_len_;
            remain_packet_len_ = header_len_;
            cur_pos_ += RPC_PB_COMMAND_HEADER_LENGTH;
            connStatus_ = kPacket;
          }
          log_info("GetRequest kHeader header_len=%u cur_pos=%u rbuf_len=%u remain_packet_len_=%d nread=%d\n", header_len_, cur_pos_, rbuf_len_, remain_packet_len_, nread);
        }
        break;
      }
      case kPacket: {
        if (header_len_ >= PB_MAX_MESSAGE - RPC_PB_COMMAND_HEADER_LENGTH) {
          return kFullError;
        } else {
          // read msg body
          ssize_t nread = read(fd(), rbuf_ + rbuf_len_, remain_packet_len_);
          if (nread == -1) {
            if (errno == EAGAIN) {
              return kReadHalf;
            } else {
              return kReadError;
            }
          } else if (nread == 0) {
            return kReadClose;
          }

          rbuf_len_ += nread;
          remain_packet_len_ -= nread;
          if (remain_packet_len_ == 0) {
            cur_pos_ = rbuf_len_;
            connStatus_ = kComplete;
          }
          log_info("GetRequest kPacket header_len=%u cur_pos=%u rbuf_len=%u remain_packet_len_=%d nread=%d\n", header_len_, cur_pos_, rbuf_len_, remain_packet_len_, nread);
        }
        break;
      }
      case kComplete: {
        DealMessage();
        connStatus_ = kHeader;

        cur_pos_ = 0;
        rbuf_len_ = 0;
        return kReadAll;
      }
      // Add this switch case just for delete compile warning
      case kBuildObuf:
        break;

      case kWriteObuf:
        break;
    }
  }

  return kReadHalf;
}

int RpcPbConn::DealMessage() {
	method_fname_.assign(rbuf_ + RPC_PB_COMMAND_HEADER_LENGTH, method_fname_len_);
	std::string service_name, method_name;
	std::string::size_type pos = method_fname_.rfind(".");
	method_name = method_fname_.substr(pos+1);
	service_name = method_fname_.substr(0, pos);
	
	std::map<std::string, google::protobuf::Service *>::const_iterator iter;
	const std::map<std::string, google::protobuf::Service *> &services = g_server->services();
	if ((iter = services.find(service_name)) == services.end()) {
		return -1;	
	}
	
	const google::protobuf::ServiceDescriptor *serv_des = iter->second->GetDescriptor();
	const google::protobuf::MethodDescriptor *meth_des = serv_des->FindMethodByName(method_name);
	if (meth_des == NULL) {
		return -1;
	}
	
	if (!msg_ || typeid(*msg_) != typeid(iter->second->GetRequestPrototype(meth_des))) {
		delete msg_; // deleting NULL is ok, equaling nop;
		msg_ = iter->second->GetRequestPrototype(meth_des).New();
	}
	if (!resp_ || typeid(*resp_) != typeid(iter->second->GetResponsePrototype(meth_des))) {
		delete resp_;
		resp_ = iter->second->GetResponsePrototype(meth_des).New();
	}

	if (!msg_->ParseFromArray(rbuf_ + RPC_PB_COMMAND_HEADER_LENGTH + method_fname_len_, msg_len_)) {
		return -1;
	}
	iter->second->CallMethod(meth_des, NULL, msg_, resp_, NULL);
	
	g_server->monitor()->incr_acc_reqs();

	res_ = resp_;
	set_is_reply(true);
	return 0;
}

}
