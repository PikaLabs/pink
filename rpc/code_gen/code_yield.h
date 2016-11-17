#ifndef _CODE_YIELD_H
#define _CODE_YIELD_H

#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/compiler/importer.h"

namespace pink {

class CodeYieldErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
public:
	void AddError(const std::string &filename, int32_t line, int32_t colum, const std::string &message) override;
};

class CodeYield {
public:
	CodeYield(const std::string service_file) :
		service_file_(service_file),
		importer_(NULL) {	
	}
	~CodeYield() {
		if (importer_) {
			delete importer_;
		}
	}

	bool ParseServiceFile();

	bool YieldServer();
	bool YieldServerServiceImpl();
	bool YieldServerServiceImplHeader();
	bool YieldServerServiceImplSource();
	bool YieldServerMain();
	bool YieldServerConf();

	bool YieldClient();
	bool YieldClientLib();
	bool YieldClientMain();

	bool YieldMakefile();

	bool CopyDependentFiles();

private:
	std::string service_file_;
	std::string proto_name_;
	std::vector<const google::protobuf::ServiceDescriptor *> service_descriptors_;

	google::protobuf::compiler::Importer *importer_;

	static const google::protobuf::MessageFactory* message_factory;
};

}

#endif
