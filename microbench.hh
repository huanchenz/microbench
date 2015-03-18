#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <utility>
#include <time.h>
#include <sys/time.h>

#include "allocatortracker.hh"

#include <map>
#include "stx/btree_map.h"
#include "stx/btree.h"
#include "hybrid_index/mtIndexAPI.hh"

#define INIT_LIMIT 50000000
#define LIMIT 10000000
#define KEY_LEN 8
#define KEY_LEN_STR 32

typedef AllocatorTracker<std::pair<const uint64_t, uint64_t> > AllocatorType;
//typedef AllocatorTracker<std::pair<const std::string, uint64_t> > AllocatorType_str;
typedef AllocatorTracker<std::pair<const std::string, uint64_t> > AllocatorType_str;

typedef std::map<uint64_t, uint64_t> MapType;
typedef std::map<uint64_t, uint64_t, std::less<uint64_t>, AllocatorType> MapType_alloc;

typedef std::map<std::string, uint64_t> MapType_str;
typedef std::map<std::string, uint64_t, std::less<std::string>, AllocatorType_str> MapType_str_alloc;

typedef stx::btree_map<uint64_t, uint64_t> BtreeType;
typedef stx::btree_map<uint64_t, uint64_t, std::less<uint64_t>, stx::btree_default_map_traits<uint64_t, uint64_t>, AllocatorType> BtreeType_alloc;

typedef stx::btree_map<std::string, uint64_t> BtreeType_str;
typedef stx::btree_map<std::string, uint64_t, std::less<std::string>, stx::btree_default_map_traits<std::string, uint64_t>, AllocatorType_str> BtreeType_str_alloc;

typedef mt_index<Masstree::default_table> HybridType;

inline double get_now() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}
