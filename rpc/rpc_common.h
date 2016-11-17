#ifndef _RPC_COMMON_H
#define _RPC_COMMON_H

#include <string>
#include <stdint.h>
#include <vector>

#include <signal.h>
#include <sys/time.h>

namespace pink {

void InitSignalHandler();
std::string GetDate();

std::string Trim(const std::string &str, const std::string del_str = " \t\n");
bool Split(const std::string &str, std::vector<std::string>& tokens, const std::string &deli = " \t\n");

inline uint64_t microtime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;	
}

}
#endif
