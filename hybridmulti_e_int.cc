#include "microbench.hh"

int main(int argc, char *argv[]) {
  std::ifstream infile_load("workloads/loade_zipf_int_100M.dat");
  std::ifstream infile_txn("workloads/txnse_zipf_int_100M.dat");

  HybridType hybrid;
  hybrid.setup(KEY_LEN, KEY_LEN, true);

  std::string op;
  uint64_t key;
  int range;

  std::vector<uint64_t> init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  std::vector<uint64_t> keys;
  std::vector<int> ranges;

  std::string insert("INSERT");
  std::string scan("SCAN");

  int count = 0;
  uint64_t value = 0;
  //read init file
  while ((count < INIT_LIMIT) && infile_load.good()) {
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
    uint64_t* value_ptr;
    for (int i = 0; i < VALUES_PER_KEY; i++) {
      value_ptr = &value;
      hybrid.put_nuv((const char*)key_ptr, 8, (const char*)value_ptr, 8);
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

  if (HYBRID >= 0)
    hybrid.merge(); //hack

  //DO TXNS
  start_time = get_now();
  int txn_num = 0;
  Str dynamic_val;
  Str static_val;
  Str val;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    uint64_t* key_ptr = &(keys[txn_num]);
    if (ops[txn_num] == 0) { //INSERT
      uint64_t* value_ptr = &value;
      hybrid.put_nuv((const char*)key_ptr, 8, (const char*)value_ptr, 8);
      value++;
    }
    else if (ops[txn_num] == 1) { //SCAN
      if (!hybrid.get_ordered_nuv((const char*)key_ptr, 8, dynamic_val, static_val)) {
	std::cout << "SCAN FIRST READ FAIL\n";
      }
      for (int i = 0; i < ranges[txn_num]; i++) {
	if (!hybrid.get_next_nuv(val)) {
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

  if (HYBRID > 0)
    std::cout << "hybrid ";
  else if (HYBRID < 0)
    std::cout << "mt ";
  else
    std::cout << "cmt ";
  std::cout << "int " << "scan " << tput << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
