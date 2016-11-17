#ifndef _RPC_CONF_H
#define _RPC_CONF_H

#include <string>
#include <map>
#include <set>

namespace pink { 

#define CONF_COMMENT_CHAR '#'

class RpcConf {
public:
	RpcConf() :
		local_ips_({"0.0.0.0"}) {	
	}

	~RpcConf() {
	}

	void LoadConf(const std::string &conf_file);
	void Init(const std::string &conf_file);

	int32_t workers_num() const {
		return workers_num_;
	}

	const std::set<std::string>& local_ips() const {
		return local_ips_;
	}
	
	int32_t local_port() const {
		return local_port_;
	}
	
	bool daemon() const {
		return daemon_;
	}

	std::string log_file() const {
		return log_file_;
	}

	std::string log_level() const {
		return log_level_;
	}

	bool GetConfInt(const std::string &item_key, int32_t* const item_value);
	bool GetConfStr(const std::string &item_key, std::string* const item_value);
	bool GetConfBool(const std::string &item_key, bool* const item_value);
	bool GetConfStrSet(const std::string &item_key, std::set<std::string>* const item_value);

private:
	int32_t workers_num_;
	std::set<std::string> local_ips_;
	int32_t local_port_;

	bool daemon_;

	std::string log_file_;
	std::string log_level_;	

	std::map<std::string, std::string> items_;
	std::string conf_file_;
};

}

#endif
