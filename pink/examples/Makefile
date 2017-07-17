CC=cc
CXX=g++
LDFLAGS= -lpthread -lrt -lprotobuf $(EXTRA_LDFLAGS)
CFLAGS= -DOS_LINUX -fno-builtin-memcmp -msse -msse4.2 ${EXTRA_CFLAGS}
CXXFLAGS=-O2 -std=c++11 -DOS_LINUX -fno-builtin-memcmp -msse -msse4.2 ${EXTRA_CXXFLAGS}

.PHONY: clean all

all: bg_thread http_server mydispatch_srv myholy_srv myholy_srv_chandle myproto_cli \
	redis_cli_test simple_http_server

ifndef SLASH_PATH
  $(warning Warning: missing slash path, using default)
	SLASH_PATH=$(CURDIR)/../third/slash
endif

SLASH_INCLUDE_DIR=$(SLASH_PATH)
SLASH_LIBRARY=$(SLASH_PATH)/slash/lib/libslash.a

CXXFLAGS+=-I../.. \
					-I$(SLASH_INCLUDE_DIR) \

LIBPINK=../lib/libpink.a

bg_thread: bg_thread.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

http_server: http_server.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

https_server: https_server.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

mydispatch_srv: mydispatch_srv.cc myproto.pb.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

myholy_srv: myholy_srv.cc myproto.pb.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

myholy_srv_chandle: myholy_srv_chandle.cc myproto.pb.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

myproto_cli: myproto_cli.cc myproto.pb.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

redis_cli_test: redis_cli_test.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

simple_http_server: simple_http_server.cc $(LIBPINK)
	$(CXX) $(CXXFLAGS) $^ -o$@ $(SLASH_LIBRARY) $(LDFLAGS)

myproto.pb.cc: myproto.proto
	protoc -I=. --cpp_out=. ./$^

clean:
	find . -name "*.[oda]" -exec rm -f {} \;
	rm -rf ./bg_thread ./http_server ./https_server ./mydispatch_srv ./myholy_srv ./myholy_srv_chandle ./myproto_cli \
				./redis_cli_test ./simple_http_server

$(LIBPINK):
	cd .. && $(MAKE) static_lib
