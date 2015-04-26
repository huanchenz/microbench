#include "microbench.hh"

int main() {
  std::ifstream infile_load("workloads/loadc_zipf_int_100M.dat");
  std::ifstream infile_txn("workloads/txnsc_zipf_int_100M.dat");

  //MapType stdmap;
  //MapType::const_iterator stdmap_keyIter;

  int64_t memory = 0;
  AllocatorType_gen *alloc = new AllocatorType_gen(&memory);

  MapType_multi_gen_alloc *multimap = new MapType_multi_gen_alloc(GenericComparator<21>(), (*alloc));
  std::pair<MapType_multi_gen_alloc::const_iterator, MapType_multi_gen_alloc::const_iterator> multimap_keyIter;

  std::string op;
  std::string key;

  std::vector<GenericKey<21> > init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
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
    //if (count % 1000000 == 0)
    //std::cout << count << "\n";
    for (int i = 0; i < VALUES_PER_KEY; i++) {
      multimap->insert(std::pair<GenericKey<21>, uint64_t>(init_keys[count], value));
      value++;
    }
    count++;
  }
  double end_time = get_now();

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  //std::cout << tput << "\n";
  std::cout << "multimap " << "string " << "memory " << ((memory + 0.0)/1000000) << "\n";
  //std::cout << "stdmap " << "int " << "memory " << (memory + 0.0) << "\n";

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
      //stdmap_keyIter = stdmap->find(keys[txn_num]);
      multimap_keyIter = multimap->equal_range(keys[txn_num]);
      if (multimap_keyIter.first == multimap_keyIter.second) {
	std::cout << "READ FAIL\n";
      }
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      return -1;
    }
    txn_num++;
  }
  end_time = get_now();

  tput = txn_num / (end_time - start_time) / 1000000; //Mops/sec
  std::cout << "multimap " << "string " << "read " << (tput + (sum - sum)) << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
