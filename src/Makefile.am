SUBDIRS = map lmntal/concurrent

bin_PROGRAMS = benchmark

benchmark_CFLAGS = $(CFLAG) -std=c99 -Wall -fpermissive
benchmark_LDFLAGS = -L./map  -L./lmntal/concurrent
benchmark_DEPENDENCIES = ./map/liblmn_map.a ./lmntal/concurrent/liblmn_concurrent.a 

benchmark_LDADD = -llmn_map -llmn_concurrent -lpthread

if ENABLE_TCMALLOC
benchmark_LDFLAGS += -L./third_party/gperftools-2.1/.libs
benchmark_DEPENDENCIES += ./third_party/gperftools-2.1/.libs/libtcmalloc_minimal.a
benchmark_LDADD += -ltcmalloc_minimal
endif

benchmark_SOURCES = main.cc
