#include "microbench.hh"

int main(int argc, char *argv[]) {
  int merge_threshold = atoi(argv[1]);
  int merge_ratio = atoi(argv[2]);
  std::ifstream infile_load("workloads/loade_zipf_int_1M.dat");
  std::ifstream infile_txn("workloads/txnse_zipf_int_1M.dat");

  HybridType hybrid;
  hybrid.setup(KEY_LEN, false, merge_threshold, merge_ratio);

  std::string op;
  uint64_t key;
  int range;

  std::vector<uint64_t> init_keys;
  std::vector<int> ops; //INSERT = 0, SCAN = 1
  std::vector<uint64_t> keys;
  std::vector<int> ranges;

  std::string insert("INSERT");
  std::string scan("SCAN");

  int count = 0;
  uint64_t value = 0;
  //read init file
  while ((count < LIMIT) && infile_load.good()) {
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
    uint64_t* key_ptr = &(init_keys[count]);
    uint64_t* value_ptr = &value;
    if (!hybrid.put_uv((const char*)key_ptr, 8, (const char*)value_ptr, 8)) {
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
      keys.push_back(key + 1);
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

  if (merge_threshold <= LIMIT)
    hybrid.merge(); //hack

  //SCAN/INSERT
  start_time = get_now();
  int txn_num = 0;
  value = 0;
  Str val;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    uint64_t* key_ptr = &(keys[txn_num]);
    if (ops[txn_num] == 0) { //INSERT
      /*
      uint64_t* value_ptr = &value;
      if (!hybrid.put_uv((const char*)key_ptr, 8, (const char*)value_ptr, 8)) {
	std::cout << "INSERT FAIL!\n";
      }
      value++;
      */
    }
    else if (ops[txn_num] == 1) { //SCAN
      if (!hybrid.get_ordered((const char*)key_ptr, 8, val)) {
	std::cout << "SCAN FIRST READ FAIL\n";
      }
      for (int i = 0; i < ranges[txn_num]; i++) {
	if (!hybrid.get_next(val)) {
	  //std::cout << "SCAN FAIL\n";
	  break;
	}
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
  if (merge_threshold > LIMIT)
    std::cout << "mt ";
  else if (merge_threshold == LIMIT)
    std::cout << "smt ";
  else
    std::cout << "hybrid ";
  std::cout << "int " << "scan " << tput << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  //std::cout << "hybrid " << "int " << "dynamichit " << hybrid.get_mt_hit() << "\n";
  //std::cout << "hybrid " << "int " << "statichit " << hybrid.get_cmt_hit() << "\n";

  return 0;
}
