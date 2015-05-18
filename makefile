CC = gcc
CXX = g++ -std=gnu++0x
CFLAGS = -g -O2 -fPIC
DEPSDIR := hybrid_index/.deps
DEPCFLAGS = -MD -MF $(DEPSDIR)/$*.d -MP
MEMMGR = -ltcmalloc_minimal

#all: stdmap_a_int stdmap_a_str stdmap_a_url stdmap_c_int stdmap_c_str stdmap_c_url stdmap_e_int stdmap_e_str stdmap_e_url stxbtree_a_int stxbtree_a_str stxbtree_a_url stxbtree_c_int stxbtree_c_str stxbtree_c_url stxbtree_e_int stxbtree_e_str stxbtree_e_url hybrid_a_int hybrid_a_str hybrid_a_url hybrid_c_int hybrid_c_str hybrid_c_url hybrid_c_int_merge hybrid_c_str_merge hybrid_c_url_merge hybrid_e_int hybrid_e_str hybrid_e_url merge_cost_int merge_time_int merge_cost_str merge_time_str merge_cost_url merge_time_url

#all: multimap_a_int hybridmulti_a_int multimap_a_str hybridmulti_a_str multimap_a_url hybridmulti_a_url multimap_c_int hybridmulti_c_int multimap_c_str hybridmulti_c_str multimap_c_url hybridmulti_c_url multimap_e_int hybridmulti_e_int multimap_e_str hybridmulti_e_str multimap_e_url hybridmulti_e_url

all: merge_cost_int merge_cost_str merge_cost_url
#all: skiplist_a_int skiplist_a_str skiplist_a_url skiplist_c_int skiplist_c_str skiplist_c_url skiplist_e_int skiplist_e_str skiplist_e_url
#all: hybrid_a_int hybrid_c_int hybrid_e_int
#all: hybrid_e_url
#all: hybridmulti_e_int hybridmulti_e_str hybridmulti_e_url
#all: stdmap_e_int stxbtree_e_int stdmap_e_str stxbtree_e_str

hiTest.o: hiTest.cc hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hiTest.o hiTest.cc

hiTest: hiTest.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hiTest hiTest.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

# stdmap ycsb A-------------------------------------------------------------------------------------------------------------------------------
stdmap_a_int.o: stdmap_a_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_a_int.o stdmap_a_int.cc

stdmap_a_int: stdmap_a_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_a_int stdmap_a_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stdmap_a_str.o: stdmap_a_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_a_str.o stdmap_a_str.cc

stdmap_a_str: stdmap_a_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_a_str stdmap_a_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stdmap_a_url.o: stdmap_a_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_a_url.o stdmap_a_url.cc

stdmap_a_url: stdmap_a_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_a_url stdmap_a_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# stdmap ycsb C-------------------------------------------------------------------------------------------------------------------------------
stdmap_c_int.o: stdmap_c_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_c_int.o stdmap_c_int.cc

stdmap_c_int: stdmap_c_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_c_int stdmap_c_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stdmap_c_str.o: stdmap_c_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_c_str.o stdmap_c_str.cc

stdmap_c_str: stdmap_c_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_c_str stdmap_c_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stdmap_c_url.o: stdmap_c_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_c_url.o stdmap_c_url.cc

stdmap_c_url: stdmap_c_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_c_url stdmap_c_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# stdmap ycsb E-------------------------------------------------------------------------------------------------------------------------------
stdmap_e_int.o: stdmap_e_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_e_int.o stdmap_e_int.cc

stdmap_e_int: stdmap_e_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_e_int stdmap_e_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stdmap_e_str.o: stdmap_e_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_e_str.o stdmap_e_str.cc

stdmap_e_str: stdmap_e_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_e_str stdmap_e_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stdmap_e_url.o: stdmap_e_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stdmap_e_url.o stdmap_e_url.cc

stdmap_e_url: stdmap_e_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stdmap_e_url stdmap_e_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# stxbtree ycsb A-----------------------------------------------------------------------------------------------------------------------------
stxbtree_a_int.o: stxbtree_a_int.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_a_int.o stxbtree_a_int.cc

stxbtree_a_int: stxbtree_a_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_a_int stxbtree_a_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stxbtree_a_str.o: stxbtree_a_str.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_a_str.o stxbtree_a_str.cc

