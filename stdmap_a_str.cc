#include "microbench.hh"

int main() {
  std::ifstream infile_load("workloads/loada_zipf_int_100M.dat");
  std::ifstream infile_txn("workloads/txnsa_zipf_int_100M.dat");

  //MapType_str stdmap;
  //MapType_str::const_iterator stdmap_keyIter;
  MapType_gen stdmap;
  MapType_gen::const_iterator stdmap_keyIter;

  std::string op;
  std::string key;

  //std::vector<std::string> init_keys;
  std::vector<GenericKey<21> > init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  //std::vector<std::string> keys;
  std::vector<GenericKey<21> > keys;

  std::string insert("INSERT");
  std::string read("READ");
  std::string update("UPDATE");

  int count = 0;
  uint64_t value = 0;

  GenericKey<21> key_gen;
  //read init file
  while ((count < INIT_LIMIT) && infile_txn.good()) {
    infile_load >> op >> key;
    if (op.compare(insert) != 0) {
      std::cout << "READING LOAD FILE FAIL!\n";
      return -1;
    }
    //init_keys.push_back(key);
    key_gen.setFromString(key);
    init_keys.push_back(key_gen);
    count++;
  }

  //initial load
  //WRITE ONLY TEST
  count = 0;
  double start_time = get_now();
  while (count < (int)init_keys.size()) {
    //std::pair<typename MapType_str::iterator, bool> retval =
    //stdmap.insert(std::pair<std::string, uint64_t>(init_keys[count], value));
    std::pair<typename MapType_gen::iterator, bool> retval =
      stdmap.insert(std::pair<GenericKey<21>, uint64_t>(init_keys[count], value));
    if (retval.second == false) {
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    count++;
    value++;
  }
  double end_time = get_now();

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  std::cout << "stdmap " << "string " << "insert " << tput << "\n";

  //load txns
  count = 0;
  while ((count < LIMIT) && infile_txn.good()) {
    infile_txn >> op >> key;
    if (op.compare(read) == 0) {
      ops.push_back(1);
      //keys.push_back(key);
      key_gen.setFromString(key);
      keys.push_back(key_gen);
    }
    else if (op.compare(update) == 0) {
      ops.push_back(2);
      //keys.push_back(key);
      key_gen.setFromString(key);
      keys.push_back(key_gen);
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      return -1;
    }
    count++;
  }

  //READ/UPDATE
  start_time = get_now();
  int txn_num = 0;
  value = 0;
  uint64_t sum = 0;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    if (ops[txn_num] == 1) { //READ
      stdmap_keyIter = stdmap.find(keys[txn_num]);
      if (stdmap_keyIter == stdmap.end()) {
	std::cout << "READ FAIL\n";
      }
      sum += stdmap_keyIter->second;
    }
    else if (ops[txn_num] == 2) { //UPDATE
      stdmap[keys[txn_num]] = value;
      value++;
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      return -1;
    }
    txn_num++;
  }
  end_time = get_now();

  tput = txn_num / (end_time - start_time) / 1000000; //Mops/sec
  std::cout << "stdmap " << "string " << "read/update " << (tput + (sum - sum)) << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
