#ifndef MTINDEXAPI_H
#define MTINDEXAPI_H

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <limits.h>
#if HAVE_NUMA_H
#include <numa.h>
#endif
#if HAVE_SYS_EPOLL_H
#include <sys/epoll.h>
#endif
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#if __linux__
#include <asm-generic/mman.h>
#endif
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#ifdef __linux__
#include <malloc.h>
#endif
#include "nodeversion.hh"
#include "kvstats.hh"
#include "query_masstree.hh"
#include "masstree_tcursor.hh"
#include "masstree_insert.hh"
#include "masstree_remove.hh"
#include "masstree_scan.hh"
#include "timestamp.hh"
#include "json.hh"
#include "kvtest.hh"
#include "kvrandom.hh"
#include "kvrow.hh"
#include "kvio.hh"
#include "clp.h"
#include <algorithm>
#include <numeric>

#include <stdint.h>
#include "config.h"

#define GC_THRESHOLD 1000000
#define MERGE 0
#define MERGE_THRESHOLD 100
#define MERGE_RATIO 10
#define VALUE_LEN 8

#define USE_BLOOM_FILTER 0
#define LITTLEENDIAN 1
#define BITS_PER_KEY 8
#define K 2

#define SECONDARY_INDEX_TYPE 1

template <typename T>
class mt_index {
public:
  mt_index() {}
  ~mt_index() {
    table_->destroy(*ti_);
    delete table_;
    ti_->rcu_clean();
    ti_->deallocate_ti();
    free(ti_);

    if (multivalue_) {
      if (SECONDARY_INDEX_TYPE == 0)
	q_[0].run_destroy_static_multivalue(static_table_->table(), *sti_);
      else if (SECONDARY_INDEX_TYPE == 1)
	q_[0].run_destroy_static_dynamicvalue(static_table_->table(), *sti_);
    }
    else
      q_[0].run_destroy_static(static_table_->table(), *sti_);
    static_table_->destroy(*sti_);
    delete static_table_;
    sti_->rcu_clean();
    sti_->deallocate_ti();
    free(sti_);

    if (cur_key_)
      free(cur_key_);
    if (static_cur_key_)
      free(static_cur_key_);
    if (next_key_)
      free(next_key_);
    if (static_next_key_)
      free(static_next_key_);

    //bloom filter
    if (USE_BLOOM_FILTER)
      free(bloom_filter);
  }

  //#####################################################################################
  // Initialize
  //#####################################################################################
  unsigned long long rdtsc_timer() {
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
  }

  inline void setup() {
    //ti_->rcu_start();
    ti_ = threadinfo::make(threadinfo::TI_MAIN, -1);
    //ti_->run();
    table_ = new T;
    table_->initialize(*ti_);

    sti_ = threadinfo::make(threadinfo::TI_MAIN, -1);
    static_table_ = new T;
    static_table_->initialize(*sti_);

    ic = 0;
    sic = 0;

    srand(rdtsc_timer());
    //merge_ratio = MERGE_RATIO + ((rand() % 100) * 0.1);
    merge_ratio = MERGE_RATIO;
    //std::cout << "merge_ratio = " << merge_ratio << "\n";

    //bloom filter
    if (USE_BLOOM_FILTER)
      bloom_filter = CreateEmptyFilter(MERGE_THRESHOLD);
    else
      bits = 0;
  }

  void setup(int keysize, bool multivalue) {
    setup();

    cur_key_ = NULL;
    cur_keylen_ = 0;
    next_key_ = NULL;
    next_keylen_ = 0;
    static_cur_key_ = NULL;
    static_cur_keylen_ = 0;
    static_next_key_ = NULL;
    static_next_keylen_ = 0;

    key_size_ = keysize;
    multivalue_ = multivalue;
  }

  void setup(int keysize, int keyLen, bool multivalue) {
    setup();

    cur_key_ = (char*)malloc(keyLen * 2);
    cur_keylen_ = 0;
    next_key_ = (char*)malloc(keyLen * 2);
    next_keylen_ = 0;
    static_cur_key_ = (char*)malloc(keyLen * 2);
    static_cur_keylen_ = 0;
    static_next_key_ = (char*)malloc(keyLen * 2);
    static_next_keylen_ = 0;

    key_size_ = keysize;
    multivalue_ = multivalue;
  }


  //#####################################################################################
  // Garbage Collection
  //#####################################################################################
  inline void clean_rcu() {
    ti_->rcu_quiesce();
  }

  inline void static_clean_rcu() {
    sti_->rcu_quiesce();
  }

  inline void gc_dynamic() {
    if (ti_->limbo >= GC_THRESHOLD) {
      clean_rcu();
      ti_->dealloc_rcu += ti_->limbo;
      ti_->limbo = 0;
    }
  }

  inline void gc_static() {
    if (sti_->limbo >= GC_THRESHOLD) {
      static_clean_rcu();
      sti_->dealloc_rcu += sti_->limbo;
      sti_->limbo = 0;
    }
  }

  inline void reset() {
    if (multivalue_) {
      if (SECONDARY_INDEX_TYPE == 0)
	table_->destroy(*ti_);
      else if (SECONDARY_INDEX_TYPE == 1)
	table_->destroy_novalue(*ti_);
    }
    else {
      table_->destroy_novalue(*ti_);
    }
    delete table_;
    gc_dynamic();
    table_ = new T;
    table_->initialize(*ti_);
    ic = 0;
  }

  //#####################################################################################  
  //Bloom Filter
  //#####################################################################################
  inline uint32_t DecodeFixed32(const char* ptr) {
    if (LITTLEENDIAN) {
      // Load the raw bytes
      uint32_t result;
      memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
      return result;
    } else {
      return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
	      | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
	      | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
	      | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
    }
  }

  uint32_t Hash(const char* data, size_t n, uint32_t seed) {
    // Similar to murmur hash
    const uint32_t m = 0xc6a4a793;
    const uint32_t r = 24;
    const char* limit = data + n;
    uint32_t h = seed ^ (n * m);

    // Pick up four bytes at a time
    while (data + 4 <= limit) {
      uint32_t w = DecodeFixed32(data);
      data += 4;
      h += w;
      h *= m;
      h ^= (h >> 16);
    }

    // Pick up remaining bytes
    switch (limit - data) {
    case 3:
      h += static_cast<unsigned char>(data[2]) << 16;
      //FALLTHROUGH_INTENDED;
    case 2:
      h += static_cast<unsigned char>(data[1]) << 8;
      //FALLTHROUGH_INTENDED;
    case 1:
      h += static_cast<unsigned char>(data[0]);
      h *= m;
      h ^= (h >> r);
      break;
    }
    return h;
  }

  uint32_t BloomHash(const char* data, size_t n) {
    return Hash(data, n, 0xbc9f1d34);
  }

  char* CreateEmptyFilter(int n) {
    bits = n * BITS_PER_KEY;
    size_t bytes = (bits + 7) / 8;
    bits = bytes * 8;

    char* array = (char*)malloc(bytes);
    memset((void*)array, '\0', bytes);
    return array;
  }

  void InsertToFilter(const char* data, size_t n, char* filter) {
    uint32_t h = BloomHash(data, n);
    const uint32_t delta = (h >> 17) | (h << 15);
    for (size_t j = 0; j < K; j++) {
      const uint32_t bitpos = h% bits;
      filter[bitpos/8] |= (1 << (bitpos % 8));
      h += delta;
    }
  }

  bool KeyMayMatch(const char* data, size_t n, char* filter) {
    uint32_t h = BloomHash(data, n);
    const uint32_t delta = (h >> 17) | (h << 15);
    for (size_t j = 0; j < K; j++) {
      const uint32_t bitpos = h % bits;
      if ((filter[bitpos/8] & (1 << (bitpos % 8))) == 0)
	return false;
      h += delta;
    }
    return true;
  }

  //#####################################################################################
  //Insert Unique
  //#####################################################################################
  inline bool put_uv(const Str &key, const Str &value) {
    if (sic != 0)
      if (static_exist(key.s, key.len))
	return false;
    lp_mt_l.setup_cursor(table_->table(), key);
    bool found = lp_mt_l.find_insert(*ti_);
    if (!found)
      ti_->advance_timestamp(lp_mt_l.node_timestamp());
    else {
      lp_mt_l.finish(1, *ti_);
      return false;
    }
    qtimes_.ts = ti_->update_timestamp();
    qtimes_.prev_ts = 0;
    lp_mt_l.value() = row_type::create1(value, qtimes_.ts, *ti_);
    lp_mt_l.finish(1, *ti_);
    ic++;

    //bloom filter
    if (USE_BLOOM_FILTER)
      InsertToFilter(key.s, key.len, bloom_filter);

    if ((MERGE == 1) && ((ic * merge_ratio) >= sic) && (ic >= MERGE_THRESHOLD))
      return merge_uv();
    return true;
  }
  bool put_uv(const char *key, int keylen, const char *value, int valuelen) {
    return put_uv(Str(key, keylen), Str(value, valuelen));
  }


  //#####################################################################################
  // Upsert
  //#####################################################################################
  inline void put(const Str &key, const Str &value) {
    lp_mt_l.setup_cursor(table_->table(), key);
    bool found = lp_mt_l.find_insert(*ti_);
    if (!found) {
      ti_->advance_timestamp(lp_mt_l.node_timestamp());
      qtimes_.ts = ti_->update_timestamp();
      qtimes_.prev_ts = 0;

      //bloom filter
      if (USE_BLOOM_FILTER)
	InsertToFilter(key.s, key.len, bloom_filter);
    }
    else {
      qtimes_.ts = ti_->update_timestamp(lp_mt_l.value()->timestamp());
      qtimes_.prev_ts = lp_mt_l.value()->timestamp();
      lp_mt_l.value()->deallocate_rcu(*ti_);
    }
    lp_mt_l.value() = row_type::create1(value, qtimes_.ts, *ti_);
    lp_mt_l.finish(1, *ti_);
    //ic++;
    ic += (value.len/VALUE_LEN);
  }
  void put(const char *key, int keylen, const char *value, int valuelen) {
    return put(Str(key, keylen), Str(value, valuelen));
  }


