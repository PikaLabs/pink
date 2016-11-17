#include <fstream>
#include <iostream>
#include <vector>
#include <functional>

#include "utils.h"
#include "define.h"
#include "rpc_template.h"
#include "code_yield.h" 
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/compiler/importer.h"

namespace pink {

void CodeYieldErrorCollector::AddError(const std::string &filename , int32_t line, int32_t colum,
                                       const std::string &message) {
	std::cerr << " filename: " << filename
		        << ",    line: " << line
						<< ",   colum: " << colum
						<< ", message: " << message << std::endl;
}

bool CodeYield::ParseServiceFile() {
	std::string::size_type pos = service_file_.rfind("/");
	if (pos == std::string::npos) {
		pos = 0;
	} else {
		++pos;
	}
	proto_name_ = service_file_.substr(pos, service_file_.rfind(SERVICE_FILE_SUFFIX));

//	const google::protobuf::FileDescriptor *file_descriptor =
//		google::protobuf::DescriptorPool::generated_pool()->FindFileByName(service_file_);
	CodeYieldErrorCollector error_collector_;
	google::protobuf::compiler::DiskSourceTree source_tree;
	source_tree.MapPath("", "./");
	google::protobuf::compiler::Importer *importer = new google::protobuf::compiler::Importer(&source_tree, &error_collector_);
	importer_ = importer;
	const google::protobuf::FileDescriptor *file_descriptor = NULL;
//	importer->AddUnusedImportTrackFile(service_file_);
	file_descriptor = importer->Import(service_file_);
	if (file_descriptor == NULL) {
		std::cerr << "Build " << service_file_ << " relating file descriptor failed!" << std::endl;
		return false;
	}
	int32_t service_count = file_descriptor->service_count();
	for (int32_t idx = 0; idx < service_count; ++idx) {
		const google::protobuf::ServiceDescriptor* sd = file_descriptor->service(idx);
		service_descriptors_.push_back(sd);
	}
	return true;
}

bool CodeYield::YieldServer() {
	//Yield server conf	
	YieldServerConf();
	
	//Yield service implementation
	YieldServerServiceImpl();

	//Yield server
	YieldServerMain();	
}

bool CodeYield::YieldServerServiceImpl() {
	/*************** header file used **************/
	std::string header_file_content = service_impl_header_template; //Inilialization needn't, just for reading, and some similar actions in the following
	std::string proto_name_capital_content = ToUpper(proto_name_);;
	std::string header_includes_content = "#include \"" + proto_name_ + ".pb.h\"";
	std::string service_impl_declares_content;

	std::string service_impl_declare = service_impl_declare_template;
	std::string service_name_content;
	std::string service_full_name_content;
	std::string service_impl_method_declares_content;

	std::string service_impl_method_declare = service_impl_method_declare_template;
	std::string method_name_content;
	std::string request_type_name_content;
	std::string response_type_name_content;
	/*************** header file used **************/

	/*************** source file used **************/
	std::string source_file_content = service_impl_source_template;
	std::string source_includes_content = "#include \"" + proto_name_ + "_impl.h\"";
	std::string service_impl_method_implements_content;

	std::string service_impl_method_implement = service_impl_method_implement_template;
	/*************** source file used **************/
	
	

	/*************** temp use **************/
	int32_t method_count;
	const google::protobuf::MethodDescriptor *method_descriptor = NULL;
	/*************** temp use **************/

	for (std::vector<const google::protobuf::ServiceDescriptor *>::const_iterator iter = service_descriptors_.begin();
			iter != service_descriptors_.end();
			++iter) {
		service_impl_declare = service_impl_declare_template;
		service_name_content = (*iter)->name();
		service_full_name_content = (*iter)->full_name();
		Replace(service_full_name_content, ".", "::");
		service_impl_method_declares_content.clear();

		method_count = (*iter)->method_count();
		for (int32_t idx = 0; idx < method_count; ++idx) {
			method_descriptor = (*iter)->method(idx);
			method_name_content = method_descriptor->name();
			request_type_name_content = method_descriptor->input_type()->full_name();
			Replace(request_type_name_content, ".", "::");
			response_type_name_content = method_descriptor->output_type()->full_name();
			Replace(response_type_name_content, ".", "::");


			service_impl_method_declare = service_impl_method_declare_template;
			Replace(service_impl_method_declare, METHOD_NAME_MARK, method_name_content);
			Replace(service_impl_method_declare, REQUEST_TYPE_NAME_MARK, request_type_name_content);
			Replace(service_impl_method_declare, RESPONSE_TYPE_NAME_MARK, response_type_name_content);
			service_impl_method_declares_content.append(service_impl_method_declare);

			service_impl_method_implement = service_impl_method_implement_template;
			Replace(service_impl_method_implement, SERVICE_NAME_MARK, service_name_content);
			Replace(service_impl_method_implement, METHOD_NAME_MARK, method_name_content);
			Replace(service_impl_method_implement, REQUEST_TYPE_NAME_MARK, request_type_name_content);
			Replace(service_impl_method_implement, RESPONSE_TYPE_NAME_MARK, response_type_name_content);
			service_impl_method_implements_content.append(service_impl_method_implement);
		}
		
		Replace(service_impl_declare, SERVICE_NAME_MARK, service_name_content);
		Replace(service_impl_declare, SERVICE_FULL_NAME_MARK, service_full_name_content);
		Replace(service_impl_declare, SERVICE_IMPL_METHOD_DECLARES_MARK, service_impl_method_declares_content);

		service_impl_declares_content.append(service_impl_declare);
	}

	Replace(header_file_content, PROTO_NAME_CAPITAL_MARK, proto_name_capital_content);
	Replace(header_file_content, INCLUDES_MARK, header_includes_content);
	Replace(header_file_content, SERVICE_IMPL_DECLARES_MARK, service_impl_declares_content);

	Replace(source_file_content, INCLUDES_MARK, source_includes_content);
	Replace(source_file_content, SERVICE_IMPL_METHOD_IMPLEMENTS_MARK, service_impl_method_implements_content);
	
	std::string header_file_name = proto_name_ + "_impl.h";
	if (Pour2File(header_file_name, header_file_content) == -1) {
		return false;
	}

	std::string source_file_name = proto_name_ + "_impl.cc";
	if (Pour2File(source_file_name, source_file_content) == -1) {
		return false;
	}
	return true;
}

bool CodeYield::YieldServerMain() {
	std::string server_main_source_file_content = server_main_source_file_template;	
	std::string includes_content;
	std::string proto_name_content = proto_name_;
	std::string server_register_services_content;

	std::string service_register_service = server_register_service_template;
	std::string service_name_content;

	std::vector<const google::protobuf::ServiceDescriptor *>::const_iterator iter;
	std::string include;
	for (iter = service_descriptors_.begin(); iter != service_descriptors_.end(); ++iter) {
		service_name_content = (*iter)->name();
		
		include = "#include \"" + proto_name_ + "_impl.h\"\n";
		includes_content.append(include);

		service_register_service = server_register_service_template;
		Replace(service_register_service, SERVICE_NAME_MARK, service_name_content);
		server_register_services_content.append(service_register_service);		
	}

	Replace(server_main_source_file_content, INCLUDES_MARK, includes_content);
	Replace(server_main_source_file_content, PROTO_NAME_MARK, proto_name_content);
	Replace(server_main_source_file_content, SERVER_REGISTER_SERVICES_MARK, server_register_services_content);

	std::string main_source_name = proto_name_ + "_main.cc";
	if (Pour2File(main_source_name, server_main_source_file_content) == -1) {
		return false;
	}
	return true;
}

bool CodeYield::YieldServerConf() {
	std::string server_conf_file_name = proto_name_ + ".conf";
	std::string server_conf_file_content = server_conf_file_template;
	if (Pour2File(server_conf_file_name, server_conf_file_content) == -1) {
		return false;
	}
	return true;
}

bool CodeYield::YieldClient() {
	// Yield the service related library	
	YieldClientLib();

	//Yiled a client sample main source
	YieldClientMain();

	return true;
}


bool CodeYield::YieldClientLib() {
	/*************** header file used **************/
	std::string client_header_file_content = client_header_file_template;
	std::string proto_name_capital_content = ToUpper(proto_name_);
	std::string header_includes_content = "#include \"" + proto_name_ + ".pb.h\"";
	std::string service_client_declares_content;
	
	std::string service_client_declare;
	std::string service_name_content;
	std::string client_method_declares_content;

	std::string client_method_declare;
	std::string method_name_content;
	std::string request_type_name_content;
	std::string response_type_name_content;
	/*************** header file used **************/

	/*************** source file used **************/
	std::string client_source_file_content = client_source_file_template;
	std::string source_includes_content = "#include \"" + proto_name_ + "_client.h\"";
	std::string client_method_implements_content;

	std::string client_method_implement = client_method_implement_template;
	/*************** source file used **************/

	std::vector<const google::protobuf::ServiceDescriptor *>::const_iterator iter;
	const google::protobuf::MethodDescriptor *method_descriptor = NULL;
	int32_t method_count;
	for (iter = service_descriptors_.begin(); iter != service_descriptors_.end(); ++iter) {
		service_name_content = (*iter)->name();

		method_count = (*iter)->method_count();
		for (int32_t idx = 0; idx < method_count; ++idx) {
			method_descriptor = (*iter)->method(idx);
			method_name_content = method_descriptor->name();
			request_type_name_content = method_descriptor->input_type()->full_name();
			Replace(request_type_name_content, ".", "::");
			response_type_name_content = method_descriptor->output_type()->full_name();
			Replace(response_type_name_content, ".", "::");
		
			client_method_declare = client_method_declare_template;
			Replace(client_method_declare, METHOD_NAME_MARK, method_name_content);
			Replace(client_method_declare, REQUEST_TYPE_NAME_MARK, request_type_name_content);
			Replace(client_method_declare, RESPONSE_TYPE_NAME_MARK, response_type_name_content);
			client_method_declares_content.append(client_method_declare);

			client_method_implement = client_method_implement_template;
			Replace(client_method_implement, SERVICE_NAME_MARK, service_name_content);
			Replace(client_method_implement, METHOD_NAME_MARK, method_name_content);
			Replace(client_method_implement, REQUEST_TYPE_NAME_MARK, request_type_name_content);
			Replace(client_method_implement, RESPONSE_TYPE_NAME_MARK, response_type_name_content);
			client_method_implements_content.append(client_method_implement);	
		}

		service_client_declare = service_client_declare_template;
		Replace(service_client_declare, SERVICE_NAME_MARK, service_name_content);
		Replace(service_client_declare, CLIENT_METHOD_DECLARES_MARK, client_method_declares_content);
		service_client_declares_content.append(service_client_declare);
	}

	Replace(client_header_file_content, PROTO_NAME_CAPITAL_MARK, proto_name_capital_content);
	Replace(client_header_file_content, INCLUDES_MARK, header_includes_content);
	Replace(client_header_file_content, SERVICE_CLIENT_DECLARES_MARK, service_client_declares_content);

	Replace(client_source_file_content, INCLUDES_MARK, source_includes_content);
	Replace(client_source_file_content, CLIENT_METHOD_IMPLEMENTS_MARK, client_method_implements_content);

	std::string client_header_file_name = proto_name_ + "_client.h";
	if (Pour2File(client_header_file_name, client_header_file_content) == -1) {
		return false;
	}

	std::string client_source_file_name = proto_name_ + "_client.cc";
	if (Pour2File(client_source_file_name, client_source_file_content) == -1) {
		return false;
	}

	return true;
}

bool CodeYield::YieldClientMain() {
	std::string client_main_file_content = client_main_file_template;
	std::string proto_name_content = proto_name_;
	std::string service_name_content;
	std::string request_type_name_content;
	std::string response_type_name_content;
	std::string method_name_content;

	if (service_descriptors_.empty()) {
		return false;
	}	
	std::vector<const google::protobuf::ServiceDescriptor *>::iterator iter = service_descriptors_.begin();
	while (iter != service_descriptors_.end() 
				&& !(*iter)->method_count()) {
		++iter;
	}
	if (iter == service_descriptors_.end()) {
		return false;
	}
	
	service_name_content = (*iter)->name();
	const google::protobuf::MethodDescriptor* method_des = NULL;
	method_des = (*iter)->method(0);	// pick up the first method
	method_name_content = method_des->name();
	request_type_name_content = method_des->input_type()->full_name();
	Replace(request_type_name_content, ".", "::");
	response_type_name_content = method_des->output_type()->full_name();
	Replace(response_type_name_content, ".", "::");
	
	Replace(client_main_file_content, PROTO_NAME_MARK, proto_name_content);
	Replace(client_main_file_content, SERVICE_NAME_MARK, service_name_content);
	Replace(client_main_file_content, REQUEST_TYPE_NAME_MARK, request_type_name_content);
	Replace(client_main_file_content, RESPONSE_TYPE_NAME_MARK, response_type_name_content);
	Replace(client_main_file_content, METHOD_NAME_MARK, method_name_content);

	std::string client_main_file_name = proto_name_ + "_client_main.cc";
	if (Pour2File(client_main_file_name, client_main_file_content) == -1) {
		return false;
	}
	return true;
}

bool CodeYield::YieldMakefile() {
	std::string makefile_content = makefile_template;	
	std::string pink_dir_content;
	
	Replace(makefile_content, PINK_DIR_MARK, MACRO_TO_STR(__PINK_DIR__));
	Replace(makefile_content, PROTO_NAME_MARK, proto_name_);
	
	if (Pour2File("Makefile", makefile_content) == -1) {
		return -1;
	}
	return true;
}

bool CodeYield::CopyDependentFiles() {
	std::vector<std::string>	file_fnames;
	std::vector<std::string> suffixes = {".cc", ".h"};
	std::string rpc_dir = MACRO_TO_STR(__PINK_DIR__) + std::string("/rpc/");
	if (!GetDirFiles(rpc_dir, file_fnames, std::bind(EndsWith, std::placeholders::_1, suffixes))) {
		std::cerr << "GetDirFiles failed in " << rpc_dir << "!" << std::endl;
		return false;
	}

	char buf[MAX_TMP_BUFFER_SIZE];
	size_t nread = 0, nwrite = 0, tmp = 0;
	FILE *file_r = NULL, *file_w = NULL;
	std::string file_name;
	std::vector<std::string>::iterator iter = file_fnames.begin();
	while (iter != file_fnames.end()) {
		file_name = iter->substr(iter->rfind("/") + 1);
		file_r = fopen(iter->c_str(), "r+");
		if (file_r == NULL) {
			fclose(file_r);
			std::cerr << "Open read file: " << *iter << " failed!" << std::endl;
			return false;
		}
		file_w = fopen(file_name.c_str(), "w+");
		if (file_w == NULL) {
			std::cerr << "Open write file: " << file_name << " failed!" << std::endl;			
		}

		while (!feof(file_r)) {
	
			if(!(nread = fread(buf, 1, MAX_TMP_BUFFER_SIZE, file_r))) {
				std::cerr << "Read file: " << *iter << "failed!" << std::endl;
				fclose(file_r);
				fclose(file_w);
				return false;
			}

			nwrite = 0;
			while (nread > nwrite) {
				tmp = fwrite(buf + nwrite, 1, nread - nwrite, file_w);
				if (tmp == 0) {
					std::cerr << "Read file: " << file_name << "failed!" << std::endl;
					fclose(file_r);
					fclose(file_w);
					return false;
				}
				nwrite += tmp;
			}	

		}
		
		fclose(file_r);
		fclose(file_w);
		++iter;
	}


}

}
