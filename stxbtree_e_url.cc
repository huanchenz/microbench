#include "microbench.hh"

int main() {
  std::ifstream infile_load("workloads/loade_url_1M.dat");
  std::ifstream infile_txn("workloads/txnse_url_1M.dat");

  BtreeType_str stxbtree;
  BtreeType_str::const_iterator stxbtree_keyIter;

  std::string op;
  std::string key;
  int range;

  std::vector<std::string> init_keys;
  std::vector<int> ops; //INSERT = 0, SCAN = 1
  std::vector<std::string> keys;
  std::vector<int> ranges;

  std::string insert("INSERT");
  std::string scan("SCAN");

  int count = 0;
  uint64_t value = 0;
  //read init file
  while ((count < LIMIT) && infile_txn.good()) {
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
    std::pair<typename BtreeType_str::iterator, bool> retval =
      stxbtree.insert(std::pair<std::string, uint64_t>(init_keys[count], value));
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
  value = 0;
  uint64_t sum = 0;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    if (ops[txn_num] == 0) { //INSERT
      std::pair<typename BtreeType_str::iterator, bool> retval =
	stxbtree.insert(std::pair<std::string, uint64_t>((keys[txn_num] += "0"), value));
      if (retval.second == false) {
	std::cout << "INSERT FAIL!\n";
      }
      value++;
    }
    else if (ops[txn_num] == 1) { //SCAN
      stxbtree_keyIter = stxbtree.find(keys[txn_num]);
      sum += stxbtree_keyIter->second;
      if (stxbtree_keyIter == stxbtree.end()) {
	std::cout << "SCAN FIRST READ FAIL\n";
      }
      for (int i = 0; i < ranges[txn_num]; i++) {
	++stxbtree_keyIter;
	if (stxbtree_keyIter == stxbtree.end()) {
	  //std::cout << "SCAN FAIL\n";
	  break;
	}
	sum += stxbtree_keyIter->second;
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
  std::cout << "stxbtree " << "url " << "scan " << (tput + (sum - sum)) << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
