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
//#include "skip_list/random_access_skip_list.h"
#include "skip_list/skip_list.h"
#include "hybrid_index/mtIndexAPI.hh"

//#define INIT_LIMIT 1000000
//#define LIMIT 1000000
#define INIT_LIMIT 50000000
#define LIMIT 10000000
#define KEY_LEN 8
#define KEY_LEN_STR 32
#define KEY_LEN_URL 1000
#define VALUES_PER_KEY 10

#define HYBRID 1

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



template <std::size_t keySize>
class GenericKey {
public:
  inline void setFromString(std::string key) {
    memset(data, 0, keySize);
    strcpy(data, key.c_str());
  }

  char data[keySize];

private:

};

template <std::size_t keySize>
class GenericComparator {
public:
  GenericComparator() {}

  inline bool operator()(const GenericKey<keySize> &lhs, const GenericKey<keySize> &rhs) const {
    int diff = strcmp(lhs.data, rhs.data);
    return diff < 0;
    //return strcmp(lhs.data, rhs.data);
  }

private:

};


template <std::size_t keySize>
class GenericKeyValue {
public:
  inline void setKeyFromString(std::string key_str) {
    memset(key, 0, keySize);
    strcpy(key, key_str.c_str());
  }

  char key[keySize];
  uint64_t value;

private:

};

template <std::size_t keySize>
class GenericKeyValueComparator {
public:
  GenericKeyValueComparator() {}

  inline bool operator()(const GenericKeyValue<keySize> &lhs, const GenericKeyValue<keySize> &rhs) const {
    int diff = strcmp(lhs.key, rhs.key);
    return diff < 0;
    //return strcmp(lhs.data, rhs.data);
  }

private:

};


class StringKeyValue {
public:
  std::string key;
  uint64_t value;

private:

};

class StringKeyValueComparator {
public:
  StringKeyValueComparator() {}

  inline bool operator()(const StringKeyValue &lhs, const StringKeyValue &rhs) const {
    int diff = strcmp(lhs.key.c_str(), rhs.key.c_str());
    return diff < 0;
    //return strcmp(lhs.data, rhs.data);
  }

private:

};


class IntsKeyValue {
public:
  uint64_t key;
  uint64_t value;

private:

};

class IntsKeyValueComparator {
public:
  IntsKeyValueComparator() {}

  inline bool operator()(const IntsKeyValue &lhs, const IntsKeyValue &rhs) const {
    return (lhs.key < rhs.key);
  }

private:

};


typedef AllocatorTracker<std::pair<const GenericKey<21>, uint64_t> > AllocatorType_gen;

typedef std::map<GenericKey<21>, uint64_t, GenericComparator<21> > MapType_gen;
typedef std::map<GenericKey<21>, uint64_t, GenericComparator<21>, AllocatorType_gen> MapType_gen_alloc;
typedef stx::btree_map<GenericKey<21>, uint64_t, GenericComparator<21> > BtreeType_gen;
typedef stx::btree_map<GenericKey<21>, uint64_t, GenericComparator<21>, stx::btree_default_map_traits<GenericKey<21>, uint64_t>, AllocatorType_gen> BtreeType_gen_alloc;


typedef std::multimap<uint64_t, uint64_t> MapType_multi;
typedef std::multimap<uint64_t, uint64_t, std::less<uint64_t>, AllocatorType> MapType_multi_alloc;

typedef std::multimap<std::string, uint64_t> MapType_multi_str;
typedef std::multimap<std::string, uint64_t, std::less<std::string>, AllocatorType_str> MapType_multi_str_alloc;

typedef std::multimap<GenericKey<21>, uint64_t, GenericComparator<21> > MapType_multi_gen;
typedef std::multimap<GenericKey<21>, uint64_t, GenericComparator<21>, AllocatorType> MapType_multi_gen_alloc;


//typedef AllocatorTracker<std::string> AllocatorType_skiplist;
//typedef goodliffe::random_access_skip_list<std::string, std::less<std::string>, AllocatorType_skiplist> SkipListType_str_alloc;

//typedef goodliffe::random_access_skip_list<IntsKeyValue, IntsKeyValueComparator> SkipListType_int;
//typedef goodliffe::random_access_skip_list<GenericKeyValue<21>, GenericKeyValueComparator<21> > SkipListType_str;

typedef goodliffe::skip_list<IntsKeyValue, IntsKeyValueComparator> SkipListType_int;
typedef goodliffe::skip_list<GenericKeyValue<21>, GenericKeyValueComparator<21> > SkipListType_str;
typedef goodliffe::skip_list<StringKeyValue, StringKeyValueComparator> SkipListType_string;

//typedef AllocatorTracker<IntsKeyValue> AllocatorType_skiplist_ints;
//typedef goodliffe::random_access_skip_list<IntsKeyValue, IntsKeyValueComparator, AllocatorType_skiplist_ints> SkipListType_int_alloc;

typedef AllocatorTracker<IntsKeyValue> AllocatorType_skiplist_ints;
typedef goodliffe::skip_list<IntsKeyValue, IntsKeyValueComparator, AllocatorType_skiplist_ints> SkipListType_int_alloc;

//typedef AllocatorTracker<GenericKeyValue<21> > AllocatorType_skiplist;
//typedef goodliffe::random_access_skip_list<GenericKeyValue<21>, GenericKeyValueComparator<21>, AllocatorType_skiplist> SkipListType_str_alloc;

typedef AllocatorTracker<GenericKeyValue<21> > AllocatorType_skiplist;
typedef goodliffe::skip_list<GenericKeyValue<21>, GenericKeyValueComparator<21>, AllocatorType_skiplist> SkipListType_str_alloc;

typedef AllocatorTracker<StringKeyValue> AllocatorType_skiplist_string;
typedef goodliffe::skip_list<StringKeyValue, StringKeyValueComparator, AllocatorType_skiplist_string> SkipListType_string_alloc;
