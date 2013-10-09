SUBDIRS = map

bin_PROGRAMS = benchmark

benchmark_LDFLAGS = -L./map 
benchmark_CFLAGS = -std=c99 -I ./map
benchmark_DEPENDENCIES = ./map/liblmn_map.a
benchmark_LDADD = -llmn_map -lpthread

if ENABLE_TCMALLOC
benchmark_LDFLAGS += -L./third_party/google-perftools-1.8.3/.libs
benchmark_DEPENDENCIES += ./third_party/google-perftools-1.8.3/.libs/libtcmalloc_minimal_in_slim.a
benchmark_LDADD += -ltcmalloc_minimal_in_slim
endif

benchmark_SOURCES = main.c
