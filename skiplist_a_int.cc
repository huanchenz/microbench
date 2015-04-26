#include "microbench.hh"

int main() {

  std::ifstream infile_load("workloads/loada_zipf_int_100M.dat");
  std::ifstream infile_txn("workloads/txnsa_zipf_int_100M.dat");

  SkipListType_int skiplist;
  SkipListType_int::const_iterator skiplist_keyIter;

  std::string op;
  uint64_t key;

  std::vector<IntsKeyValue> init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  std::vector<IntsKeyValue> keys;

  std::string insert("INSERT");
  std::string read("READ");
  std::string update("UPDATE");

  int count = 0;
  uint64_t value = 0;

  IntsKeyValue key_value_int;
  //read init file
  while ((count < INIT_LIMIT) && infile_txn.good()) {
    infile_load >> op >> key;
    if (op.compare(insert) != 0) {
      std::cout << "READING LOAD FILE FAIL!\n";
      return -1;
    }
    //init_keys.push_back(key);
    key_value_int.key = key;
    key_value_int.value = value;
    init_keys.push_back(key_value_int);
    count++;
    value++;
  }

  //initial load
  //WRITE ONLY TEST
  count = 0;
  double start_time = get_now();
  while (count < (int)init_keys.size()) {
    SkipListType_int::insert_by_value_result retval = skiplist.insert(init_keys[count]);
    if (retval.second == false) {
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    count++;
  }
  double end_time = get_now();

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  std::cout << "skiplist " << "int " << "insert " << tput << "\n";

  //load txns
  count = 0;
  while ((count < LIMIT) && infile_txn.good()) {
    infile_txn >> op >> key;
    if (op.compare(read) == 0) {
      ops.push_back(1);
      //keys.push_back(key);
      key_value_int.key = key;
      keys.push_back(key_value_int);
    }
    else if (op.compare(update) == 0) {
      ops.push_back(2);
      //keys.push_back(key);
      key_value_int.key = key;
      keys.push_back(key_value_int);
    }
    else {
      std::cout << "UNRECOGNIZED CMD (load)!\n";
      return -1;
    }
    count++;
  }

  //READ
  start_time = get_now();
  int txn_num = 0;
  value = 0;
  uint64_t sum;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    if (ops[txn_num] == 1) { //READ
      /*
      if (skiplist.find(keys[txn_num]) == skiplist.end()) {
	std::cout << "READ FAIL\n";
      }
      */
      skiplist.find(keys[txn_num]);
    }
    else if (ops[txn_num] == 2) { //UPDATE
      skiplist.impl.find(keys[txn_num])->value.value = value;
      value++;
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
  std::cout << "skiplist " << "int " << "read/update " << tput << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
