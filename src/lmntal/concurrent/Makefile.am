noinst_LIBRARIES = liblmn_concurrent.a

liblmn_concurrent_a_CFLAGS = $(CFLAGS) -I ./hashmap
liblmn_concurrent_a_CXXFLAGS = $(CXXFLAG) -fpermissive

liblmn_concurrent_a_SOURCES = \
							 thread.cc thread.h \
						   hashmap/hashmap.cc hashmap/hashmap.h \
						   hashmap/cc_hashmap.cc hashmap/cc_hashmap.h \
							 hashmap/chain_hashmap.cc hashmap/chain_hashmap.h \
							 hashmap/lf_chain_hashmap.cc hashmap/lf_chain_hashmap.h