stxbtree_a_str: stxbtree_a_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_a_str stxbtree_a_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stxbtree_a_url.o: stxbtree_a_url.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_a_url.o stxbtree_a_url.cc

stxbtree_a_url: stxbtree_a_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_a_url stxbtree_a_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# stxbtree ycsb C-----------------------------------------------------------------------------------------------------------------------------
stxbtree_c_int.o: stxbtree_c_int.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_c_int.o stxbtree_c_int.cc

stxbtree_c_int: stxbtree_c_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_c_int stxbtree_c_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stxbtree_c_str.o: stxbtree_c_str.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_c_str.o stxbtree_c_str.cc

stxbtree_c_str: stxbtree_c_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_c_str stxbtree_c_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stxbtree_c_url.o: stxbtree_c_url.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_c_url.o stxbtree_c_url.cc

stxbtree_c_url: stxbtree_c_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_c_url stxbtree_c_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# stxbtree ycsb E-----------------------------------------------------------------------------------------------------------------------------
stxbtree_e_int.o: stxbtree_e_int.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_e_int.o stxbtree_e_int.cc

stxbtree_e_int: stxbtree_e_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_e_int stxbtree_e_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stxbtree_e_str.o: stxbtree_e_str.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_e_str.o stxbtree_e_str.cc

stxbtree_e_str: stxbtree_e_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_e_str stxbtree_e_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

stxbtree_e_url.o: stxbtree_e_url.cc microbench.hh stx/btree.h stx/btree_map.h hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o stxbtree_e_url.o stxbtree_e_url.cc

stxbtree_e_url: stxbtree_e_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o stxbtree_e_url stxbtree_e_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# skiplist ycsb A-----------------------------------------------------------------------------------------------------------------------------
skiplist_a_int.o: skiplist_a_int.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_a_int.o skiplist_a_int.cc

skiplist_a_int: skiplist_a_int.o
	$(CXX) $(CFLAGS) -o skiplist_a_int skiplist_a_int.o $(MEMMGR) -lpthread -lm

skiplist_a_str.o: skiplist_a_str.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_a_str.o skiplist_a_str.cc

skiplist_a_str: skiplist_a_str.o
	$(CXX) $(CFLAGS) -o skiplist_a_str skiplist_a_str.o $(MEMMGR) -lpthread -lm

skiplist_a_url.o: skiplist_a_url.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_a_url.o skiplist_a_url.cc

skiplist_a_url: skiplist_a_url.o
	$(CXX) $(CFLAGS) -o skiplist_a_url skiplist_a_url.o $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# skiplist ycsb C-----------------------------------------------------------------------------------------------------------------------------
skiplist_c_int.o: skiplist_c_int.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_c_int.o skiplist_c_int.cc

skiplist_c_int: skiplist_c_int.o
	$(CXX) $(CFLAGS) -o skiplist_c_int skiplist_c_int.o $(MEMMGR) -lpthread -lm

skiplist_c_str.o: skiplist_c_str.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_c_str.o skiplist_c_str.cc

skiplist_c_str: skiplist_c_str.o
	$(CXX) $(CFLAGS) -o skiplist_c_str skiplist_c_str.o $(MEMMGR) -lpthread -lm

skiplist_c_url.o: skiplist_c_url.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_c_url.o skiplist_c_url.cc

skiplist_c_url: skiplist_c_url.o
	$(CXX) $(CFLAGS) -o skiplist_c_url skiplist_c_url.o $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# skiplist ycsb E-----------------------------------------------------------------------------------------------------------------------------
skiplist_e_int.o: skiplist_e_int.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_e_int.o skiplist_e_int.cc

skiplist_e_int: skiplist_e_int.o
	$(CXX) $(CFLAGS) -o skiplist_e_int skiplist_e_int.o $(MEMMGR) -lpthread -lm

skiplist_e_str.o: skiplist_e_str.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_e_str.o skiplist_e_str.cc

skiplist_e_str: skiplist_e_str.o
	$(CXX) $(CFLAGS) -o skiplist_e_str skiplist_e_str.o $(MEMMGR) -lpthread -lm

skiplist_e_url.o: skiplist_e_url.cc microbench.hh
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o skiplist_e_url.o skiplist_e_url.cc

