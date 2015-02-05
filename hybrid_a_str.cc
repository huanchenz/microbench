#include "microbench.hh"

int main(int argc, char *argv[]) {
  int merge_threshold = atoi(argv[1]);
  int merge_ratio = atoi(argv[2]);
  std::ifstream infile_load("workloads/loada_int_1M.dat");
  std::ifstream infile_txn("workloads/txnsa_int_1M.dat");

  HybridType hybrid;
  hybrid.setup(false, merge_threshold, merge_ratio);

  std::string op;
  std::string key;

  std::vector<std::string> init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  std::vector<std::string> keys;

  std::string insert("INSERT");
  std::string read("READ");
  std::string update("UPDATE");

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
    uint64_t* value_ptr = &value;
    if (!hybrid.put_uv((const char*)(init_keys[count].c_str()), init_keys[count].size(), (const char*)value_ptr, 8)) {
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    count++;
    value++;
  }
  double end_time = get_now();

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  //double memory = (hybrid.memory_consumption() + 0.0) /1000000; //MB
  std::cout << "hybrid " << "string " << "insert " << tput << "\n";
  //std::cout << "hybrid " << "string " << "memory " << memory << "\n";

  //load txns
  count = 0;
  while ((count < LIMIT) && infile_txn.good()) {
    infile_txn >> op >> key;
    if (op.compare(read) == 0) {
      ops.push_back(1);
      keys.push_back(key);
    }
    else if (op.compare(update) == 0) {
      ops.push_back(2);
      keys.push_back(key);
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      return -1;
    }
    count++;
  }

  //DO TXNS
  start_time = get_now();
  int txn_num = 0;
  value = 0;
  Str val;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    if (ops[txn_num] == 1) { //READ
      if (!hybrid.get((const char*)(keys[txn_num].c_str()), keys[txn_num].size(), val)) {
	std::cout << "READ FAIL\n";
      }
    }
    else if (ops[txn_num] == 2) { //UPDATE
      uint64_t* value_ptr = &value;
      hybrid.put((const char*)(keys[txn_num].c_str()), keys[txn_num].size(), (const char*)value_ptr, 8);
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
  if (merge_threshold > LIMIT)
    std::cout << "mt ";
  else if (merge_threshold == LIMIT)
    std::cout << "smt ";
  else
    std::cout << "hybrid ";
  std::cout << "string " << "read/update " << tput << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
