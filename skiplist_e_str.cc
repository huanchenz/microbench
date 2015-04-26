#include "microbench.hh"

int main() {
  std::ifstream infile_load("workloads/loade_zipf_int_100M.dat");
  std::ifstream infile_txn("workloads/txnse_zipf_int_100M.dat");

  SkipListType_str skiplist;
  SkipListType_str::const_iterator skiplist_keyIter;

  std::string op;
  std::string key;
  int range;

  std::vector<GenericKeyValue<21> > init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  std::vector<GenericKeyValue<21> > keys;
  std::vector<int> ranges;

  std::string insert("INSERT");
  std::string scan("SCAN");

  int count = 0;
  uint64_t value = 0;

  GenericKeyValue<21> key_value_gen;
  //read init file
  while ((count < INIT_LIMIT) && infile_txn.good()) {
    infile_load >> op >> key;
    if (op.compare(insert) != 0) {
      std::cout << "READING LOAD FILE FAIL!\n";
      return -1;
    }
    key_value_gen.setKeyFromString(key);
    key_value_gen.value = value;
    init_keys.push_back(key_value_gen);
    count++;
    value++;
  }

  //initial load
  //WRITE ONLY TEST
  count = 0;
  double start_time = get_now();
  while (count < (int)init_keys.size()) {
    SkipListType_str::insert_by_value_result retval = skiplist.insert(init_keys[count]);
    if (retval.second == false) {
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    count++;
  }
  double end_time = get_now();

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec

  //load txns
  count = 0;
  while ((count < LIMIT) && infile_txn.good()) {
    infile_txn >> op >> key;
    if (op.compare(insert) == 0) {
      ops.push_back(0);
      key_value_gen.setKeyFromString(key += "0");
      key_value_gen.value = value;
      keys.push_back(key_value_gen);
      ranges.push_back(1);
      value++;
    }
    else if (op.compare(scan) == 0) {
      infile_txn >> range;
      ops.push_back(1);
      key_value_gen.setKeyFromString(key);
      keys.push_back(key_value_gen);
      ranges.push_back(range);
    }
    else {
      std::cout << "UNRECOGNIZED CMD (load)!\n";
      return -1;
    }
    count++;
  }

  //SCAN/INSERT
  start_time = get_now();
  int txn_num = 0;
  value = 0;
  uint64_t sum;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    if (ops[txn_num] == 0) { //INSERT
      SkipListType_str::insert_by_value_result retval = skiplist.insert(keys[txn_num]);
      if (retval.second == false) {
	std::cout << "INSERT FAIL!\n";
      }
    }
    else if (ops[txn_num] == 1) { //SCAN
      skiplist_keyIter = skiplist.find(keys[txn_num]);
      sum += skiplist_keyIter.get_node()->value.value;
      if (skiplist_keyIter == skiplist.end()) {
	std::cout << "SCAN FIRST READ FAIL\n";
      }
      for (int i = 0; i < ranges[txn_num]; i++) {
	++skiplist_keyIter;
	if (skiplist_keyIter == skiplist.end()) {
	  //std::cout << "SCAN FAIL\n";
	  break;
	}
	sum += skiplist_keyIter.get_node()->value.value;
      }      
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      std::cout << ops[txn_num] << "\n";
      return -1;
    }
    txn_num++;
  }
  end_time = get_now();

  tput = txn_num / (end_time - start_time) / 1000000; //Mops/sec
  std::cout << sum << "\n";
  std::cout << "skiplist " << "string " << "scan " << tput << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