  //#####################################################################################
  // Insert (multi value)
  //#####################################################################################
  inline void put_nuv0(const Str &key, const Str &value) {
    lp_mt_l.setup_cursor(table_->table(), key);
    bool found = lp_mt_l.find_insert(*ti_);
    if (!found)
      ti_->advance_timestamp(lp_mt_l.node_timestamp());
    char *put_value_string;
    int put_value_len;
    if (!found) {
      qtimes_.ts = ti_->update_timestamp();
      qtimes_.prev_ts = 0;
      lp_mt_l.value() = row_type::create1(value, qtimes_.ts, *ti_);

      //bloom filter
      if (USE_BLOOM_FILTER)
	InsertToFilter(key.s, key.len, bloom_filter);
    }
    else {
      qtimes_.ts = ti_->update_timestamp(lp_mt_l.value()->timestamp());
      qtimes_.prev_ts = lp_mt_l.value()->timestamp();
      put_value_len = value.len + lp_mt_l.value()->col(0).len;
      put_value_string = (char*)malloc(put_value_len);
      memcpy(put_value_string, value.s, value.len);
      memcpy(put_value_string + value.len, lp_mt_l.value()->col(0).s, lp_mt_l.value()->col(0).len);
      lp_mt_l.value()->deallocate_rcu(*ti_);
      lp_mt_l.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *ti_);
      free(put_value_string);
    }
    lp_mt_l.finish(1, *ti_);
    //ic++;
    ic += (value.len/VALUE_LEN);

