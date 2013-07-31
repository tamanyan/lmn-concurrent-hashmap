SUBDIRS = map

bin_PROGRAMS = benchmark


benchmark_LDFLAGS = -L./map

benchmark_CFLAGS =  -I ./map

benchmark_DEPENDENCIES = ./map/liblmn_hashmap.a

benchmark_LDADD = -llmn_hashmap

benchmark_SOURCES = main.c
