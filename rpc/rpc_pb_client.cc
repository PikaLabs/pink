#include "rpc_pb_client.h"
#include "rpc_define.h"
#include "pink_define.h"
#include "google/protobuf/message.h"

namespace pink {

void RpcPbCli::BuildWbuf() {
	uint32_t name_len = method_fname_.size();
	uint32_t msg_len = msg_->ByteSize();

	if (name_len + msg_len + RPC_PB_COMMAND_HEADER_LENGTH > PB_MAX_MESSAGE) {
//		LOG_WARN("Command length too large\n");
//TODO: implete the log later
		return;
	}
	
	memcpy(wbuf_ + RPC_PB_COMMAND_HEADER_LENGTH, method_fname_.data(), method_fname_.size());
	msg_->SerializeToArray(wbuf_ + RPC_PB_COMMAND_HEADER_LENGTH + name_len, PB_MAX_MESSAGE - RPC_PB_COMMAND_HEADER_LENGTH - name_len);

	wbuf_len_ = RPC_PB_COMMAND_HEADER_LENGTH + name_len + msg_len;

	name_len = htonl(name_len);
	msg_len = htonl(msg_len);
	memcpy(wbuf_, &name_len, sizeof(name_len));
	memcpy(wbuf_ + sizeof(name_len), &msg_len, sizeof(msg_len));	
}

Status RpcPbCli::Send(void *msg, const std::string &method_fname) {
	method_fname_ = method_fname;
	return PbCli::Send(msg);
}


}