skiplist_e_url: skiplist_e_url.o
	$(CXX) $(CFLAGS) -o skiplist_e_url skiplist_e_url.o $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------


# hybrid ycsb A-------------------------------------------------------------------------------------------------------------------------------
hybrid_a_int.o: hybrid_a_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_a_int.o hybrid_a_int.cc

hybrid_a_int: hybrid_a_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_a_int hybrid_a_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_a_str.o: hybrid_a_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_a_str.o hybrid_a_str.cc

hybrid_a_str: hybrid_a_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_a_str hybrid_a_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_a_url.o: hybrid_a_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_a_url.o hybrid_a_url.cc

hybrid_a_url: hybrid_a_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_a_url hybrid_a_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# hybrid ycsb C-------------------------------------------------------------------------------------------------------------------------------
hybrid_c_int.o: hybrid_c_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_c_int.o hybrid_c_int.cc

hybrid_c_int: hybrid_c_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_c_int hybrid_c_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_c_str.o: hybrid_c_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_c_str.o hybrid_c_str.cc

hybrid_c_str: hybrid_c_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_c_str hybrid_c_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_c_url.o: hybrid_c_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_c_url.o hybrid_c_url.cc

hybrid_c_url: hybrid_c_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_c_url hybrid_c_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_c_int_merge.o: hybrid_c_int_merge.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_c_int_merge.o hybrid_c_int_merge.cc

hybrid_c_int_merge: hybrid_c_int_merge.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_c_int_merge hybrid_c_int_merge.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_c_str_merge.o: hybrid_c_str_merge.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_c_str_merge.o hybrid_c_str_merge.cc

hybrid_c_str_merge: hybrid_c_str_merge.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_c_str_merge hybrid_c_str_merge.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_c_url_merge.o: hybrid_c_url_merge.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_c_url_merge.o hybrid_c_url_merge.cc

hybrid_c_url_merge: hybrid_c_url_merge.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_c_url_merge hybrid_c_url_merge.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# hybrid ycsb E-------------------------------------------------------------------------------------------------------------------------------
hybrid_e_int.o: hybrid_e_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_e_int.o hybrid_e_int.cc

hybrid_e_int: hybrid_e_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_e_int hybrid_e_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_e_str.o: hybrid_e_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_e_str.o hybrid_e_str.cc

hybrid_e_str: hybrid_e_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_e_str hybrid_e_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybrid_e_url.o: hybrid_e_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybrid_e_url.o hybrid_e_url.cc

hybrid_e_url: hybrid_e_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybrid_e_url hybrid_e_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

# cost of merge-------------------------------------------------------------------------------------------------------------------------------
merge_cost_int.o: merge_cost_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_cost_int.o merge_cost_int.cc

merge_cost_int: merge_cost_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_cost_int merge_cost_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

merge_time_int.o: merge_time_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_time_int.o merge_time_int.cc

merge_time_int: merge_time_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_time_int merge_time_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

merge_cost_str.o: merge_cost_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_cost_str.o merge_cost_str.cc

merge_cost_str: merge_cost_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_cost_str merge_cost_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

merge_time_str.o: merge_time_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_time_str.o merge_time_str.cc

merge_time_str: merge_time_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_time_str merge_time_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

merge_cost_url.o: merge_cost_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_cost_url.o merge_cost_url.cc

merge_cost_url: merge_cost_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_cost_url merge_cost_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

merge_time_url.o: merge_time_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_time_url.o merge_time_url.cc

merge_time_url: merge_time_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_time_url merge_time_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------


# multimap ycsb A-----------------------------------------------------------------------------------------------------------------------------
multimap_a_int.o: multimap_a_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_a_int.o multimap_a_int.cc

multimap_a_int: multimap_a_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_a_int multimap_a_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

multimap_a_str.o: multimap_a_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_a_str.o multimap_a_str.cc

multimap_a_str: multimap_a_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_a_str multimap_a_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

multimap_a_url.o: multimap_a_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_a_url.o multimap_a_url.cc

multimap_a_url: multimap_a_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_a_url multimap_a_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

# multimap ycsb C-----------------------------------------------------------------------------------------------------------------------------
multimap_c_int.o: multimap_c_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_c_int.o multimap_c_int.cc

multimap_c_int: multimap_c_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_c_int multimap_c_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

