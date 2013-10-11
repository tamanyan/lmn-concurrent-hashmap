SUBDIRS = map

bin_PROGRAMS = benchmark

benchmark_LDFLAGS = -L./map 
benchmark_CFLAGS = -I ./map
benchmark_DEPENDENCIES = ./map/liblmn_map.a
benchmark_LDADD = -llmn_map -lpthread

if ENABLE_TCMALLOC
benchmark_LDFLAGS += -L./third_party/gperftools-2.1/.libs
benchmark_DEPENDENCIES += ./third_party/gperftools-2.1/.libs/libtcmalloc_minimal.a
benchmark_LDADD += -ltcmalloc_minimal
endif

benchmark_SOURCES = main.cc