    if ((MERGE == 1) && ((ic * merge_ratio) >= sic) && (ic >= MERGE_THRESHOLD))
      merge_nuv();
  }
  void put_nuv0(const char *key, int keylen, const char *value, int valuelen) {
    put_nuv0(Str(key, keylen), Str(value, valuelen));
  }


  inline void put_nuv1(const Str &key, const Str &value) {
    lp_mt_u.setup_cursor(table_->table(), key);
    bool found = false;
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic != 0 && KeyMayMatch(key.s, key.len, bloom_filter))
	found = lp_mt_u.find_unlocked(*ti_);
    }
    else {
      if (ic != 0)
	found = lp_mt_u.find_unlocked(*ti_);
    }

    char *put_value_string;
    int put_value_len;
    // if NOT found in dynamic, search static
    if ((!found) && (sic != 0)) {
      lp_d.setup_cursor(static_table_->table(), key);
      bool found_s = lp_d.find();
      // if found in static, update the values and return
      if (found_s) {
	put_value_len = value.len + lp_d.value()->col(0).len;
	put_value_string = (char*)malloc(put_value_len);
	memcpy(put_value_string, value.s, value.len);
	memcpy(put_value_string + value.len, lp_d.value()->col(0).s, lp_d.value()->col(0).len);
	lp_d.value()->deallocate_rcu(*ti_);
	lp_d.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *ti_);
	free(put_value_string);
	sic += (value.len/VALUE_LEN);
	return;
      }
    }

    lp_mt_l.setup_cursor(table_->table(), key);
    found = lp_mt_l.find_insert(*ti_);
    // if NOT found in either dynamic or static, insert a new entry to dynamic
    if (!found) {
      ti_->advance_timestamp(lp_mt_l.node_timestamp());
      qtimes_.ts = ti_->update_timestamp();
      qtimes_.prev_ts = 0;
      lp_mt_l.value() = row_type::create1(value, qtimes_.ts, *ti_);

      //bloom filter
      if (USE_BLOOM_FILTER)
	InsertToFilter(key.s, key.len, bloom_filter);
    }
    // if found in dynamic, update the values
    else {
      qtimes_.ts = ti_->update_timestamp(lp_mt_l.value()->timestamp());
      qtimes_.prev_ts = lp_mt_l.value()->timestamp();
      put_value_len = value.len + lp_mt_l.value()->col(0).len;
      put_value_string = (char*)malloc(put_value_len);
      memcpy(put_value_string, value.s, value.len);
      memcpy(put_value_string + value.len, lp_mt_l.value()->col(0).s, lp_mt_l.value()->col(0).len);
      lp_mt_l.value()->deallocate_rcu(*ti_);
      lp_mt_l.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *ti_);
      free(put_value_string);
    }
    lp_mt_l.finish(1, *ti_);
    ic += (value.len/VALUE_LEN);

    if ((MERGE == 1) && ((ic * merge_ratio) >= sic) && (ic >= MERGE_THRESHOLD))
      merge_nuv();
  }
  void put_nuv1(const char *key, int keylen, const char *value, int valuelen) {
    put_nuv1(Str(key, keylen), Str(value, valuelen));
  }


  void put_nuv(const char *key, int keylen, const char *value, int valuelen) {
    if (SECONDARY_INDEX_TYPE == 0)
      put_nuv0(key, keylen, value, valuelen);
    else if (SECONDARY_INDEX_TYPE == 1)
      put_nuv1(key, keylen, value, valuelen);
  }


  //#################################################################################
  // Get (unique value)
  //#################################################################################
  inline bool dynamic_get(const Str &key, Str &value) {
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic == 0 || !KeyMayMatch(key.s, key.len, bloom_filter)) {
	return false;
      }
    }
    else {
      if (ic == 0)
	return false;
    }
    lp_mt_u.setup_cursor(table_->table(), key);
    bool found = lp_mt_u.find_unlocked(*ti_);
    if (found)
      value = lp_mt_u.value()->col(0);
    return found;
  }
  bool dynamic_get(const char *key, int keylen, Str &value) {
    return dynamic_get(Str(key, keylen), value);
  }

  inline bool static_get(const Str &key, Str &value) {
    if (sic == 0) {
      return false;
    }
    lp_cmt.setup_cursor(static_table_->table(), key);
    bool found = lp_cmt.find();
    if (found)
      value = Str(lp_cmt.value(), VALUE_LEN);
    return found;
  }
  inline bool static_get(const char *key, int keylen, Str &value) {
    return static_get(Str(key, keylen), value);
  }

  inline bool get (const Str &key, Str &value) {
    if (!dynamic_get(key, value))
      return static_get(key, value);
    return true;
  }
  bool get (const char *key, int keylen, Str &value) {
    return get(Str(key, keylen), value);
  }


  //#################################################################################
  // Get (multi value)
  //#################################################################################
  inline bool static_get_nuv0(const Str &key, Str &value) {
    if (sic == 0) {
      return false;
    }
    lp_cmt_multi.setup_cursor(static_table_->table(), key);
    bool found = lp_cmt_multi.find();
    if (found)
      value = Str(lp_cmt_multi.value_ptr(), lp_cmt_multi.value_len());
    return found;
  }
  bool static_get_nuv0(const char *key, int keylen, Str &value) {
    return static_get_nuv0(Str(key, keylen), value);
  }

  inline bool static_get_nuv1(const Str &key, Str &value) {
    if (sic == 0) {
      return false;
    }
    lp_d.setup_cursor(static_table_->table(), key);
    bool found = lp_d.find();
    if (found)
      value = lp_d.value()->col(0);
    return found;
  }
  bool static_get_nuv1(const char *key, int keylen, Str &value) {
    return static_get_nuv1(Str(key, keylen), value);
  }


  inline bool get_nuv(const Str &key, Str &dynamic_value, Str &static_value) {
    bool dynamic_get_success = dynamic_get(key, dynamic_value);
    bool static_get_success = false; 

    if (SECONDARY_INDEX_TYPE == 0)
      static_get_success = static_get_nuv0(key, static_value);
    else if (SECONDARY_INDEX_TYPE == 1) {
      if (!dynamic_get_success)
	static_get_success = static_get_nuv1(key, static_value);
    }

    if ((!dynamic_get_success) && (!static_get_success))
      return false;
    if (!dynamic_get_success)
      dynamic_value.len = 0;
    else if (!static_get_success)
      static_value.len = 0;
    else {
      //std::cout << "Insertion Errorxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n";
      return false;
    }
    return true;
  }
  bool get_nuv(const char *key, int keylen, Str &dynamic_value, Str &static_value) {
    return get_nuv(Str(key, keylen), dynamic_value, static_value);
  }

  //#################################################################################
  // Get (ordered, unique)
  //#################################################################################
  inline bool dynamic_get_ordered(const Str &key, Str &value) {
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic == 0 || !KeyMayMatch(key.s, key.len, bloom_filter)) {
	cur_keylen_ = 0;
	return false;
      }
    }
    else {
      if (ic == 0) {
	cur_keylen_ = 0;
	return false;
      }
    }
    lp_mt_u.setup_cursor(table_->table(), key);
    bool found = lp_mt_u.find_unlocked(*ti_);
    if (found) {
      value = lp_mt_u.value()->col(0);
      memcpy(cur_key_, key.s, key.len);
      cur_keylen_ = key.len;
    }
    else
      cur_keylen_ = 0;
    return found;
  }
  inline bool dynamic_get_ordered(const char *key, int keylen, Str &value) {
    return dynamic_get_ordered(Str(key, keylen), value);
  }

  //new scan
  inline bool static_get_ordered(const Str &key, Str &value) {
    if (sic == 0) {
      static_cur_keylen_ = 0;
      return false;
    }
    lp_cmt.setup_cursor(static_table_->table(), key);
    bool found = lp_cmt.find();
    if (found) {
      value = Str(lp_cmt.value(), VALUE_LEN);
      memcpy(static_cur_key_, key.s, key.len);
      static_cur_keylen_ = key.len;
      first_scan = true;
      lp_cmt_scan.setup_cursor(static_table_->table(), 
			       Str(static_cur_key_, static_cur_keylen_));
    }
    else
      static_cur_keylen_ = 0;
    return found;
  }
  /*
  inline bool static_get_ordered(const Str &key, Str &value) {
    if (sic == 0) {
      static_cur_keylen_ = 0;
      return false;
    }
    lp_cmt.setup_cursor(static_table_->table(), key);
    bool found = lp_cmt.find();
    if (found) {
      value = Str(lp_cmt.value(), VALUE_LEN);
      memcpy(static_cur_key_, key.s, key.len);
      static_cur_keylen_ = key.len;
    }
    else
      static_cur_keylen_ = 0;
    return found;
  }
  */
  inline bool static_get_ordered(const char *key, int keylen, Str &value) {
    return static_get_ordered(Str(key, keylen), value);
  }

  inline bool get_ordered(const Str &key, Str &value) {
    if (dynamic_get_ordered(key, value)) {
      static_cur_keylen_ = 0;
      return true;
    }
    if (static_get_ordered(key, value)) {
      cur_keylen_ = 0;
      return true;
    }
    return false;
  }
  bool get_ordered(const char *key, int keylen, Str &value) {
    return get_ordered(Str(key, keylen), value);
  }


  //#################################################################################
  // Get (ordered, multi value)
  //#################################################################################
  inline bool static_get_ordered_nuv0(const Str &key, Str &value) {
    if (sic == 0) {
      static_cur_keylen_ = 0;
      return false;
    }
    lp_cmt_multi.setup_cursor(static_table_->table(), key);
    bool found = lp_cmt_multi.find();
    if (found) {
      value = Str(lp_cmt_multi.value_ptr(), lp_cmt_multi.value_len());
      memcpy(static_cur_key_, key.s, key.len);
      static_cur_keylen_ = key.len;
    }
    else
      static_cur_keylen_ = 0;
    return found;
  }
  inline bool static_get_ordered_nuv0(const char *key, int keylen, Str &value) {
    return static_get_ordered_nuv0(Str(key, keylen), value);
  }

  inline bool static_get_ordered_nuv1(const Str &key, Str &value) {
    if (sic == 0) {
      static_cur_keylen_ = 0;
      return false;
    }
    lp_d.setup_cursor(static_table_->table(), key);
    bool found = lp_d.find();
    if (found) {
      value = lp_d.value()->col(0);
      memcpy(static_cur_key_, key.s, key.len);
      static_cur_keylen_ = key.len;
      first_scan = true;
      lp_d_scan.setup_cursor(static_table_->table(), 
			     Str(static_cur_key_, static_cur_keylen_));
    }
    else
      static_cur_keylen_ = 0;
    return found;
  }
  /*
  inline bool static_get_ordered_nuv1(const Str &key, Str &value) {
    if (sic == 0) {
      static_cur_keylen_ = 0;
      return false;
    }
    lp_d.setup_cursor(static_table_->table(), key);
    bool found = lp_d.find();
    if (found) {
      value = lp_d.value()->col(0);
      memcpy(static_cur_key_, key.s, key.len);
      static_cur_keylen_ = key.len;
    }
    else
      static_cur_keylen_ = 0;
    return found;
  }
  */
  inline bool static_get_ordered_nuv1(const char *key, int keylen, Str &value) {
    return static_get_ordered_nuv1(Str(key, keylen), value);
  }

  inline bool get_ordered_nuv(const Str &key, Str &dynamic_value, Str &static_value) {
    bool dynamic_get_success = dynamic_get_ordered(key, dynamic_value);
    bool static_get_success = false;

    if (SECONDARY_INDEX_TYPE == 0)
      static_get_success = static_get_ordered_nuv0(key, static_value);
    else if (SECONDARY_INDEX_TYPE == 1) {
      if (!dynamic_get_success)
	static_get_success = static_get_ordered_nuv1(key, static_value);
    }

    if ((!dynamic_get_success) && (!static_get_success))
      return false;
    if (!dynamic_get_success)
      dynamic_value.len = 0;
    else if (!static_get_success)
      static_value.len = 0;
    else {
      //std::cout << "Insertion Errorxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n";
      return false;
    }
    return true;
  }
  bool get_ordered_nuv(const char *key, int keylen, Str &dynamic_value, Str &static_value) {
    return get_ordered_nuv(Str(key, keylen), dynamic_value, static_value);
  }
  /*
  inline bool get_ordered_nuv(const Str &key, Str &dynamic_value, Str &static_value) {
    bool dynamic_get_success = dynamic_get_ordered(key, dynamic_value);
    bool static_get_success = static_get_ordered_nuv(key, static_value);

    if ((!dynamic_get_success) && (!static_get_success))
      return false;
    if (!dynamic_get_success)
      dynamic_value.len = 0;
    if (!static_get_success)
      static_value.len = 0;
    return true;
  }
  bool get_ordered_nuv(const char *key, int keylen, Str &dynamic_value, Str &static_value) {
    return get_ordered_nuv(Str(key, keylen), dynamic_value, static_value);
  }
  */

  //#################################################################################
  // Exist
  //#################################################################################
  inline bool exist(const Str &key) {
    bool found = false;
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic != 0 && KeyMayMatch(key.s, key.len, bloom_filter)) {
	lp_mt_u.setup_cursor(table_->table(), key);
	found = lp_mt_u.find_unlocked(*ti_);
      }
    }
    else {
      if (ic != 0) {
	lp_mt_u.setup_cursor(table_->table(), key);
	found = lp_mt_u.find_unlocked(*ti_);
      }
    }
    if (!found) {
      lp_cmt.setup_cursor(static_table_->table(), key);
      found = lp_cmt.find();
    }
    return found;
  }
  bool exist(const char *key, int keylen) {
    return exist(Str(key, keylen));
  }

  inline bool exist_nuv(const Str &key) {
    lp_mt_u.setup_cursor(table_->table(), key);
    bool found = false;
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic != 0 && KeyMayMatch(key.s, key.len, bloom_filter)) {
	found = lp_mt_u.find_unlocked(*ti_);
      }
    }
    else {
      if (ic != 0) {
	found = lp_mt_u.find_unlocked(*ti_);
      }
    }
    if (!found) {
      if (SECONDARY_INDEX_TYPE == 0) {
	lp_cmt_multi.setup_cursor(static_table_->table(), key);
	found = lp_cmt_multi.find();
      }
      else if (SECONDARY_INDEX_TYPE == 1) {
	lp_d.setup_cursor(static_table_->table(), key);
	found = lp_d.find();
      }
    }
    return found;
  }
  bool exist_nuv(const char *key, int keylen) {
    return exist_nuv(Str(key, keylen));
  }
  /*
  inline bool exist_nuv(const Str &key) {
    typename T::unlocked_cursor_type lp(table_->table(), key);
    bool found = false;
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic != 0 && KeyMayMatch(key.s, key.len, bloom_filter)) {
	found = lp.find_unlocked(*ti_);
      }
    }
    else {
      if (ic != 0) {
	found = lp.find_unlocked(*ti_);
      }
    }
    if (!found) {
      typename T::static_multivalue_cursor_type slp(static_table_->table(), key);
      found = slp.find();
    }
    return found;
  }
  bool exist_nuv(const char *key, int keylen) {
    return exist_nuv(Str(key, keylen));
  }
  */
  inline bool dynamic_exist(const Str &key) {
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic == 0 || !KeyMayMatch(key.s, key.len, bloom_filter)) {
	return false;
      }
    }
    else {
      if (ic == 0)
	return false;
    }
    lp_mt_u.setup_cursor(table_->table(), key);
    return lp_mt_u.find_unlocked(*ti_);
  }
  inline bool dynamic_exist(const char *key, int keylen) {
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic == 0 || !KeyMayMatch(key, keylen, bloom_filter)) {
	return false;
      }
    }
    else {
      if (ic == 0)
	return false;
    }
    lp_mt_u.setup_cursor(table_->table(), Str(key, keylen));
    return lp_mt_u.find_unlocked(*ti_);
  }
  inline bool static_exist(const Str &key) {
    if (sic == 0)
      return false;
    lp_cmt.setup_cursor(static_table_->table(), key);
    return lp_cmt.find();
  }
  inline bool static_exist(const char *key, int keylen) {
    if (sic == 0)
      return false;
    lp_cmt.setup_cursor(static_table_->table(), Str(key, keylen));
    return lp_cmt.find();
  }
  inline bool static_exist_nuv(const Str &key) {
    if (sic == 0)
      return false;
    if (SECONDARY_INDEX_TYPE == 0) {
      lp_cmt_multi.setup_cursor(static_table_->table(), key);
      return lp_cmt_multi.find();
    }
    else if (SECONDARY_INDEX_TYPE == 1) {
      lp_d.setup_cursor(static_table_->table(), key);
      return lp_d.find();
    }
  }
  inline bool static_exist_nuv(const char *key, int keylen) {
    if (sic == 0)
      return false;
    if (SECONDARY_INDEX_TYPE == 0) {
      lp_cmt_multi.setup_cursor(static_table_->table(), Str(key, keylen));
      return lp_cmt_multi.find();
    }
    else if (SECONDARY_INDEX_TYPE == 1) {
      lp_d.setup_cursor(static_table_->table(), Str(key, keylen));
      return lp_d.find();
    }
  }


  //#################################################################################
  // Partial Key Get (ordered)
  //#################################################################################
  inline bool dynamic_get_upper_bound_or_equal(const char *key, int keylen) {
    if (ic == 0)
      return false;
    Json req = Json::array(0, 0, Str(key, keylen), 1);
    q_[0].run_scan(table_->table(), req, *ti_);
    if (req.size() == 2) {
      cur_keylen_ = 0;
      return false;
    }
    Str retKey = req[2].as_s();
    memcpy(cur_key_, retKey.s, retKey.len);
    cur_keylen_ = retKey.len;
    return true;
  }

  inline bool static_get_upper_bound_or_equal(const char *key, int keylen) {
    if (sic == 0)
      return false;
    lp_cmt_scan.setup_cursor(static_table_->table(), Str(key, keylen));
    bool found = lp_cmt_scan.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_cmt_scan.cur_key();
    int retKeyLen = lp_cmt_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_upper_bound_or_equal_nuv0(const char *key, int keylen) {
    if (sic == 0)
      return false;
    lp_cmt_multi_scan.setup_cursor(static_table_->table(), Str(key, keylen));
    bool found = lp_cmt_multi_scan.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_cmt_multi_scan.cur_key();
    int retKeyLen = lp_cmt_multi_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_upper_bound_or_equal_nuv1(const char *key, int keylen) {
    if (sic == 0)
      return false;
    lp_d_scan.setup_cursor(static_table_->table(), Str(key, keylen));
    bool found = lp_d_scan.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_d_scan.cur_key();
    int retKeyLen = lp_d_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  bool get_upper_bound_or_equal(const char *key, int keylen) {
    bool dynamic_success = dynamic_get_upper_bound_or_equal(key, keylen);
    bool static_success = static_get_upper_bound_or_equal(key, keylen);
    return dynamic_success || static_success;
  }

  bool get_upper_bound_or_equal_nuv(const char *key, int keylen) {
    bool dynamic_success = dynamic_get_upper_bound_or_equal(key, keylen);
    bool static_success = false;
    if (SECONDARY_INDEX_TYPE == 0)
      static_success = static_get_upper_bound_or_equal_nuv0(key, keylen);
    else if (SECONDARY_INDEX_TYPE == 1)
      static_success = static_get_upper_bound_or_equal_nuv1(key, keylen);

    return dynamic_success || static_success;
  }

  inline bool dynamic_get_upper_bound(const char *key, int keylen) {
    if (ic == 0)
      return false;
    if (!dynamic_get_upper_bound_or_equal(key, keylen))
      return false;
    if (dynamic_exist(key, keylen)) {
      Str value;
      if (dynamic_get_next(value))
	return true;
      cur_keylen_ = 0;
      return false;
    }
    return true;
  }

  inline bool static_get_upper_bound(const char *key, int keylen) {
    if (sic == 0)
      return false;
    lp_cmt_scan.setup_cursor(static_table_->table(), Str(key, keylen));
    bool found = lp_cmt_scan.find_upper_bound();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_cmt_scan.cur_key();
    int retKeyLen = lp_cmt_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_upper_bound_nuv0(const char *key, int keylen) {
    if (sic == 0)
      return false;
    lp_cmt_multi_scan.setup_cursor(static_table_->table(), Str(key, keylen));
    bool found = lp_cmt_multi_scan.find_upper_bound();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_cmt_multi_scan.cur_key();
    int retKeyLen = lp_cmt_multi_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_upper_bound_nuv1(const char *key, int keylen) {
    if (sic == 0)
      return false;
    lp_d_scan.setup_cursor(static_table_->table(), Str(key, keylen));
    bool found = lp_d_scan.find_upper_bound();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_d_scan.cur_key();
    int retKeyLen = lp_d_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  bool get_upper_bound(const char *key, int keylen) {
    bool dynamic_success = dynamic_get_upper_bound(key, keylen);
    bool static_success = static_get_upper_bound(key, keylen);
    return dynamic_success || static_success;
  }

  bool get_upper_bound_nuv(const char *key, int keylen) {
    bool dynamic_success = dynamic_get_upper_bound(key, keylen);
    bool static_success = false;
    if (SECONDARY_INDEX_TYPE == 0)
      static_success = static_get_upper_bound_nuv0(key, keylen);
    if (SECONDARY_INDEX_TYPE == 1)
      static_success = static_get_upper_bound_nuv1(key, keylen);

    return dynamic_success || static_success;
  }

  inline bool dynamic_get_first() {
    if (ic == 0)
      return false;
    Json req = Json::array(0, 0, Str("\0", 1), 1);
    q_[0].run_scan(table_->table(), req, *ti_);
    if (req.size() == 2) {
      cur_keylen_ = 0;
      return false;
    }
    Str retKey = req[2].as_s();
    memcpy(cur_key_, retKey.s, retKey.len);
    cur_keylen_ = retKey.len;
    return true;
  }
  inline bool static_get_first() {
    if (sic == 0)
      return false;
    lp_cmt_scan.setup_cursor(static_table_->table(), Str("\0", 1));
    bool found = lp_cmt_scan.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_cmt_scan.cur_key();
    int retKeyLen = lp_cmt_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_first_nuv0() {
    if (sic == 0)
      return false;
    lp_cmt_multi_scan.setup_cursor(static_table_->table(), Str("\0", 1));
    bool found = lp_cmt_multi_scan.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_cmt_multi_scan.cur_key();
    int retKeyLen = lp_cmt_multi_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_first_nuv1() {
    if (sic == 0)
      return false;
    lp_d_scan.setup_cursor(static_table_->table(), Str("\0", 1));
    bool found = lp_d_scan.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp_d_scan.cur_key();
    int retKeyLen = lp_d_scan.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  bool get_first() {
    bool dynamic_success = dynamic_get_first();
    bool static_success = static_get_first();
    return dynamic_success || static_success;
  }

  bool get_first_nuv() {
    bool dynamic_success = dynamic_get_first();
    bool static_success = false;
    if (SECONDARY_INDEX_TYPE == 0)
      static_success = static_get_first_nuv0();
    if (SECONDARY_INDEX_TYPE == 1)
      static_success = static_get_first_nuv1();

    return dynamic_success || static_success;
  }


  //#################################################################################
  // Get Next
  //#################################################################################
  bool get_next(const Str &cur_key, Str &key, Str &value) {
    std::vector<Str> keys;
    std::vector<Str> values;
    Json req = Json::array(0, 0, cur_key, 2);
    q_[0].run_scan(table_->table(), req, *ti_);
    keys.clear();
    values.clear();
    for (int i = 2; i != req.size(); i += 2) {
      keys.push_back(req[i].as_s());
      values.push_back(req[i + 1].as_s());
    }
    if ((keys.size() < 2) || (values.size() < 2))
      return false;
    key = keys[1];
    value = values[1];
    if ((key.len == 0) || (value.len == 0))
      return false;
    return true;
  }

  bool get_next(const char *cur_key, int cur_keylen, Str &key, Str &value) {
    std::vector<Str> keys;
    std::vector<Str> values;
    Json req = Json::array(0, 0, Str(cur_key, cur_keylen), 2);
    q_[0].run_scan(table_->table(), req, *ti_);
    keys.clear();
    values.clear();
    for (int i = 2; i != req.size(); i += 2) {
      keys.push_back(req[i].as_s());
      values.push_back(req[i + 1].as_s());
    }
    if ((keys.size() < 2) || (values.size() < 2))
      return false;
    key = keys[1];
    value = values[1];
    return true;
  }


  //#################################################################################
  // Get Next (ordered)
  //#################################################################################
  inline bool dynamic_get_next(Str &value) {
    Json req = Json::array(0, 0, Str(cur_key_, cur_keylen_), 2);
    q_[0].run_scan(table_->table(), req, *ti_);
    if (req.size() < 4)
      return false;
    value = req[3].as_s();
    if (req.size() < 6) {
      cur_keylen_ = 0;
    }
    else {
      Str cur_key_str = req[4].as_s();
      memcpy(cur_key_, cur_key_str.s, cur_key_str.len);
      cur_keylen_ = cur_key_str.len;
    }
    return true;
  }

  //new scan
  inline bool static_get_next(Str &value) {
    if (sic == 0)
      return false;
    bool found;
    if (first_scan) {
      found = lp_cmt_scan.find_next1();
      first_scan = false;
    }
    else {
      found = lp_cmt_scan.find_next2();
    }

    value = Str(lp_cmt_scan.cur_value(), VALUE_LEN);
    if (!found) {
      static_cur_keylen_ = 0;
    }
    else {
      //lp_cmt_scan.next_key(static_cur_key_);
      static_cur_keylen_ = lp_cmt_scan.next_keylen();
    }
    return true;
  }
  /*
  inline bool static_get_next(Str &value) {
    if (sic == 0)
      return false;
    lp_cmt_scan.setup_cursor(static_table_->table(), 
			     Str(static_cur_key_, static_cur_keylen_));
    bool found = lp_cmt_scan.find_next();
    value = Str(lp_cmt_scan.cur_value(), VALUE_LEN);
    if (!found) {
      static_cur_keylen_ = 0;
    }
    else {
      char *nextKey = lp_cmt_scan.next_key();
      int nextKeyLen = lp_cmt_scan.next_keylen();
      memcpy(static_cur_key_, nextKey, nextKeyLen);
      static_cur_keylen_ = nextKeyLen;
      free(nextKey);
    }
    return true;
  }
  */
  inline bool static_get_next_nuv0(Str &value) {
    if (sic == 0)
      return false;
    lp_cmt_multi_scan.setup_cursor(static_table_->table(), 
				   Str(static_cur_key_, static_cur_keylen_));
    bool found = lp_cmt_multi_scan.find_next();
    value = Str(lp_cmt_multi_scan.cur_value_ptr(), lp_cmt_multi_scan.cur_value_len());
    if (!found) {
      static_cur_keylen_ = 0;
    }
    else {
      char *nextKey = lp_cmt_multi_scan.next_key();
      int nextKeyLen = lp_cmt_multi_scan.next_keylen();
      memcpy(static_cur_key_, nextKey, nextKeyLen);
      static_cur_keylen_ = nextKeyLen;
      free(nextKey);
    }
    return true;
  }

  inline bool static_get_next_nuv1(Str &value) {
    if (sic == 0)
      return false;
    bool found;
    if (first_scan) {
      found = lp_d_scan.find_next1();
      first_scan = false;
    }
    else {
      found = lp_d_scan.find_next2();
    }

    value = lp_d_scan.cur_value()->col(0);
    if (!found) {
      static_cur_keylen_ = 0;
    }
    else {
      //lp_d_scan.next_key(static_cur_key_);
      static_cur_keylen_ = lp_d_scan.next_keylen();
    }
    return true;
  }
  /*
  inline bool static_get_next_nuv1(Str &value) {
    if (sic == 0)
      return false;
    lp_d_scan.setup_cursor(static_table_->table(), 
			   Str(static_cur_key_, static_cur_keylen_));
    bool found = lp_d_scan.find_next();
    value = lp_d_scan.cur_value()->col(0);
    if (!found) {
      static_cur_keylen_ = 0;
    }
    else {
      char *nextKey = lp_d_scan.next_key();
      int nextKeyLen = lp_d_scan.next_keylen();
      memcpy(static_cur_key_, nextKey, nextKeyLen);
      static_cur_keylen_ = nextKeyLen;
      free(nextKey);
    }
    return true;
  }
  */
  bool get_next(Str &value) {
    if ((cur_keylen_ == 0) && (static_cur_keylen_ == 0))
      return false;
    if (cur_keylen_ == 0)
      return static_get_next(value);
    if (static_cur_keylen_ == 0)
      return dynamic_get_next(value);
    int cmplen = cur_keylen_;
    int same_cmp = -1;
    if (static_cur_keylen_ < cur_keylen_) {
      cmplen = static_cur_keylen_;
      same_cmp = 1;
    }
    int cmp = 0;
    int i = 0;

    lp_cmt_scan.next_key(static_cur_key_); //new scan
    while ((cmp == 0) && (i < cmplen)) {
      if ((uint8_t)(cur_key_[i]) < (uint8_t)(static_cur_key_[i]))
	cmp = -1; //dynamic get_next
      else if ((uint8_t)(cur_key_[i]) > (uint8_t)(static_cur_key_[i]))
	cmp = 1; //static get_next
      i++;
    }

    if (cmp == 0)
      cmp = same_cmp;

    if (cmp < 0)
      return dynamic_get_next(value);
    if (cmp > 0)
      return static_get_next(value);
    return false;
  }

  bool get_next_nuv(Str &value) {
    if ((cur_keylen_ == 0) && (static_cur_keylen_ == 0))
      return false;
    if (cur_keylen_ == 0) {
      if (SECONDARY_INDEX_TYPE == 0)
	return static_get_next_nuv0(value);
      else if (SECONDARY_INDEX_TYPE == 1)
	return static_get_next_nuv1(value);
    }
    if (static_cur_keylen_ == 0)
      return dynamic_get_next(value);
    int cmplen = cur_keylen_;
    int same_cmp = -1;
    if (static_cur_keylen_ < cur_keylen_) {
      cmplen = static_cur_keylen_;
      same_cmp = 1;
    }
    int cmp = 0;
    int i = 0;

    lp_d_scan.next_key(static_cur_key_);
    while ((cmp == 0) && (i < cmplen)) {
      if ((uint8_t)(cur_key_[i]) < (uint8_t)(static_cur_key_[i]))
	cmp = -1; //dynamic get_next
      else if ((uint8_t)(cur_key_[i]) > (uint8_t)(static_cur_key_[i]))
	cmp = 1; //static get_next
      i++;
    }

    if (cmp == 0)
      cmp = same_cmp;
    if (cmp < 0)
      return dynamic_get_next(value);
    if (cmp > 0) {
      if (SECONDARY_INDEX_TYPE == 0)
	return static_get_next_nuv0(value);
      else if (SECONDARY_INDEX_TYPE == 1)
	return static_get_next_nuv1(value);
    }
    return false;
  }
  /*
  bool get_next_nuv(Str &value) {
    if ((cur_keylen_ == 0) && (static_cur_keylen_ == 0))
      return false;
    if (cur_keylen_ == 0)
      return static_get_next_nuv(value);
    if (static_cur_keylen_ == 0)
      return dynamic_get_next(value);
    int cmplen = cur_keylen_;
    int same_cmp = -1;
    if (static_cur_keylen_ < cur_keylen_) {
      cmplen = static_cur_keylen_;
      same_cmp = 1;
    }
    int cmp = 0;
    int i = 0;
    while ((cmp == 0) && (i < cmplen)) {
      if ((uint8_t)(cur_key_[i]) < (uint8_t)(static_cur_key_[i]))
	cmp = -1; //dynamic get_next
      else if ((uint8_t)(cur_key_[i]) > (uint8_t)(static_cur_key_[i]))
	cmp = 1; //static get_next
      i++;
    }

    if (cmp == 0)
      cmp = same_cmp;
    if (cmp < 0)
      return dynamic_get_next(value);
    if (cmp > 0)
      return static_get_next_nuv(value);
    return false;
  }
  */
  inline bool dynamic_peek_static_cur(Str &value) {
    Json req = Json::array(0, 0, Str(static_cur_key_, static_cur_keylen_), 1);
    q_[0].run_scan(table_->table(), req, *ti_);
    if (req.size() < 4)
      return false;
    value = req[3].as_s();
    Str next_key_str = req[2].as_s();
    memcpy(next_key_, next_key_str.s, next_key_str.len);
    next_keylen_ = next_key_str.len;
    return true;
  }

  inline bool static_peek_dynamic_cur(Str &value) {
    if (sic == 0)
      return false;
    lp_cmt_scan.setup_cursor(static_table_->table(), Str(cur_key_, cur_keylen_));
    bool found = lp_cmt_scan.find_upper_bound_or_equal();
    if (!found)
      return false;
    value = Str(lp_cmt_scan.cur_value(), VALUE_LEN);
    char *retKey = lp_cmt_scan.cur_key();
    int retKeyLen = lp_cmt_scan.cur_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_peek_dynamic_cur_nuv0(Str &value) {
    if (sic == 0)
      return false;
    lp_cmt_multi_scan.setup_cursor(static_table_->table(), 
				   Str(cur_key_, cur_keylen_));
    bool found = lp_cmt_multi_scan.find_upper_bound_or_equal();
    if (!found)
      return false;
    value = Str(lp_cmt_multi_scan.cur_value_ptr(), lp_cmt_multi_scan.cur_value_len());
    char *retKey = lp_cmt_multi_scan.cur_key();
    int retKeyLen = lp_cmt_multi_scan.cur_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_peek_dynamic_cur_nuv1(Str &value) {
    if (sic == 0)
      return false;
    lp_d_scan.setup_cursor(static_table_->table(), 
			   Str(cur_key_, cur_keylen_));
    bool found = lp_d_scan.find_upper_bound_or_equal();
    if (!found)
      return false;
    value = lp_d_scan.cur_value()->col(0);
    char *retKey = lp_d_scan.cur_key();
    int retKeyLen = lp_d_scan.cur_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool dynamic_peek_next(Str &value) {
    Json req = Json::array(0, 0, Str(cur_key_, cur_keylen_), 2);
    q_[0].run_scan(table_->table(), req, *ti_);
    if (req.size() < 6)
      return false;
    value = req[5].as_s();
    Str next_key_str = req[4].as_s();
    memcpy(next_key_, next_key_str.s, next_key_str.len);
    next_keylen_ = next_key_str.len;
    return true;
  }

  inline bool static_peek_next(Str &value) {
    if (sic == 0)
      return false;
    lp_cmt_scan.setup_cursor(static_table_->table(), 
			     Str(static_cur_key_, static_cur_keylen_));
    bool found = lp_cmt_scan.find_next();
    if (!found)
      return false;
    value = Str(lp_cmt_scan.next_value(), VALUE_LEN);
    char *retKey = lp_cmt_scan.next_key();
    int retKeyLen = lp_cmt_scan.next_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_peek_next_nuv0(Str &value) {
    if (sic == 0)
      return false;
    lp_cmt_multi_scan.setup_cursor(static_table_->table(), 
				   Str(static_cur_key_, static_cur_keylen_));
    bool found = lp_cmt_multi_scan.find_next();
    if (!found)
      return false;
    value = Str(lp_cmt_multi_scan.next_value_ptr(), lp_cmt_multi_scan.next_value_len());
    char *retKey = lp_cmt_multi_scan.next_key();
    int retKeyLen = lp_cmt_multi_scan.next_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_peek_next_nuv1(Str &value) {
    if (sic == 0)
      return false;
    lp_d_scan.setup_cursor(static_table_->table(), 
			   Str(static_cur_key_, static_cur_keylen_));
    bool found = lp_d_scan.find_next();
    if (!found)
      return false;
    value = lp_d_scan.next_value()->col(0);
    char *retKey = lp_d_scan.next_key();
    int retKeyLen = lp_d_scan.next_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  bool advance(Str &value) {
    if ((cur_keylen_ == 0) && (static_cur_keylen_ == 0))
      return false;

    Str value_dynamic;
    Str value_static;
    bool dynamic_peek_success = false;
    bool static_peek_success = false;
    if (cur_keylen_ == 0) {
      dynamic_peek_success = dynamic_peek_static_cur(value_dynamic);
      static_peek_success = static_peek_next(value_static);
    }
    else if (static_cur_keylen_ == 0) {
      dynamic_peek_success = dynamic_peek_next(value_dynamic);
      static_peek_success = static_peek_dynamic_cur(value_static);
    }
    else
      return false;

    if (!dynamic_peek_success && !static_peek_success)
      return false;
    if (!dynamic_peek_success) {
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
      value = value_static;
      return true;
    }
    if (!static_peek_success) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      value = value_dynamic;
      return true;
    }

    int cmplen = next_keylen_;
    int same_cmp = -1;
    if (static_next_keylen_ < next_keylen_) {
      cmplen = static_next_keylen_;
      same_cmp = 1;
    }
    int cmp = 0;
    int i = 0;
    while ((cmp == 0) && (i < cmplen)) {
      if ((uint8_t)(next_key_[i]) < (uint8_t)(static_next_key_[i]))
	cmp = -1; //dynamic advance
      else if ((uint8_t)(next_key_[i]) > (uint8_t)(static_next_key_[i]))
	cmp = 1; //static advance
      i++;
    }
    if (cmp == 0)
      cmp = same_cmp;

    if (cmp < 0) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      value = value_dynamic;
    }
    if (cmp > 0) {
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
      value = value_static;
    }
    return true;
  }

  bool advance_nuv(Str &dynamic_value, Str &static_value) {
    if ((cur_keylen_ == 0) && (static_cur_keylen_ == 0))
      return false;

    bool dynamic_peek_success = false;
    bool static_peek_success = false;
    if (cur_keylen_ == 0) {
      dynamic_peek_success = dynamic_peek_static_cur(dynamic_value);
      if (SECONDARY_INDEX_TYPE == 0)
	static_peek_success = static_peek_next_nuv0(static_value);
      else if (SECONDARY_INDEX_TYPE == 1)
	static_peek_success = static_peek_next_nuv1(static_value);
    }
    else if (static_cur_keylen_ == 0) {
      dynamic_peek_success = dynamic_peek_next(dynamic_value);
      if (SECONDARY_INDEX_TYPE == 0)
	static_peek_success = static_peek_dynamic_cur_nuv0(static_value);
      else if (SECONDARY_INDEX_TYPE == 1)
	static_peek_success = static_peek_dynamic_cur_nuv1(static_value);
    }
    else {
      dynamic_peek_success = dynamic_peek_next(dynamic_value);
      if (SECONDARY_INDEX_TYPE == 0)
	static_peek_success = static_peek_next_nuv0(static_value);
      else if (SECONDARY_INDEX_TYPE == 1)
	static_peek_success = static_peek_next_nuv1(static_value);
    }

    if (!dynamic_peek_success && !static_peek_success)
      return false;
    if (!dynamic_peek_success) {
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
      dynamic_value.len = 0;
      return true;
    }
    if (!static_peek_success) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      static_value.len = 0;
      return true;
    }

    int cmplen = next_keylen_;
    if (static_next_keylen_ < next_keylen_)
      cmplen = static_next_keylen_;

    int cmp = 0;
    int i = 0;
    while ((cmp == 0) && (i < cmplen)) {
      if ((uint8_t)(next_key_[i]) < (uint8_t)(static_next_key_[i]))
	cmp = -1; //dynamic advance
      else if ((uint8_t)(next_key_[i]) > (uint8_t)(static_next_key_[i]))
	cmp = 1; //static advance
      i++;
    }

    if (cmp < 0) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      static_value.len = 0;
    }
    if (cmp > 0) {
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
      dynamic_value.len = 0;
    }
    if (cmp == 0) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
    }
    return true;
  }

  /*
  bool advance_nuv(Str &dynamic_value, Str &static_value) {
    if ((cur_keylen_ == 0) && (static_cur_keylen_ == 0))
      return false;

    bool dynamic_peek_success = false;
    bool static_peek_success = false;
    if (cur_keylen_ == 0) {
      dynamic_peek_success = dynamic_peek_static_cur(dynamic_value);
      static_peek_success = static_peek_next_nuv(static_value);
    }
    else if (static_cur_keylen_ == 0) {
      dynamic_peek_success = dynamic_peek_next(dynamic_value);
      static_peek_success = static_peek_dynamic_cur_nuv(static_value);
    }
    else {
      dynamic_peek_success = dynamic_peek_next(dynamic_value);
      static_peek_success = static_peek_next_nuv(static_value);
    }

    if (!dynamic_peek_success && !static_peek_success)
      return false;
    if (!dynamic_peek_success) {
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
      dynamic_value.len = 0;
      return true;
    }
    if (!static_peek_success) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      static_value.len = 0;
      return true;
    }

    int cmplen = next_keylen_;
    if (static_next_keylen_ < next_keylen_)
      cmplen = static_next_keylen_;

    int cmp = 0;
    int i = 0;
    while ((cmp == 0) && (i < cmplen)) {
      if ((uint8_t)(next_key_[i]) < (uint8_t)(static_next_key_[i]))
	cmp = -1; //dynamic advance
      else if ((uint8_t)(next_key_[i]) > (uint8_t)(static_next_key_[i]))
	cmp = 1; //static advance
      i++;
    }

    if (cmp < 0) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      static_value.len = 0;
    }
    if (cmp > 0) {
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
      dynamic_value.len = 0;
    }
    if (cmp == 0) {
      memcpy(cur_key_, next_key_, next_keylen_);
      cur_keylen_ = next_keylen_;
      memcpy(static_cur_key_, static_next_key_, static_next_keylen_);
      static_cur_keylen_ = static_next_keylen_;
    }
    return true;
  }
  */

  //#################################################################################
  // Remove Unique
  //#################################################################################
  inline bool dynamic_remove(const Str &key) {
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic == 0 || !KeyMayMatch(key.s, key.len, bloom_filter)) {
	return false;
      }
    }
    else {
      if (ic == 0)
	return false;
    }
    if (ti_->limbo >= GC_THRESHOLD) {
      clean_rcu();
      ti_->dealloc_rcu += ti_->limbo;
      ti_->limbo = 0;
    }
    bool remove_success = q_[0].run_remove(table_->table(), key, *ti_);
    if (remove_success)
      ic--;
    return remove_success;
  }
  inline bool static_remove(const Str &key) {
    if (sic == 0)
      return false;
    //static_clean_rcu();
    lp_cmt.setup_cursor(static_table_->table(), key);
    bool remove_success = lp_cmt.remove();
    if (remove_success)
      sic--;
    return remove_success;
  }

  inline bool remove(const Str &key) {
    bool remove_success = dynamic_remove(key);
    if (!remove_success)
      remove_success = static_remove(key);
    return remove_success;
  }

  bool remove(const char *key, int keylen) {
    return remove(Str(key, keylen));
  }


  //#################################################################################
  // Remove Multi
  //#################################################################################
  inline bool dynamic_remove_nuv(const Str &key, const Str &value) {
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic == 0 || !KeyMayMatch(key.s, key.len, bloom_filter)) {
	return false;
      }
    }
    else {
      if (ic == 0)
	return false;
    }
    Str get_value;
    if (!dynamic_get(key, get_value))
      return false;
    int i = 0;
    int j = 0;
    while ((i < get_value.len/value.len) && (j < value.len)) {
      if (get_value.s[i*value.len+j] != value.s[j]) {
	i++;
	j = 0;
      }
      else
	j++;
    }
    if (i == (get_value.len/value.len))
      return false;
    if (get_value.len == value.len)
      return dynamic_remove(key);
    int found_pos = i * value.len;
    char *put_back_value_string;
    int put_back_value_len;
    put_back_value_len = get_value.len - value.len;
    put_back_value_string = (char*)malloc(put_back_value_len);
    memcpy(put_back_value_string, get_value.s, found_pos);
    memcpy(put_back_value_string + found_pos, get_value.s + found_pos + value.len, get_value.len - found_pos - value.len);
    put(key, Str(put_back_value_string, put_back_value_len));
    free(put_back_value_string);
    ic--;
    ic -= (put_back_value_len/VALUE_LEN);
    return true;
  }

  inline bool static_remove_nuv0(const Str &key) {
    if (sic == 0)
      return false;
    lp_cmt_multi.setup_cursor(static_table_->table(), key);
    bool remove_success = lp_cmt_multi.remove();
    if (remove_success)
      sic--;
    return remove_success;
  }

  inline bool static_remove_nuv1(const Str &key) {
    if (sic == 0)
      return false;
    lp_d.setup_cursor(static_table_->table(), key);
    bool remove_success = lp_d.remove(*ti_);
    if (remove_success)
      sic--;
    return remove_success;
  }

  inline bool static_remove_nuv0(const Str &key, const Str &value) {
    if (sic == 0)
      return false;
    Str get_value;
    if (!static_get_nuv0(key, get_value))
      return false;
    int i = 0;
    int j = 0;
    while ((i < get_value.len/value.len) && (j < value.len)) {
      if (get_value.s[i*value.len+j] != value.s[j]) {
	i++;
	j = 0;
      }
      else
	j++;
    }
    if (i == (get_value.len/value.len))
      return false;

    if (!static_remove_nuv0(key))
      return false;
    if (get_value.len != value.len) {
      int found_pos = i * value.len;
      char *put_back_value_string;
      int put_back_value_len;
      put_back_value_len = get_value.len - value.len;
      put_back_value_string = (char*)malloc(put_back_value_len);
      memcpy(put_back_value_string, get_value.s, found_pos);
      memcpy(put_back_value_string + found_pos, get_value.s + found_pos + value.len, get_value.len - found_pos - value.len);
      put_nuv0(key, Str(put_back_value_string, put_back_value_len));
      free(put_back_value_string);

      sic -= (put_back_value_len/VALUE_LEN);
    }
    return true;
  }

  inline bool static_remove_nuv1(const Str &key, const Str &value) {
    if (sic == 0)
      return false;
    lp_d.setup_cursor(static_table_->table(), key);
    bool  found = lp_d.find();
    if (!found)
      return false;
    Str get_value = lp_d.value()->col(0);

    int i = 0;
    int j = 0;
    while ((i < get_value.len/value.len) && (j < value.len)) {
      if (get_value.s[i*value.len+j] != value.s[j]) {
	i++;
	j = 0;
      }
      else
	j++;
    }
    if (i == (get_value.len/value.len))
      return false;

    if (get_value.len > value.len) {
      int found_pos = i * value.len;
      char *put_back_value_string;
      int put_back_value_len;
      put_back_value_len = get_value.len - value.len;
      put_back_value_string = (char*)malloc(put_back_value_len);
      memcpy(put_back_value_string, get_value.s, found_pos);
      memcpy(put_back_value_string + found_pos, get_value.s + found_pos + value.len, get_value.len - found_pos - value.len);

      lp_d.value()->deallocate_rcu(*ti_);
      lp_d.value() = row_type::create1(Str(put_back_value_string, put_back_value_len), qtimes_.ts, *ti_);

      free(put_back_value_string);
      sic -= (value.len/VALUE_LEN);
      return true;
    }
    else if (get_value.len == value.len) {
      sic -= (value.len/VALUE_LEN);
      return static_remove_nuv1(key);
    }
    return false;
   }

  inline bool remove_nuv(const Str &key, const Str &value) {
    bool remove_success = dynamic_remove_nuv(key, value);
    if (!remove_success) {
      if (SECONDARY_INDEX_TYPE == 0)
	remove_success = static_remove_nuv0(key, value);
      else if (SECONDARY_INDEX_TYPE == 1)
	remove_success = static_remove_nuv1(key, value);
    }
    return remove_success;
  }
  bool remove_nuv(const char *key, int keylen, const char *value, int valuelen) {
    return remove_nuv(Str(key, keylen), Str(value, valuelen));
  }


  //#################################################################################
  // Replace
  //#################################################################################
  inline bool replace_first1(const Str &key, const Str &value) {
    //lp_mt_u.setup_cursor(table_->table());
    lp_mt_u.setup_cursor(table_->table(), key);
    bool found = false;
    if (USE_BLOOM_FILTER) {
      if (ic != 0 && KeyMayMatch(key.s, key.len, bloom_filter))
	found = lp_mt_u.find_unlocked(*ti_);
    }
    else {
      if (ic != 0)
	found = lp_mt_u.find_unlocked(*ti_);
    }

    char *put_value_string;
    int put_value_len;
    // if NOT found in dynamic, search static
    if ((!found) && (sic != 0)) {
      lp_d.setup_cursor(static_table_->table(), key);
      bool found_s = lp_d.find();
      // if found in static, update the values and return
      if (found_s) {
	put_value_len = lp_d.value()->col(0).len;
	put_value_string = (char*)malloc(put_value_len);
	memcpy(put_value_string, value.s, value.len);
	memcpy(put_value_string + value.len, lp_d.value()->col(0).s + value.len, (put_value_len - value.len));
	lp_d.value()->deallocate_rcu(*ti_);
	lp_d.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *ti_);
	free(put_value_string);
	return true;
      }
    }

    lp_mt_l.setup_cursor(table_->table(), key);
    found = lp_mt_l.find_insert(*ti_);
    if (found) {
      qtimes_.ts = ti_->update_timestamp(lp_mt_l.value()->timestamp());
      qtimes_.prev_ts = lp_mt_l.value()->timestamp();
      put_value_len = lp_mt_l.value()->col(0).len;
      put_value_string = (char*)malloc(put_value_len);
      memcpy(put_value_string, value.s, value.len);
      memcpy(put_value_string + value.len, lp_mt_l.value()->col(0).s + value.len, (put_value_len - value.len));
      lp_mt_l.value()->deallocate_rcu(*ti_);
      lp_mt_l.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *ti_);
      free(put_value_string);
    }
    else {
      return false;
    }
    lp_mt_l.finish(1, *ti_);
    return true;
  }
  bool replace_first1(const char *key, int keylen, const char *value, int valuelen) {
    return replace_first1(Str(key, keylen), Str(value, valuelen));
  }

  bool replace_first(const char *key, int keylen, const char *value, int valuelen) {
    if (SECONDARY_INDEX_TYPE == 0)
      return replace_first1(key, keylen, value, valuelen);
    else if (SECONDARY_INDEX_TYPE == 1)
      return replace_first1(key, keylen, value, valuelen);
  }


  inline bool dynamic_replace(const Str &key, const Str &value, const Str &old_value) {
    //bloom filter
    if (USE_BLOOM_FILTER) {
      if (ic == 0 || !KeyMayMatch(key.s, key.len, bloom_filter)) {
	return false;
      }
    }
    else {
      if (ic == 0)
	return false;
    }
    if (value.len != old_value.len)
      return false;
    lp_mt_l.setup_cursor(table_->table(), key);
    bool found = lp_mt_l.find_insert(*ti_);
    if (!found)
      return false;
    char *put_value_string;
    int put_value_len;
    qtimes_.ts = ti_->update_timestamp(lp_mt_l.value()->timestamp());
    qtimes_.prev_ts = lp_mt_l.value()->timestamp();
    put_value_len = lp_mt_l.value()->col(0).len;
    put_value_string = (char*)malloc(put_value_len);

    int i = 0;
    int j = 0;
    while ((i < (lp_mt_l.value()->col(0).len / value.len)) && (j < value.len)) {
      if ((lp_mt_l.value()->col(0).s)[i*value.len+j] != old_value.s[j]) {
	i++;
	j = 0;
      }
      else
	j++;
    }
    if (i == (lp_mt_l.value()->col(0).len/value.len))
      return false;

    //lp.value()->deallocate_rcu(*ti_);
    int found_pos = i * value.len;
    memcpy(put_value_string, lp_mt_l.value()->col(0).s, put_value_len);
    memcpy(put_value_string + found_pos, value.s, value.len);
    //memcpy(put_value_string, lp.value()->col(0).s, found_pos);
    //memcpy(put_value_string + found_pos, value.s, value.len);
    //memcpy(put_value_string + found_pos + value.len, lp.value()->col(0).s + found_pos + value.len, lp.value()->col(0).len - found_pos - value.len);

    lp_mt_l.value()->deallocate_rcu(*ti_);
    lp_mt_l.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *ti_);

    lp_mt_l.finish(1, *ti_);
    free(put_value_string);
    return true;
  }


  inline bool static_replace0(const Str &key, const Str &value, const Str &old_value) {
    if (sic == 0)
      return false;
    if (value.len != old_value.len)
      return false;
    lp_cmt_multi.setup_cursor(static_table_->table(), key);
    bool found = lp_cmt_multi.find();
    if (!found)
      return false;

    int i = 0;
    int j = 0;
    while ((i < lp_cmt_multi.value_len() / value.len) && (j < value.len)) {
      if ((lp_cmt_multi.value_ptr())[i*value.len+j] != old_value.s[j]) {
	i++;
	j = 0;
      }
      else
	j++;
    }
    if (i == (lp_cmt_multi.value_len()/value.len))
      return false;

    //lp.value()->deallocate_rcu(*ti_);
    int found_pos = i * value.len;
    memcpy(lp_cmt_multi.value_ptr() + found_pos, value.s, value.len);
    return true;
  }

  inline bool static_replace1(const Str &key, const Str &value, const Str &old_value) {
    if (sic == 0)
      return false;
    if (value.len != old_value.len)
      return false;
    lp_d.setup_cursor(static_table_->table(), key);
    bool found = lp_d.find();
    if (!found)
      return false;
    Str get_value = lp_d.value()->col(0);

    int i = 0;
    int j = 0;
    while ((i < get_value.len / value.len) && (j < value.len)) {
      if (get_value.s[i*value.len+j] != old_value.s[j]) {
	i++;
	j = 0;
      }
      else
	j++;
    }
    if (i == (get_value.len/value.len))
      return false;

    int found_pos = i * value.len;
    char *put_back_value_string;
    int put_back_value_len;
    put_back_value_len = get_value.len;
    put_back_value_string = (char*)malloc(put_back_value_len);
    memcpy(put_back_value_string, get_value.s, get_value.len);
    memcpy(put_back_value_string + found_pos, value.s, value.len);

    lp_d.value()->deallocate_rcu(*ti_);
    lp_d.value() = row_type::create1(Str(put_back_value_string, put_back_value_len), qtimes_.ts, *ti_);
    free(put_back_value_string);
    return true;
  }

  inline bool replace(const Str &key, const Str &value, const Str &old_value) {
    bool replace_success = dynamic_replace(key, value, old_value);
    if (!replace_success) {
      if (SECONDARY_INDEX_TYPE == 0)
	replace_success = static_replace0(key, value, old_value);
      else if (SECONDARY_INDEX_TYPE == 1)
	replace_success = static_replace1(key, value, old_value);
    }
    return replace_success;
  }
  bool replace(const char *key, int keylen, const char *value, int valuelen, const char *old_value, int oldvaluelen) {
    return replace(Str(key, keylen), Str(value, valuelen), Str(old_value, oldvaluelen));
  }


  //#################################################################################
  // Update
  //#################################################################################
  inline bool static_update_uv(const Str &key, const char *value) {
    lp_cmt.setup_cursor(static_table_->table(), key);
    return lp_cmt.update(value);
  }

  bool static_update_uv(const char *key, int keylen, const char *value) {
    return static_update_uv(Str(key, keylen), value);
  }

  inline bool update_uv(const Str &key, const char *value) {
    Str get_value;
    if (dynamic_get(key, get_value)) {
      put(key, Str(value, VALUE_LEN));
      return true;
    }
    else
      return static_update_uv(key, value);
  }

  bool update_uv(const char *key, int keylen, const char *value) {
    return update_uv(Str(key, keylen), value);
  }


  //#################################################################################
  // Merge
  //#################################################################################
  bool merge_uv() {
    //std::cout << "merge unique\n";
    //std::cout << "ic = " << ic << "\n";
    //std::cout << "sic = " << sic << "\n";
    //std::cout << "MEMORY CONSUMPTION = " << memory_consumption() << "\n";
    //std::cout << "TREE STATS===============================================\n";
    //tree_stats();
    //std::cout << "TREE STATS END===========================================\n";
    //std::cout << "STATIC TREE STATS========================================\n";
    //tree_stats();
    //std::cout << "STSTIC TREE STATS END====================================\n";
    //std::cout << "ic = " << ic << "\n";
    //std::cout << "sic = " << sic << "\n";
    //print_items();
    if (key_size_ <= 8) {
      q_[0].run_buildStatic_quick(table_->table(), ic, *ti_);
    }
    else {
      q_[0].run_buildStatic(table_->table(), *ti_);
    }
    q_[0].run_merge(static_table_->table(), table_->table(), *sti_, *ti_);
    sic += ic;
    reset();

    //bloom filter
    if (USE_BLOOM_FILTER) {
      free(bloom_filter);
      bloom_filter = CreateEmptyFilter(sic/merge_ratio);
    }

    //static_print_items();
    return true;
  }

  bool merge_nuv() {
    //std::cout << "merge non-unique\n";
    //std::cout << "ic = " << ic << "\n";
    //std::cout << "sic = " << sic << "\n";
    //print_items();
    if (SECONDARY_INDEX_TYPE == 0) {
      q_[0].run_buildStatic_multivalue(table_->table(), *ti_);
      q_[0].run_merge_multivalue(static_table_->table(), table_->table(), *sti_, *ti_);
    }
    else if (SECONDARY_INDEX_TYPE == 1) {
      //std::cout << "merge non-unique\n";
      //std::cout << "ic = " << ic << "\n";
      //std::cout << "sic = " << sic << "\n";
      q_[0].run_buildStatic_dynamicvalue(table_->table(), *ti_);
      q_[0].run_merge_dynamicvalue(static_table_->table(), table_->table(), *sti_, *ti_);
    }

    sic += ic;
    reset();

    //bloom filter
    if (USE_BLOOM_FILTER) {
      free(bloom_filter);
      bloom_filter = CreateEmptyFilter(sic/merge_ratio);
    }

    //static_print_items();
    return true;
  }

  /*
  bool merge_uv() {
    return true;
  }

  bool merge_nuv() {
    return true;
  }
  */


  //#################################################################################
  // Print Tree
  //#################################################################################
  /*
  void print_items () {
    std::vector<Str> keys;
    std::vector<Str> values;
    Json req = Json::array(0, 0, Str("\0", 1), ic);
    q_[0].run_scan(table_->table(), req, *ti_);
    keys.clear();
    values.clear();
    for (int i = 2; i != req.size(); i+= 2) {
      keys.push_back(req[i].as_s());
      values.push_back(req[i + 1].as_s());
    }
    std::cout << "\n-----------------------------------------------------------\n";
    std::cout << "size = " << keys.size() << "\n\n";
    for (int i = 0; i < keys.size(); i++) {
      for (int j = 0; j < keys[i].len; j++)
	std::cout << (int)((keys[i].s)[j]);
      std::cout << " ";
    }
    std::cout << "\n-----------------------------------------------------------\n";
  }

  void static_print_items () {
    std::vector<Str> keys;
    std::vector<Str> values;
    Json req = Json::array(0, 0, Str("\0", 1), sic);
    q_[0].run_scan(static_table_->table(), req, *sti_);
    keys.clear();
    values.clear();
    for (int i = 2; i != req.size(); i+= 2) {
      keys.push_back(req[i].as_s());
      values.push_back(req[i + 1].as_s());
    }
    std::cout << "\nSTATIC-----------------------------------------------------------\n";
    std::cout << "static size = " << keys.size() << "\n\n";
    for (int i = 0; i < keys.size(); i++) {
      for (int j = 0; j < keys[i].len; j++)
	std::cout << (int)((keys[i].s)[j]);
      std::cout << " ";
    }
    std::cout << "\nEND-----------------------------------------------------------\n";
  }
  */

  //#################################################################################
  // Memory Stats
  //#################################################################################
  uint64_t memory_consumption () const {
    /*
    std::cout << "pool_alloc = " << ti_->pool_alloc << "\n";
    std::cout << "pool_dealloc = " << ti_->pool_dealloc << "\n";
    std::cout << "pool_dealloc_rcu = " << ti_->pool_dealloc_rcu << "\n";
    std::cout << "alloc = " << ti_->alloc << "\n";
    std::cout << "dealloc = " << ti_->dealloc << "\n";
    std::cout << "dealloc_rcu = " << ti_->dealloc_rcu << "\n\n";

    std::cout << "static pool_alloc = " << sti_->pool_alloc << "\n";
    std::cout << "static pool_dealloc = " << sti_->pool_dealloc << "\n";
    std::cout << "static pool_dealloc_rcu = " << sti_->pool_dealloc_rcu << "\n";
    std::cout << "static alloc = " << sti_->alloc << "\n";
    std::cout << "static dealloc = " << sti_->dealloc << "\n";
    std::cout << "static dealloc_rcu = " << sti_->dealloc_rcu << "\n\n";

    std::cout << "bits = " << bits << "\n\n";
    */
    /*
    std::vector<uint32_t> nkeys_stats;
    q_[0].run_stats(table_->table(), *ti_, nkeys_stats);
    for (int i = 0; i < nkeys_stats.size(); i++)
      std::cout << nkeys_stats[i] << " ";
    std::cout << "\n";
    */
    return (ti_->pool_alloc 
	    + ti_->alloc 
	    - ti_->pool_dealloc 
	    - ti_->dealloc 
	    - ti_->pool_dealloc_rcu 
	    - ti_->dealloc_rcu
	    + sti_->pool_alloc 
	    + sti_->alloc 
	    - sti_->pool_dealloc 
	    - sti_->dealloc 
	    - sti_->pool_dealloc_rcu 
	    - sti_->dealloc_rcu
	    + bits/8);
    //return (ti_->pool_alloc + ti_->alloc - ti_->pool_dealloc - ti_->dealloc);
  }
  /*
  void tree_stats () {
    std::vector<uint32_t> nkeys_stats;
    q_[0].run_stats(table_->table(), *ti_, nkeys_stats);
    for (int i = 0; i < nkeys_stats.size(); i++)
      std::cout << "\t" << nkeys_stats[i] << " ";
    std::cout << "\n";
    std::cout << "\t" << "pool_alloc = " << ti_->pool_alloc << "\n";
    std::cout << "\t" << "pool_dealloc = " << ti_->pool_dealloc << "\n";
    std::cout << "\t" << "pool_dealloc_rcu = " << ti_->pool_dealloc_rcu << "\n";
    std::cout << "\t" << "alloc = " << ti_->alloc << "\n";
    std::cout << "\t" << "dealloc = " << ti_->dealloc << "\n";
    std::cout << "\t" << "dealloc_rcu = " << ti_->dealloc_rcu << "\n";
    std::cout << "\t" << "stringbag_alloc = " << ti_->stringbag_alloc << "\n";
    std::cout << "\t" << "valuebag_alloc = " << ti_->valuebag_alloc << "\n";
    std::cout << "\t" << "limbo = " << ti_->limbo << "\n";

    if (multivalue_)
      q_[0].run_static_multivalue_stats(static_table_->table());
    else
      q_[0].run_static_stats(static_table_->table());

    std::cout << "\t" << "pool_alloc = " << sti_->pool_alloc << "\n";
    std::cout << "\t" << "pool_dealloc = " << sti_->pool_dealloc << "\n";
    std::cout << "\t" << "pool_dealloc_rcu = " << sti_->pool_dealloc_rcu << "\n";
    std::cout << "\t" << "alloc = " << sti_->alloc << "\n";
    std::cout << "\t" << "dealloc = " << sti_->dealloc << "\n";
    std::cout << "\t" << "dealloc_rcu = " << sti_->dealloc_rcu << "\n";
    std::cout << "\t" << "stringbag_alloc = " << sti_->stringbag_alloc << "\n";
    std::cout << "\t" << "valuebag_alloc = " << sti_->valuebag_alloc << "\n";
    std::cout << "\t" << "limbo = " << sti_->limbo << "\n";
  }
  */

  //#################################################################################
  // Accessors
  //#################################################################################
  int get_ic () {
    return ic;
  }
  int get_sic () {
    return sic;
  }

  bool merge() {
    if (multivalue_)
      return merge_nuv();
    else
      return merge_uv();
  }


private:
  T *table_;
  T *static_table_;
  int ic;
  int sic;
  threadinfo *ti_;
  threadinfo *sti_;
  query<row_type> q_[1];
  loginfo::query_times qtimes_;

  char* cur_key_;
  int cur_keylen_;
  char* next_key_;
  int next_keylen_;
  char* static_cur_key_;
  int static_cur_keylen_;
  char* static_next_key_;
  int static_next_keylen_;

  bool multivalue_;
  double merge_ratio;
  int key_size_;

  size_t bits;
  char* bloom_filter;

  bool first_scan;

  typename T::unlocked_cursor_type lp_mt_u;
  typename T::cursor_type lp_mt_l;

  typename T::static_cursor_type lp_cmt;
  typename T::static_multivalue_cursor_type lp_cmt_multi;
  typename T::static_dynamicvalue_cursor_type lp_d;

  typename T::static_cursor_scan_type lp_cmt_scan;
  typename T::static_multivalue_cursor_scan_type lp_cmt_multi_scan;
  typename T::static_dynamicvalue_cursor_scan_type lp_d_scan;
};

#endif //MTINDEXAPI_H
