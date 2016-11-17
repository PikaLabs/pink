#include <stdio.h>
#include <string>
#include <stdarg.h>

#include "rpc_log.h"

namespace pink {

std::string RpcLog::rpc_log_level_str[kMaxLevel] = {
	"[ DEBUG ]",
	"[ TRACE ]",
	"[ INFO  ]",
	"[ WARN  ]",
	"[ ERROR ]",
	"[ FATAL ]"
};

void RpcLog::Init(const RpcConf *conf) {
	log_file_name_ = conf->log_file();
	if (log_file_name_.empty()) {
		log_file_name_ = "./rpc_log_" + GetDate();
	}
	log_file_ = fopen(log_file_name_.c_str(), "a+");
	if (log_file_) {
		fprintf(stderr, "log file open failed\n");
		exit(-1);
	}
	
	log_daemon_ = conf->daemon();
}


void RpcLog::Log(RpcLogLevel level, char *format, ...) {
	static const std::string format_prefix = rpc_log_level_str[level] + "(%d:%d) ";
	if (level < log_level_) {
		return;
	}
	va_list ap;
	va_start(ap, format);
	fprintf(log_file_, (format_prefix + format).c_str(), __FILE__, __LINE__, ap);

	if (log_daemon_) {
		va_start(ap, format);
		fprintf(stderr, (format_prefix + format).c_str(), __FILE__, __LINE__, ap);
	}
	va_end(ap);
}

}
