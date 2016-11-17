#include "rpc_server.h"
#include "rpc_common.h"

#include <stdlib.h>


namespace pink {

void RpcServer::Init(const RpcConf *conf) {
	transfer_.Init(conf);
}

int32_t RpcServer::RegisterService(google::protobuf::Service *service) {
	const google::protobuf::ServiceDescriptor *service_descriptor = service->GetDescriptor();
	std::string service_name = service_descriptor->name();
	services_[service_name] = service;
}

void RpcServer::Start() {
	transfer_.Start();
	DoTimeWork();
	transfer_.Stop();
}

void RpcServer::DoTimeWork() {
	while (!shutdown_) {
		sleep(RPC_SERVER_CRON_INTERVAL);

		//TODO: Implement some time work, such as statistics and monitoring

		uint64_t cur_reqs = monitor_.acc_reqs();
		uint64_t cur_times = microtime();
		double qps = (cur_reqs - monitor_.last_acc_reqs()) * 1000000.0 / (cur_times - monitor_.last_times());
		monitor_.set_qps(qps);
		monitor_.set_last_acc_reqs(cur_reqs);
		monitor_.set_last_times(cur_times);
		
	}
}

void RpcServer::Stop() {
	shutdown_.store(true);
}

}
