#ifndef _RPC_SERVER_H
#define _RPC_SERVER_H

#include <google/protobuf/service.h>

#include "rpc_transfer.h"
#include "rpc_monitor.h"
#include "rpc_controller.h"
#include "rpc_pb_conn.h"

namespace pink {

struct RpcServer {
public:
	RpcServer() :
		shutdown_(false),
		transfer_(&monitor_, &controller_) {
//		monitor_ = new RpcMonitor();
//		controller_ = new RpcController();
//		transfer_ = new RpcTransfer(monitor_, controller_);
	}

	~RpcServer() {
//		if (monitor_) {
//			delete monitor_;
//			monitor_ = NULL;
//		}	
//		if (controller_) {
//			delete controller_;
//			controller_ = NULL;
//		}
//		if (transfer_ = NULL) {
//			delete controller_;
//			controller_ = NULL;
//		}
	}

	void Init(const RpcConf *conf);
	int32_t RegisterService(google::protobuf::Service *service);
	void Start();
	void DoTimeWork();
	void Stop();

	const std::map<std::string, google::protobuf::Service *> &services() const {
		return services_;
	}

	RpcMonitor *monitor() {
		return &monitor_;
	}

	RpcController *controller() {
		return &controller_;
	} 

private:
	std::atomic<bool> shutdown_;
	std::map<std::string, google::protobuf::Service *> services_;
	
	RpcMonitor monitor_;
	RpcController controller_;
	RpcTransfer<RpcPbConn> transfer_;
};

}
#endif
