#ifndef _RPC_MONITOR_H
#define _RPC_MONITOR_H

#include <atomic>
#include <stdint.h>

#include "rpc_common.h"

namespace pink {

class RpcMonitor {
public:
	RpcMonitor() :
		acc_reqs_(0),
		last_acc_reqs_(0),
		last_times_(microtime()) {
	}

	/*
 	 * field updating
 	 */

	void incr_acc_reqs() {
		++acc_reqs_;
	}

	void set_last_times(uint64_t value) {
		last_times_ = value;
	}

	void set_last_acc_reqs(uint64_t value) {
		last_acc_reqs_ = value;
	}

	void set_qps(float value) {
		qps_ = value;
	}
	/*
	 * field getting
	 */

	uint64_t acc_reqs() {
		return acc_reqs_.load();
	}

	uint64_t last_times() {
		return last_times_;
	}

	uint64_t last_acc_reqs() {
		return last_acc_reqs_;
	}

	float qps() {
		return qps_;	
	}

private:
	std::atomic<uint64_t> acc_reqs_;

/*
 * unit is microsecond
 */
	uint64_t last_times_;
	uint64_t last_acc_reqs_;
	float qps_;
};

}

#endif