multimap_c_str.o: multimap_c_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_c_str.o multimap_c_str.cc

multimap_c_str: multimap_c_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_c_str multimap_c_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

multimap_c_url.o: multimap_c_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_c_url.o multimap_c_url.cc

multimap_c_url: multimap_c_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_c_url multimap_c_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

# multimap ycsb E-----------------------------------------------------------------------------------------------------------------------------
multimap_e_int.o: multimap_e_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_e_int.o multimap_e_int.cc

multimap_e_int: multimap_e_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_e_int multimap_e_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

multimap_e_str.o: multimap_e_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_e_str.o multimap_e_str.cc

multimap_e_str: multimap_e_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_e_str multimap_e_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

multimap_e_url.o: multimap_e_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o multimap_e_url.o multimap_e_url.cc

multimap_e_url: multimap_e_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o multimap_e_url multimap_e_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

# hybridmulti ycsb A--------------------------------------------------------------------------------------------------------------------------
hybridmulti_a_int.o: hybridmulti_a_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_a_int.o hybridmulti_a_int.cc

hybridmulti_a_int: hybridmulti_a_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_a_int hybridmulti_a_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybridmulti_a_str.o: hybridmulti_a_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_a_str.o hybridmulti_a_str.cc

hybridmulti_a_str: hybridmulti_a_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_a_str hybridmulti_a_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybridmulti_a_url.o: hybridmulti_a_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_a_url.o hybridmulti_a_url.cc

hybridmulti_a_url: hybridmulti_a_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_a_url hybridmulti_a_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

# hybridmulti ycsb C--------------------------------------------------------------------------------------------------------------------------
hybridmulti_c_int.o: hybridmulti_c_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_c_int.o hybridmulti_c_int.cc

hybridmulti_c_int: hybridmulti_c_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_c_int hybridmulti_c_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybridmulti_c_str.o: hybridmulti_c_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_c_str.o hybridmulti_c_str.cc

hybridmulti_c_str: hybridmulti_c_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_c_str hybridmulti_c_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybridmulti_c_url.o: hybridmulti_c_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_c_url.o hybridmulti_c_url.cc

hybridmulti_c_url: hybridmulti_c_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_c_url hybridmulti_c_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

# hybridmulti ycsb E--------------------------------------------------------------------------------------------------------------------------
hybridmulti_e_int.o: hybridmulti_e_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_e_int.o hybridmulti_e_int.cc

hybridmulti_e_int: hybridmulti_e_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_e_int hybridmulti_e_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybridmulti_e_str.o: hybridmulti_e_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_e_str.o hybridmulti_e_str.cc

hybridmulti_e_str: hybridmulti_e_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_e_str hybridmulti_e_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

hybridmulti_e_url.o: hybridmulti_e_url.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o hybridmulti_e_url.o hybridmulti_e_url.cc

hybridmulti_e_url: hybridmulti_e_url.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o hybridmulti_e_url hybridmulti_e_url.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm


clean:
	$(RM) stdmap_a_int stdmap_a_str stdmap_a_url stdmap_c_int stdmap_c_str stdmap_c_url stdmap_e_int stdmap_e_str stdmap_e_url stxbtree_a_int stxbtree_a_str stxbtree_a_url stxbtree_c_int stxbtree_c_str stxbtree_c_url stxbtree_e_int stxbtree_e_str stxbtree_e_url hybrid_a_int hybrid_a_str hybrid_a_url hybrid_c_int hybrid_c_str hybrid_c_url hybrid_c_int_merge hybrid_c_str_merge hybrid_c_url_merge hybrid_e_int hybrid_e_str hybrid_e_url merge_cost_int merge_time_int merge_cost_str merge_time_str merge_cost_url merge_time_url skiplist_a_int skiplist_a_str skiplist_a_url skiplist_c_int skiplist_c_str skiplist_c_url skiplist_e_int skiplist_e_str skiplist_e_url multimap_a_int hybridmulti_a_int multimap_a_str hybridmulti_a_str multimap_a_url hybridmulti_a_url multimap_c_int hybridmulti_c_int multimap_c_str hybridmulti_c_str multimap_c_url hybridmulti_c_url multimap_e_int hybridmulti_e_int multimap_e_str hybridmulti_e_str multimap_e_url hybridmulti_e_url *.o *~
