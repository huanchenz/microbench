CC = gcc
CXX = g++ -std=gnu++0x
CFLAGS = -g -O2 -fPIC
DEPSDIR := hybrid_index/.deps
DEPCFLAGS = -MD -MF $(DEPSDIR)/$*.d -MP
MEMMGR = -ltcmalloc_minimal

all: stdmap_a_int stdmap_a_str stdmap_c_int stdmap_c_str stdmap_c_url stdmap_e_int stdmap_e_str stxbtree_a_int stxbtree_a_str stxbtree_c_int stxbtree_c_str stxbtree_c_url stxbtree_e_int stxbtree_e_str hybrid_a_int hybrid_a_str hybrid_c_int hybrid_c_str hybrid_c_url hybrid_e_int hybrid_e_str merge_cost_int merge_cost_str

#all: hybrid_c_url
#all: stdmap_c_int

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
# --------------------------------------------------------------------------------------------------------------------------------------------

# cost of merge-------------------------------------------------------------------------------------------------------------------------------
merge_cost_int.o: merge_cost_int.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_cost_int.o merge_cost_int.cc

merge_cost_int: merge_cost_int.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_cost_int merge_cost_int.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm

merge_cost_str.o: merge_cost_str.cc microbench.hh hybrid_index/config.h $(DEPSDIR)/stamp
	$(CXX) $(CFLAGS) $(DEPCFLAGS) -include hybrid_index/config.h -c -o merge_cost_str.o merge_cost_str.cc

merge_cost_str: merge_cost_str.o hybrid_index/mtIndexAPI.a
	$(CXX) $(CFLAGS) -o merge_cost_str merge_cost_str.o hybrid_index/mtIndexAPI.a $(MEMMGR) -lpthread -lm
# --------------------------------------------------------------------------------------------------------------------------------------------

clean:
	$(RM) stdmap_a_int stdmap_a_str stdmap_c_int stdmap_c_str stdmap_c_url stdmap_e_int stdmap_e_str stxbtree_a_int stxbtree_a_str stxbtree_c_int stxbtree_c_str stxbtree_c_url stxbtree_e_int stxbtree_e_str hybrid_a_int hybrid_a_str hybrid_c_int hybrid_c_str hybrid_c_url hybrid_e_int hybrid_e_str merge_cost_int merge_cost_str *.o *~
