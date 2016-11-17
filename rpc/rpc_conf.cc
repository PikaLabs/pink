#include <fstream>
#include <strings.h>

#include "rpc_conf.h"
#include "rpc_define.h" 
#include "rpc_common.h"

namespace pink {

void RpcConf::LoadConf(const std::string &conf_file) {
	std::ifstream in(conf_file.c_str());
	if (!in.good()) {
		fprintf(stderr, "cannot open the specified conf file");
	}

	std::string line, item_key, item_value;
	std::string::size_type pos;
	int32_t line_no;

	while (!in.eof()) {
		++line_no;
		getline(in, line);

		line = Trim(line);
		if (line.empty() || line.at(0) == '#') {
			continue;
		}
		if ((pos = line.find("=")) == std::string::npos
				|| pos == (line.size() - 1)) {
			fprintf(stderr, "error item format at %d line in %s\n", line_no, conf_file.c_str());
			continue;
		}

		item_key = line.substr(0, pos);
		item_key = Trim(item_key);
		item_value = line.substr(pos + 1);
		item_value = Trim(item_value);
		items_[item_key] = item_value;
	}

	in.close();
}

void RpcConf::Init(const std::string &conf_file) {
	conf_file_ = conf_file;

	LoadConf(conf_file_);

	/*
	 * Server
	 */
	GetConfBool("daemon", &daemon_);

	/*
	 * Transfer
	 */
	GetConfInt("workers_num", &workers_num_);
	GetConfStrSet("local_ips", &local_ips_);
	GetConfInt("local_port", &local_port_);

	/*
	 * Log
	 */
	GetConfStr("log_file", &log_file_);
	GetConfStr("log_level", &log_level_);
}

bool RpcConf::GetConfInt(const std::string &item_key, int32_t* const item_value) {
	std::map<std::string, std::string>::const_iterator iter;
	if ((iter = items_.find(item_key)) == items_.end()) {
		return false;
	}
	*item_value = atoi(iter->second.c_str());
	return true;
}

bool RpcConf::GetConfStr(const std::string &item_key, std::string* const item_value) {
	std::map<std::string, std::string>::const_iterator iter;
	if ((iter = items_.find(item_key)) == items_.end()) {
		return false;
	}
	*item_value = iter->second;
	return true;
}

bool RpcConf::GetConfBool(const std::string &item_key, bool* const item_value) {
	std::map<std::string, std::string>::const_iterator iter;
	if ((iter = items_.find(item_key)) == items_.end()) {
		return false;
	}

	const char *value_str = iter->second.c_str();
	std::string::size_type size = iter->second.size();

	if (strncasecmp(value_str, "true", size) == 0
			|| strncasecmp(value_str, "1", size) == 0
			|| strncasecmp(value_str, "yes", size == 0)) {
		*item_value = true;
	} else if (strncasecmp(value_str, "false", size) == 0
			|| strncasecmp(value_str, "0", size) == 0
			|| strncasecmp(value_str, "no", size) == 0) {
		*item_value = false;
	} else {
		return false;
	}

	return true;
}

bool RpcConf::GetConfStrSet(const std::string &item_key, std::set<std::string>* const item_value) {
	std::map<std::string, std::string>::const_iterator iter;
	if ((iter = items_.find(item_key)) == items_.end()) {
		return false;
	}

	std::vector<std::string> tmp_v;
	Split(iter->second, tmp_v);
	for(auto str : tmp_v) {
		item_value->insert(str);
	}
	return true;
}

}


