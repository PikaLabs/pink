#include <time.h>
#include <iostream>
#include <fstream>

#include "rpc_common.h"
#include "rpc_server.h"


extern pink::RpcServer *g_server;

namespace pink {

void SignalHandler0(int32_t signal_num) {
	g_server->Stop();
}

void InitSignalHandler() {
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, SignalHandler0);
	signal(SIGQUIT, SignalHandler0);
	signal(SIGTERM, SignalHandler0);
}

std::string GetDate() {
	time_t t = time(NULL);
	struct tm *p = localtime(&t);
	return std::to_string(p->tm_yday) + std::to_string(p->tm_mon) + std::to_string(p->tm_mday);	
}

std::string Trim(const std::string &str, const std::string del_str) {
	std::string::size_type begin, end;
	std::string ret;
	if ((begin = str.find_first_not_of(del_str)) == std::string::npos) {
		return ret; 
	}
	end = str.find_last_not_of(del_str);
	return str.substr(begin, end-begin+1);
}

bool Split(const std::string &str, std::vector<std::string> &tokens, const std::string &deli) {
	char *ptr = NULL, *save = NULL;
	const char *deli_p = deli.c_str();
	std::string bak = str;
	ptr = strtok_r(const_cast<char *>(bak.c_str()), deli.c_str(), &save);
	while (ptr) {
		tokens.push_back(ptr);
		ptr = strtok_r(NULL, deli_p, &save);
	}
	return true;
}


}
