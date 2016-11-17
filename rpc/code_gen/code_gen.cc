#include <iostream>

#include "code_yield.h"


static void Usage() {
	std::cerr << "Usage:" << std::endl;
	std::cerr << "	./code_gen proto_file_path" << std::endl;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		Usage();
		return -1;
	}	
	pink::CodeYield code_yield(argv[1]);
	code_yield.ParseServiceFile();
	
	code_yield.YieldServer();
	code_yield.YieldClient();
	code_yield.YieldMakefile();
	code_yield.CopyDependentFiles();
	return 0;
}
