#include "microbench.hh"

int main() {
  std::ifstream infile_load("workloads/loadc_zipf_int_100M.dat");
  std::ifstream infile_txn("workloads/txnsc_zipf_int_100M.dat");
  //std::ifstream infile_load("workloads/loadc_zipf_int_1M.dat");
  //std::ifstream infile_txn("workloads/txnsc_zipf_int_1M.dat");

  //BtreeType_str stxbtree;
  //BtreeType_str::const_iterator stxbtree_keyIter;

  int64_t memory = 0;
  //AllocatorType *alloc = new AllocatorType(&memory);
  AllocatorType_gen *alloc = new AllocatorType_gen(&memory);

  //BtreeType_str_alloc *stxbtree = new BtreeType_str_alloc(std::less<std::string>(), (*alloc));
  //BtreeType_str_alloc::const_iterator stxbtree_keyIter;
  BtreeType_gen_alloc *stxbtree = new BtreeType_gen_alloc(GenericComparator<21>(), (*alloc));
  BtreeType_gen_alloc::const_iterator stxbtree_keyIter;

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
  int64_t memory_string = 0;

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
    memory_string += key.capacity();
    count++;
  }

  //initial load
  //WRITE ONLY TEST
  count = 0;
  double start_time = get_now();
  while (count < (int)init_keys.size()) {
    //std::pair<typename BtreeType_str_alloc::iterator, bool> retval =
    //stxbtree->insert(std::pair<std::string, uint64_t>(init_keys[count], value));
    std::pair<typename BtreeType_gen_alloc::iterator, bool> retval =
      stxbtree->insert(std::pair<GenericKey<21>, uint64_t>(init_keys[count], value));
    if (retval.second == false) {
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    count++;
    value++;
  }
  double end_time = get_now();

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  //std::cout << tput << "\n";
  //std::cout << "stxbtree " << "string " << "memory " << ((memory + memory_string + 0.0)/1000000) << "\n";
  std::cout << "stxbtree " << "string " << "memory " << ((memory + 0.0)/1000000) << "\n";

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

  //READ
  start_time = get_now();
  int txn_num = 0;
  value = 0;
  uint64_t sum;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    if (ops[txn_num] == 1) { //READ
      stxbtree_keyIter = stxbtree->find(keys[txn_num]);
      if (stxbtree_keyIter == stxbtree->end()) {
	std::cout << "READ FAIL\n";
      }
      sum += stxbtree_keyIter->second;
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      return -1;
    }
    txn_num++;
  }
  end_time = get_now();

  tput = txn_num / (end_time - start_time) / 1000000; //Mops/sec
  std::cout << "stxbtree " << "string " << "read " << (tput + (sum - sum)) << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
