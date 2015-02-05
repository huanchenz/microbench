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

#include "config.h"

#define GC_THRESHOLD 1000000
#define MERGE_THESHOLD 1000
#define MERGE_RATIO 10
#define VALUE_LEN 8

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

    if (multivalue_)
      q_[0].run_destroy_static_multivalue(static_table_->table(), *sti_);
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
    first_merge = true;
  }

  void setup(bool multivalue, int mt, int mr) {
    setup();

    cur_key_ = NULL;
    cur_keylen_ = 0;
    next_key_ = NULL;
    next_keylen_ = 0;
    static_cur_key_ = NULL;
    static_cur_keylen_ = 0;
    static_next_key_ = NULL;
    static_next_keylen_ = 0;

    multivalue_ = multivalue;
    merge_threshold = mt;
    merge_ratio = mr;
  }

  void setup(int keyLen, bool multivalue, int mt, int mr) {
    setup();

    cur_key_ = (char*)malloc(keyLen * 2);
    cur_keylen_ = 0;
    next_key_ = (char*)malloc(keyLen * 2);
    next_keylen_ = 0;
    static_cur_key_ = (char*)malloc(keyLen * 2);
    static_cur_keylen_ = 0;
    static_next_key_ = (char*)malloc(keyLen * 2);
    static_next_keylen_ = 0;

    multivalue_ = multivalue;
    merge_threshold = mt;
    merge_ratio = mr;
  }

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
    table_->destroy(*ti_);
    delete table_;
    gc_dynamic();
    table_ = new T;
    table_->initialize(*ti_);
    ic = 0;
  }

  //put unique
  //=====================================================================================  
  inline bool put_uv(const Str &key, const Str &value) {
    if (static_exist(key.s, key.len)) {
      std::cout << "static exist\n";
      return false;
    }
    typename T::cursor_type lp(table_->table(), key);
    bool found = lp.find_insert(*ti_);
    if (!found)
      ti_->advance_timestamp(lp.node_timestamp());
    else {
      lp.finish(1, *ti_);
      return false;
    }
    qtimes_.ts = ti_->update_timestamp();
    qtimes_.prev_ts = 0;
    lp.value() = row_type::create1(value, qtimes_.ts, *ti_);
    lp.finish(1, *ti_);
    ic++;
    //if (ic >= MERGE_THESHOLD)
    //return merge_uv();
    //if (((ic * MERGE_RATIO) >= sic) && (ic >= MERGE_THESHOLD))
    if ((((ic * merge_ratio) >= sic) || (merge_ratio == 0)) && (ic >= merge_threshold))
      return merge_uv();
    return true;
  }
  bool put_uv(const char *key, int keylen, const char *value, int valuelen) {
    return put_uv(Str(key, keylen), Str(value, valuelen));
  }

  inline bool static_put_uv(const Str &key, const Str &value) {
    typename T::cursor_type lp(static_table_->table(), key);
    bool found = lp.find_insert(*sti_);
    if (!found)
      sti_->advance_timestamp(lp.node_timestamp());
    else {
      lp.finish(1, *sti_);
      return false;
    }
    qtimes_.ts = sti_->update_timestamp();
    qtimes_.prev_ts = 0;
    lp.value() = row_type::create1(value, qtimes_.ts, *sti_);
    lp.finish(1, *sti_);
    sic++;
    return true;
  }

  //=====================================================================================  

  //put (overwrite)
  //=====================================================================================  
  inline void put(const Str &key, const Str &value) {
    typename T::cursor_type lp(table_->table(), key);
    bool found = lp.find_insert(*ti_);
    if (!found) {
      ti_->advance_timestamp(lp.node_timestamp());
      qtimes_.ts = ti_->update_timestamp();
      qtimes_.prev_ts = 0;
    }
    else {
      qtimes_.ts = ti_->update_timestamp(lp.value()->timestamp());
      qtimes_.prev_ts = lp.value()->timestamp();
      lp.value()->deallocate_rcu(*ti_);
    }
    lp.value() = row_type::create1(value, qtimes_.ts, *ti_);
    lp.finish(1, *ti_);
    //ic++;
    ic += (value.len/VALUE_LEN);
  }
  void put(const char *key, int keylen, const char *value, int valuelen) {
    return put(Str(key, keylen), Str(value, valuelen));
  }

  inline void static_put(const Str &key, const Str &value) {
    typename T::cursor_type lp(static_table_->table(), key);
    bool found = lp.find_insert(*sti_);
    if (!found) {
      sti_->advance_timestamp(lp.node_timestamp());
      qtimes_.ts = sti_->update_timestamp();
      qtimes_.prev_ts = 0;
    }
    else {
      qtimes_.ts = sti_->update_timestamp(lp.value()->timestamp());
      qtimes_.prev_ts = lp.value()->timestamp();
      lp.value()->deallocate_rcu(*sti_);
    }
    lp.value() = row_type::create1(value, qtimes_.ts, *sti_);
    lp.finish(1, *sti_);
    //sic++;
    sic += (value.len/VALUE_LEN);
  }

  //=====================================================================================  

  //put non-unique
  //=====================================================================================  
  inline void put_nuv(const Str &key, const Str &value) {
    typename T::cursor_type lp(table_->table(), key);
    bool found = lp.find_insert(*ti_);
    if (!found)
      ti_->advance_timestamp(lp.node_timestamp());
    char *put_value_string;
    int put_value_len;
    if (!found) {
      qtimes_.ts = ti_->update_timestamp();
      qtimes_.prev_ts = 0;
      lp.value() = row_type::create1(value, qtimes_.ts, *ti_);
    }
    else {
      qtimes_.ts = ti_->update_timestamp(lp.value()->timestamp());
      qtimes_.prev_ts = lp.value()->timestamp();
      //lp.value()->deallocate_rcu(*ti_);
      put_value_len = value.len + lp.value()->col(0).len;
      put_value_string = (char*)malloc(put_value_len);
      //char put_value_string[4096];
      memcpy(put_value_string, value.s, value.len);
      memcpy(put_value_string + value.len, lp.value()->col(0).s, lp.value()->col(0).len);
      lp.value()->deallocate_rcu(*ti_);
      lp.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *ti_);
      free(put_value_string);
    }
    lp.finish(1, *ti_);
    //ic++;
    ic += (value.len/VALUE_LEN);
    //if (ic >= MERGE_THESHOLD)
    //if (((ic * MERGE_RATIO) >= sic) && (ic >= MERGE_THESHOLD))
    if ((((ic * merge_ratio) >= sic) || (merge_ratio == 0)) && (ic >= merge_threshold))
      merge_nuv();
  }
  void put_nuv(const char *key, int keylen, const char *value, int valuelen) {
    return put_nuv(Str(key, keylen), Str(value, valuelen));
  }

  inline void static_put_nuv(const Str &key, const Str &value) {
    typename T::cursor_type lp(static_table_->table(), key);
    bool found = lp.find_insert(*sti_);
    if (!found)
      sti_->advance_timestamp(lp.node_timestamp());
    char *put_value_string;
    int put_value_len;
    if (!found) {
      qtimes_.ts = sti_->update_timestamp();
      qtimes_.prev_ts = 0;
      lp.value() = row_type::create1(value, qtimes_.ts, *sti_);
    }
    else {
      qtimes_.ts = sti_->update_timestamp(lp.value()->timestamp());
      qtimes_.prev_ts = lp.value()->timestamp();
      //lp.value()->deallocate_rcu(*ti_);
      put_value_len = value.len + lp.value()->col(0).len;
      put_value_string = (char*)malloc(put_value_len);
      //char put_value_string[4096];
      memcpy(put_value_string, value.s, value.len);
      memcpy(put_value_string + value.len, lp.value()->col(0).s, lp.value()->col(0).len);
      lp.value()->deallocate_rcu(*sti_);
      lp.value() = row_type::create1(Str(put_value_string, put_value_len), qtimes_.ts, *sti_);
      free(put_value_string);
    }
    lp.finish(1, *sti_);
    //sic++;
    sic += (value.len/VALUE_LEN);
  }

  //=====================================================================================

  //get
  //=====================================================================================
  inline bool dynamic_get(const Str &key, Str &value) {
    if (ic == 0)
      return false;
    typename T::unlocked_cursor_type lp(table_->table(), key);
    bool found = lp.find_unlocked(*ti_);
    if (found)
      value = lp.value()->col(0);
    return found;
  }
  inline bool dynamic_get(const char *key, int keylen, Str &value) {
    return dynamic_get(Str(key, keylen), value);
  }

  inline bool static_get(const Str &key, Str &value) {
    if (sic == 0)
      return false;
    typename T::static_cursor_type lp(static_table_->table(), key);
    bool found = lp.find();
    if (found)
      value = Str(lp.value(), VALUE_LEN);
    return found;
  }
  inline bool static_get(const char *key, int keylen, Str &value) {
    return static_get(Str(key, keylen), value);
  }

  inline bool static_get_nuv(const Str &key, Str &value) {
    if (sic == 0)
      return false;
    typename T::static_multivalue_cursor_type lp(static_table_->table(), key);
    bool found = lp.find();
    if (found)
      value = Str(lp.value_ptr(), lp.value_len());
    return found;
  }
  inline bool static_get_nuv(const char *key, int keylen, Str &value) {
    return static_get_nuv(Str(key, keylen), value);
  }

  inline bool get (const Str &key, Str &value) {
    if (!dynamic_get(key, value))
      return static_get(key, value);
    return true;
  }
  bool get (const char *key, int keylen, Str &value) {
    return get(Str(key, keylen), value);
  }

  inline bool get_nuv(const Str &key, Str &dynamic_value, Str &static_value) {
    bool dynamic_get_success = dynamic_get(key, dynamic_value);
    bool static_get_success = static_get_nuv(key, static_value);
    if ((!dynamic_get_success) && (!static_get_success))
      return false;
    if (!dynamic_get_success)
      dynamic_value.len = 0;
    if (!static_get_success)
      static_value.len = 0;
    return true;
  }
  bool get_nuv(const char *key, int keylen, Str &dynamic_value, Str &static_value) {
    return get_nuv(Str(key, keylen), dynamic_value, static_value);
  }
  //=====================================================================================

  //get_ordered (prepare for get_next)
  //=====================================================================================
  inline bool dynamic_get_ordered(const Str &key, Str &value) {
    if (ic == 0) {
      cur_keylen_ = 0;
      return false;
    }
    typename T::unlocked_cursor_type lp(table_->table(), key);
    bool found = lp.find_unlocked(*ti_);
    if (found) {
      value = lp.value()->col(0);
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

  inline bool static_get_ordered(const Str &key, Str &value) {
    if (sic == 0) {
      static_cur_keylen_ = 0;
      return false;
    }
    typename T::static_cursor_type lp(static_table_->table(), key);
    bool found = lp.find();
    if (found) {
      value = Str(lp.value(), VALUE_LEN);
      memcpy(static_cur_key_, key.s, key.len);
      static_cur_keylen_ = key.len;
    }
    else
      static_cur_keylen_ = 0;
    return found;
  }
  inline bool static_get_ordered(const char *key, int keylen, Str &value) {
    return static_get_ordered(Str(key, keylen), value);
  }

  inline bool static_get_ordered_nuv(const Str &key, Str &value) {
    if (sic == 0) {
      static_cur_keylen_ = 0;
      return false;
    }
    typename T::static_multivalue_cursor_type lp(static_table_->table(), key);
    bool found = lp.find();
    if (found) {
      value = Str(lp.value_ptr(), lp.value_len());
      memcpy(static_cur_key_, key.s, key.len);
      static_cur_keylen_ = key.len;
    }
    else
      static_cur_keylen_ = 0;
    return found;
  }
  inline bool static_get_ordered_nuv(const char *key, int keylen, Str &value) {
    return static_get_ordered_nuv(Str(key, keylen), value);
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
  //=====================================================================================

  //exist
  //=====================================================================================
  inline bool exist(const Str &key) {
    typename T::unlocked_cursor_type lp(table_->table(), key);
    bool found = lp.find_unlocked(*ti_);
    if (!found) {
      typename T::static_cursor_type slp(static_table_->table(), key);
      found = slp.find();
    }
    return found;
  }
  bool exist(const char *key, int keylen) {
    return exist(Str(key, keylen));
  }

  inline bool exist_nuv(const Str &key) {
    typename T::unlocked_cursor_type lp(table_->table(), key);
    bool found = lp.find_unlocked(*ti_);
    if (!found) {
      typename T::static_multivalue_cursor_type slp(static_table_->table(), key);
      found = slp.find();
    }
    return found;
  }
  bool exist_nuv(const char *key, int keylen) {
    return exist_nuv(Str(key, keylen));
  }

  inline bool dynamic_exist(const Str &key) {
    typename T::unlocked_cursor_type lp(table_->table(), key);
    return lp.find_unlocked(*ti_);
  }
  inline bool dynamic_exist(const char *key, int keylen) {
    typename T::unlocked_cursor_type lp(table_->table(), Str(key, keylen));
    return lp.find_unlocked(*ti_);
  }
  inline bool static_exist(const Str &key) {
    typename T::static_cursor_type lp(static_table_->table(), key);
    return lp.find();
  }
  inline bool static_exist(const char *key, int keylen) {
    typename T::static_cursor_type lp(static_table_->table(), Str(key, keylen));
    return lp.find();
  }
  inline bool static_exist_nuv(const Str &key) {
    typename T::static_multivalue_cursor_type lp(static_table_->table(), key);
    return lp.find();
  }
  inline bool static_exist_nuv(const char *key, int keylen) {
    typename T::static_multivalue_cursor_type lp(static_table_->table(), Str(key, keylen));
    return lp.find();
  }
  //=====================================================================================

  //partial key get
  //=====================================================================================
  bool get_upper_bound_or_equal(const char *key, int keylen, Str &retKey) {
    Json req = Json::array(0, 0, Str(key, keylen), 1);
    q_[0].run_scan(table_->table(), req, *ti_);
    if (req.size() == 2)
      return false;
    retKey = req[2].as_s();
    return true;
  }

  bool get_upper_bound(const char *key, int keylen, Str &retKey) {
    if (!get_upper_bound_or_equal(key, keylen, retKey))
      return false;
    Str value;
    if (get(key, keylen, value)) {
      if (get_next(retKey, retKey, value))
	return true;
      return false;
    }
    return true;
  }

  bool get_first(Str &key) {
    Json req = Json::array(0, 0, Str("\0", 1), 1);
    q_[0].run_scan(table_->table(), req, *ti_);
    if (req.size() == 2)
      return false;
    key = req[2].as_s();
    return true;
  }
  //=====================================================================================

  //partial key get (prepare for get_next)
  //=====================================================================================
  inline bool dynamic_get_upper_bound_or_equal(const char *key, int keylen) {
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
    typename T::static_cursor_scan_type lp(static_table_->table(), Str(key, keylen));
    bool found = lp.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_upper_bound_or_equal_nuv(const char *key, int keylen) {
    typename T::static_multivalue_cursor_scan_type lp(static_table_->table(), Str(key, keylen));
    bool found = lp.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
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
    bool static_success = static_get_upper_bound_or_equal_nuv(key, keylen);
    return dynamic_success || static_success;
  }

  inline bool dynamic_get_upper_bound(const char *key, int keylen) {
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
    typename T::static_cursor_scan_type lp(static_table_->table(), Str(key, keylen));
    bool found = lp.find_upper_bound();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_get_upper_bound_nuv(const char *key, int keylen) {
    typename T::static_multivalue_cursor_scan_type lp(static_table_->table(), Str(key, keylen));
    bool found = lp.find_upper_bound();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
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
    bool static_success = static_get_upper_bound_nuv(key, keylen);
    return dynamic_success || static_success;
  }

  inline bool dynamic_get_first() {
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
    typename T::static_cursor_scan_type lp(static_table_->table(), Str("\0", 1));
    bool found = lp.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
    memcpy(static_cur_key_, retKey, retKeyLen);
    static_cur_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }
  inline bool static_get_first_nuv() {
    typename T::static_multivalue_cursor_scan_type lp(static_table_->table(), Str("\0", 1));
    bool found = lp.find_upper_bound_or_equal();
    if (!found) {
      static_cur_keylen_ = 0;
      return false;
    }
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
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
    bool static_success = static_get_first_nuv();
    return dynamic_success || static_success;
  }
  //=====================================================================================

  //scan
  //=====================================================================================
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
  //=====================================================================================

  //scan (prepare for get_next)
  //=====================================================================================
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

  inline bool static_get_next(Str &value) {
    typename T::static_cursor_scan_type lp(static_table_->table(), 
					   Str(static_cur_key_, static_cur_keylen_));
    bool found = lp.find_next();
    value = Str(lp.cur_value(), VALUE_LEN);
    if (!found) {
      static_cur_keylen_ = 0;
    }
    else {
      char *nextKey = lp.next_key();
      int nextKeyLen = lp.next_keylen();
      memcpy(static_cur_key_, nextKey, nextKeyLen);
      static_cur_keylen_ = nextKeyLen;
      free(nextKey);
    }
    return true;
  }

  inline bool static_get_next_nuv(Str &value) {
    typename T::static_multivalue_cursor_scan_type lp(static_table_->table(), 
						      Str(static_cur_key_, static_cur_keylen_));
    bool found = lp.find_next();
    value = Str(lp.cur_value_ptr(), lp.cur_value_len());
    if (!found) {
      static_cur_keylen_ = 0;
    }
    else {
      char *nextKey = lp.next_key();
      int nextKeyLen = lp.next_keylen();
      memcpy(static_cur_key_, nextKey, nextKeyLen);
      static_cur_keylen_ = nextKeyLen;
      free(nextKey);
    }
    return true;
  }

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
    typename T::static_cursor_scan_type lp(static_table_->table(), Str(cur_key_, cur_keylen_));
    bool found = lp.find_upper_bound_or_equal();
    if (!found)
      return false;
    value = Str(lp.cur_value(), VALUE_LEN);
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_peek_dynamic_cur_nuv(Str &value) {
    typename T::static_multivalue_cursor_scan_type lp(static_table_->table(), 
						      Str(cur_key_, cur_keylen_));
    bool found = lp.find_upper_bound_or_equal();
    if (!found)
      return false;
    value = Str(lp.cur_value_ptr(), lp.cur_value_len());
    char *retKey = lp.cur_key();
    int retKeyLen = lp.cur_keylen();
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
    typename T::static_cursor_scan_type lp(static_table_->table(), 
					   Str(static_cur_key_, static_cur_keylen_));
    bool found = lp.find_next();
    if (!found)
      return false;
    value = Str(lp.next_value(), VALUE_LEN);
    char *retKey = lp.next_key();
    int retKeyLen = lp.next_keylen();
    memcpy(static_next_key_, retKey, retKeyLen);
    static_next_keylen_ = retKeyLen;
    free(retKey);
    return true;
  }

  inline bool static_peek_next_nuv(Str &value) {
    typename T::static_multivalue_cursor_scan_type lp(static_table_->table(), 
						      Str(static_cur_key_, static_cur_keylen_));
    bool found = lp.find_next();
    if (!found)
      return false;
    value = Str(lp.next_value_ptr(), lp.next_value_len());
    char *retKey = lp.next_key();
    int retKeyLen = lp.next_keylen();
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
  //=====================================================================================

  //remove
  //=====================================================================================
  inline bool dynamic_remove(const Str &key) {
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
    //static_clean_rcu();
    typename T::static_cursor_type lp(static_table_->table(), key);
    bool remove_success = lp.remove();
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
  //=====================================================================================

  //remove non-unique
  //=====================================================================================
  inline bool dynamic_remove_nuv(const Str &key, const Str &value) {
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

  inline bool static_remove_nuv(const Str &key) {
    bool remove_success = q_[0].run_remove(static_table_->table(), key, *sti_); //redundant
    typename T::static_multivalue_cursor_type lp(static_table_->table(), key);
    remove_success = lp.remove();
    if (remove_success)
      sic--;
    return remove_success;
  }

  inline bool static_remove_nuv(const Str &key, const Str &value) {
    Str get_value;
    if (!static_get_nuv(key, get_value))
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

    if (!static_remove_nuv(key))
      return false;
    if (get_value.len != value.len) {
      int found_pos = i * value.len;
      char *put_back_value_string;
      int put_back_value_len;
      put_back_value_len = get_value.len - value.len;
      put_back_value_string = (char*)malloc(put_back_value_len);
      memcpy(put_back_value_string, get_value.s, found_pos);
      memcpy(put_back_value_string + found_pos, get_value.s + found_pos + value.len, get_value.len - found_pos - value.len);
      put_nuv(key, Str(put_back_value_string, put_back_value_len));
      free(put_back_value_string);

      sic -= (put_back_value_len/VALUE_LEN);
    }
    return true;
  }

  inline bool remove_nuv(const Str &key, const Str &value) {
    bool remove_success = dynamic_remove_nuv(key, value);
    if (!remove_success)
      remove_success = static_remove_nuv(key, value);
    return remove_success;
  }
  bool remove_nuv(const char *key, int keylen, const char *value, int valuelen) {
    return remove_nuv(Str(key, keylen), Str(value, valuelen));
  }
  //=====================================================================================

  //replace
  //=====================================================================================
  inline bool dynamic_replace_first(const Str &key, const Str &value) {
    Str get_value;
    if (!dynamic_get(key, get_value))
      return false;
    char *put_value_string;
    int put_value_len;
    put_value_len = get_value.len;
    put_value_string = (char*)malloc(put_value_len);
    memcpy(put_value_string, value.s, value.len);
    memcpy(put_value_string + value.len, get_value.s + value.len, get_value.len - value.len);
    put(key, Str(put_value_string, put_value_len));
    free(put_value_string);
    return true;
  }

  inline bool static_replace_first(const Str &key, const Str &value) {
    Str get_value;
    if (!static_get_nuv(key, get_value))
      return false;
    char *put_value_string;
    int put_value_len;
    put_value_len = get_value.len;
    put_value_string = (char*)malloc(put_value_len);
    memcpy(put_value_string, value.s, value.len);
    memcpy(put_value_string + value.len, get_value.s + value.len, get_value.len - value.len);
    put(key, Str(put_value_string, put_value_len));
    free(put_value_string);

    static_remove(key);
    return static_remove_nuv(key);
  }

  inline bool replace_first(const Str &key, const Str &value) {
    bool replace_success = dynamic_replace_first(key, value);
    if (!replace_success)
      replace_success = static_replace_first(key, value);
    return replace_success;
  }
  bool replace_first(const char *key, int keylen, const char *value, int valuelen) {
    return replace_first(Str(key, keylen), Str(value, valuelen));
  }

  //=====================================================================================

  //merge
  //=====================================================================================
  bool merge_uv() {
    /*
    std::cout << "MEMORY CONSUMPTION = " << memory_consumption() << "\n";
    std::cout << "TREE STATS===============================================\n";
    tree_stats();
    std::cout << "TREE STATS END===========================================\n";
    std::cout << "STATIC TREE STATS========================================\n";
    tree_stats();
    std::cout << "STSTIC TREE STATS END====================================\n";
    */
    //std::cout << "ic = " << ic << "\n";
    //std::cout << "sic = " << sic << "\n";
    if (first_merge) {
      /*
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
      if (((int)(keys.size()) != ic) || ((int)(values.size()) != ic))
	return false;
      for (unsigned int j = 0; j < keys.size(); j++)
	static_put(keys[j], values[j]);

      q_[0].run_buildStatic(static_table_->table(), *sti_);
      //TODO
      //delete dynamic part
      //static_table_->destroy(*sti_);
      first_merge = false;
      */
      q_[0].run_buildStatic(table_->table(), *ti_);
      //static_table_->table().set_static_root(table_->table().static_root());
      //table_->table().set_static_root(NULL);
      q_[0].run_merge(static_table_->table(), table_->table(), *sti_, *ti_);
      sic += ic;
      first_merge = false;
    }
    else {
      q_[0].run_buildStatic(table_->table(), *ti_);
      q_[0].run_merge(static_table_->table(), table_->table(), *sti_, *ti_);
      sic += ic;
    }
    //print_items();
    reset();
    //static_print_items();
    return true;
  }

  bool merge_nuv() {
    //std::cout << "ic = " << ic << "\n";
    //std::cout << "sic = " << sic << "\n";
    if (first_merge) {
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
      //if ((keys.size() != ic) || (values.size() != ic))
      //return false;
      for (int j = 0; j < keys.size(); j++)
	static_put_nuv(keys[j], values[j]);

      q_[0].run_buildStatic_multivalue(static_table_->table(), *sti_);
      //static_table_->destroy(*sti_);
      first_merge = false;
    }
    else {
      q_[0].run_buildStatic_multivalue(table_->table(), *ti_);
      q_[0].run_merge_multivalue(static_table_->table(), table_->table(), *sti_, *ti_);
      sic += ic;
    }
    //print_items();
    reset();
    //static_print_items();
    return true;
  }

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
  //=====================================================================================

  int memory_consumption () const {
    /*
    std::cout << "pool_alloc = " << ti_->pool_alloc << "\n";
    std::cout << "pool_dealloc = " << ti_->pool_dealloc << "\n";
    std::cout << "pool_dealloc_rcu = " << ti_->pool_dealloc_rcu << "\n";
    std::cout << "alloc = " << ti_->alloc << "\n";
    std::cout << "dealloc = " << ti_->dealloc << "\n";
    std::cout << "dealloc_rcu = " << ti_->dealloc_rcu << "\n";
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
	    - sti_->dealloc_rcu);
    //return (ti_->pool_alloc + ti_->alloc - ti_->pool_dealloc - ti_->dealloc);
  }

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
  /*
  void static_tree_stats () {
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

  void static_multivalue_tree_stats () {
    q_[0].run_static_multivalue_stats(static_table_->table());
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
  int get_ic () {
    return ic;
  }

  int get_sic () {
    return sic;
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
  bool first_merge;
  int merge_threshold;
  int merge_ratio;
};

#endif //MTINDEXAPI_H
