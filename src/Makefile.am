SUBDIRS = map

bin_PROGRAMS = benchmark

benchmark_LDFLAGS = -L./map  -L./lmntal/hashmap
benchmark_CFLAGS = -I ./map ./lmntal/hashmap
benchmark_DEPENDENCIES = ./map/liblmn_map.a ./lmntal/hashmap/liblmn_hashmap.a
benchmark_LDADD = -llmn_map -llmn_hashmap -lpthread

if ENABLE_TCMALLOC
benchmark_LDFLAGS += -L./third_party/gperftools-2.1/.libs
benchmark_DEPENDENCIES += ./third_party/gperftools-2.1/.libs/libtcmalloc_minimal.a
benchmark_LDADD += -ltcmalloc_minimal
endif

benchmark_SOURCES = main.cc
