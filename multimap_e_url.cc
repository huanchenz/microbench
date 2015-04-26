#include "microbench.hh"

int main() {
  std::ifstream infile_load("workloads/loade_zipf_url_100M.dat");
  std::ifstream infile_txn("workloads/txnse_zipf_url_100M.dat");

  //MapType stdmap;
  //MapType::const_iterator stdmap_keyIter;

  int64_t memory = 0;
  AllocatorType_str *alloc = new AllocatorType_str(&memory);

  MapType_multi_str_alloc *multimap = new MapType_multi_str_alloc(std::less<std::string>(), (*alloc));
  std::pair<MapType_multi_str_alloc::const_iterator, MapType_multi_str_alloc::const_iterator> multimap_keyIter;

  MapType_multi_str_alloc::const_iterator multimap_keyIter_seq;

  std::string op;
  std::string key;
  int range;

  std::vector<std::string> init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  std::vector<std::string> keys;
  std::vector<int> ranges;

  std::string insert("INSERT");
  std::string scan("SCAN");

  int count = 0;
  uint64_t value = 0;

  //read init file
  while ((count < INIT_LIMIT) && infile_txn.good()) {
    infile_load >> op >> key;
    if (op.compare(insert) != 0) {
      std::cout << "READING LOAD FILE FAIL!\n";
      return -1;
    }
    init_keys.push_back(key);
    count++;
  }

  //initial load
  //WRITE ONLY TEST
  count = 0;
  double start_time = get_now();
  while (count < (int)init_keys.size()) {
    for (int i = 0; i < VALUES_PER_KEY; i++) {
      multimap->insert(std::pair<std::string, uint64_t>(init_keys[count], value));
      value++;
    }
    count++;
  }
  double end_time = get_now();

  double tput = count * VALUES_PER_KEY / (end_time - start_time) / 1000000; //Mops/sec
  //std::cout << tput << "\n";

  //load txns
  count = 0;
  while ((count < LIMIT) && infile_txn.good()) {
    infile_txn >> op >> key;
    if (op.compare(insert) == 0) { //INSERT
      ops.push_back(0);
      keys.push_back(key);
      ranges.push_back(1);
    }
    else if (op.compare(scan) == 0) { //SCAN
      infile_txn >> range;
      ops.push_back(1);
      keys.push_back(key);
      ranges.push_back(range);
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      return -1;
    }
    count++;
  }

  //SCAN/INSERT
  start_time = get_now();
  int txn_num = 0;
  uint64_t sum;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    if (ops[txn_num] == 0) { //INSERT
      multimap->insert(std::pair<std::string, uint64_t>(keys[txn_num], value));
      value++;
    }
    else if (ops[txn_num] == 1) { //SCAN
      multimap_keyIter_seq = multimap->lower_bound(keys[txn_num]);
      for (int i = 0; i < ranges[txn_num] * VALUES_PER_KEY; i++) {
	++(multimap_keyIter_seq);
	sum += multimap_keyIter_seq->second;
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
  std::cout << "multimap " << "url " << "scan " << tput << " " << sum << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
