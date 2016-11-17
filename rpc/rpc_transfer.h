#ifndef _RPC_TRANSFER_H
#define _RPC_TRANSFER_H_

#include <strings.h>

#include "dispatch_thread.h"
#include "worker_thread.h"
#include "rpc_define.h"
#include "rpc_conf.h"
#include "rpc_monitor.h"
#include "rpc_controller.h"
#include <set>

namespace pink {

template<typename ProtoConn>
class RpcWorkerThread : public WorkerThread<ProtoConn> {
public:
	RpcWorkerThread(RpcMonitor *monitor,
									RpcController *controller,
									int32_t cron_interval = 0):
		monitor_(monitor),
		controller_(controller),
		WorkerThread<ProtoConn>(cron_interval) {
	}

private:		
	RpcMonitor *monitor_;
	RpcController *controller_;

	void CronHandle() override;
};

template<typename ProtoConn>
class RpcDispatchThread : public DispatchThread<ProtoConn> {
public:
	RpcDispatchThread(const std::set<std::string> &ips,
										int32_t port, 
										int32_t workers_num,
										WorkerThread<ProtoConn> **worker_thread, 
										RpcMonitor *monitor, 
										RpcController *controller, 
										int32_t cron_interval = 0):
		DispatchThread<ProtoConn>(ips, port, workers_num, worker_thread, cron_interval),
		monitor_(monitor),
		controller_(controller) {
	}

private:
	RpcMonitor *monitor_;
	RpcController *controller_;

	void CronHandle() override;
	bool AccessHandle(const std::string &ip_port) override;
};


template<typename ProtoConn>
class RpcTransfer {
public:
	RpcTransfer(RpcMonitor* monitor, RpcController* controller);
	~RpcTransfer();
	void Init(const RpcConf *conf);
	void Start();
	void Stop();
private:
	DispatchThread<ProtoConn>* dispatch_;
	WorkerThread<ProtoConn>* workers_[RPC_MAX_WORKER_NUM];
	int32_t workers_num_;

	RpcMonitor *monitor_;
	RpcController *controller_;

	PINK_PROHIBIT_EVIL_COPY(RpcTransfer);
};

template<typename ProtoConn>
void RpcWorkerThread<ProtoConn>::CronHandle() {
	//TODO: Impletement the worker CronHandle

}

template<typename ProtoConn>
void RpcDispatchThread<ProtoConn>::CronHandle() {
	//TODO: Impletement the dispatch CronHandle

}

template<typename ProtoConn>
bool RpcDispatchThread<ProtoConn>::AccessHandle(const std::string &ip_port) {
	//TODO: Impletement the dispatch CronHandle
	return true;
}

template<typename ProtoConn>	
RpcTransfer<ProtoConn>::RpcTransfer(RpcMonitor* monitor, RpcController* controller) :
	dispatch_(NULL),
	monitor_(monitor),
	controller_(controller) {
	memset(workers_, 0, sizeof(workers_));
}

template<typename ProtoConn>
RpcTransfer<ProtoConn>::~RpcTransfer() {
	for (int32_t idx = 0; idx < workers_num_; ++idx) {
		if (workers_[idx]) {
			delete workers_[idx];
		}
	}
	if (dispatch_) {
		delete dispatch_;
	}
	//monitor_ and controller_ deletetion acts in upper layer(RpcServer's destructor function)
}

template<typename ProtoConn>
void RpcTransfer<ProtoConn>::Init(const RpcConf *conf) {
	workers_num_ = conf->workers_num();
	for (int32_t idx = 0; idx < workers_num_; ++idx) {
		workers_[idx] = new RpcWorkerThread<ProtoConn>(monitor_, controller_, RPC_WORKER_CRON_INTERVAl);
	}
	dispatch_ = new RpcDispatchThread<ProtoConn>(conf->local_ips(), conf->local_port(), workers_num_, workers_, monitor_, controller_, RPC_DISPATCH_CRON_INTERVAL);
}

template<typename ProtoConn>
void RpcTransfer<ProtoConn>::Start() {
	dispatch_->StartThread();
}

template<typename ProtoConn>
void RpcTransfer<ProtoConn>::Stop() {
	dispatch_->should_exit_ = true;
	for (int32_t idx = 0; idx < workers_num_; ++idx) {
		workers_[idx]->should_exit_ = true;
	}

	pthread_join(dispatch_->thread_id(), NULL);
	for (int32_t idx = 0; idx < workers_num_; ++idx) {
		pthread_join(workers_[idx]->thread_id(), NULL);
	}
}

}
#endif
