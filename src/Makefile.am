SUBDIRS = map

bin_PROGRAMS = benchmark


benchmark_LDFLAGS = -L./map

benchmark_CFLAGS =  -I ./map

benchmark_DEPENDENCIES = ./map/liblmn_map.a

benchmark_LDADD = -llmn_map

benchmark_SOURCES = main.c
