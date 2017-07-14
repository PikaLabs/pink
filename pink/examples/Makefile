include ../make_config.mk

.PHONY: clean

all: bg_thread http_server mydispatch_srv myholy_srv myholy_srv_chandle myproto_cli \
	redis_cli_test simple_http_server

ifndef SLASH_PATH
  $(warning Warning: slash path required, using defaut path)
  SLASH_PATH = ../../../slash
endif

LIBSLASH =$(SLASH_PATH)/slash/lib/libslash.a
LIBPINK =../lib/libpink.a

bg_thread: bg_thread.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

http_server: http_server.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@  -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

https_server: https_server.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

mydispatch_srv: mydispatch_srv.cc myproto.pb.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

myholy_srv: myholy_srv.cc myproto.pb.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

myholy_srv_chandle: myholy_srv_chandle.cc myproto.pb.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

myproto_cli: myproto_cli.cc myproto.pb.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

redis_cli_test: redis_cli_test.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

simple_http_server: simple_http_server.cc $(LIBPINK) $(LIBSLASH)
	$(CXX) $(CXXFLAGS) $^ -o$@ -I../.. -I$(SLASH_PATH) -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

myproto.pb.cc: myproto.proto
	protoc -I=. --cpp_out=. ./$^

clean:
	find . -name "*.[oda]" -exec rm -f {} \;
	rm -rf ./bg_thread ./http_server ./https_server ./mydispatch_srv ./myholy_srv ./myholy_srv_chandle ./myproto_cli \
				./redis_cli_test ./simple_http_server
