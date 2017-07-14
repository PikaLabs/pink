LIB_SOURCES = \
	src/bg_thread.cc \
	src/build_version.cc \
	src/dispatch_thread.cc \
	src/holy_thread.cc \
	src/http_conn.cc \
	src/pb_cli.cc \
	src/pb_conn.cc \
	src/period_thread.cc \
	src/pink_cli.cc \
	src/pink_conn.cc \
	src/pink_epoll.cc \
	src/pink_item.cc \
	src/pink_thread.cc \
	src/pink_util.cc \
	src/redis_cli.cc \
	src/redis_conn.cc \
	src/server_socket.cc \
	src/server_thread.cc \
	src/simple_http_conn.cc \
	src/worker_thread.cc \

EXAMPLE_SOURCE = \
	examples/bg_thread.cc \
	examples/http_server.cc \
	examples/https_server.cc \
	examples/mydispatch_srv.cc \
	examples/myholy_srv.cc \
	examples/myholy_srv_chandle.cc \
	examples/myproto_cli.cc \
	examples/redis_cli_test.cc \
	examples/simple_http_server.cc \
