#include "microbench.hh"

int main(int argc, char *argv[]) {
  std::ifstream infile_load("workloads/loadc_zipf_url_100M.dat");
  std::ifstream infile_txn("workloads/txnsc_zipf_url_100M.dat");

  HybridType hybrid;
  hybrid.setup(KEY_LEN_URL, false);

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
  while ((count < INIT_LIMIT) && infile_load.good()) {
    infile_load >> op >> key;
    if (op.compare(insert) != 0) {
      std::cout << "READING LOAD FILE FAIL!\n";
      return -1;
    }
    init_keys.push_back(key);
    count++;
  }

  //std::cout << "start\n";
  //initial load
  //WRITE ONLY TEST
  count = 0;
  double start_time = get_now();
  while (count < (int)init_keys.size()) {
  //while (count < 10000) {
    //std::cout << "count = " << count << "\n";
    uint64_t* value_ptr = &value;
    if (!hybrid.put_uv((const char*)(init_keys[count].c_str()), init_keys[count].size(), (const char*)value_ptr, 8)) {
      std::cout << init_keys[count] << "\n";
      std::cout << count << "===========\n";
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    //if ((count % 1000000) == 0)
    //std::cout << count << "\n";
    count++;
    value++;
  }
  double end_time = get_now();

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  double memory = (hybrid.memory_consumption() + 0.0) /1000000; //MB
  if (HYBRID > 0)
    std::cout << "hybrid ";
  else if (HYBRID < 0)
    std::cout << "mt ";
  else
    std::cout << "cmt ";
  std::cout << "url " << "memory " << memory << "\n";
  //std::cout << tput << "\n";

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

  if (HYBRID >= 0)
    hybrid.merge(); //hack

  //DO TXNS
  start_time = get_now();
  int txn_num = 0;
  value = 0;
  while ((txn_num < LIMIT) && (txn_num < (int)ops.size())) {
    Str val;
    if (ops[txn_num] == 1) { //READ
      if (!hybrid.get((const char*)(keys[txn_num].c_str()), keys[txn_num].size(), val)) {
	//std::cout << txn_num << "\n";
	std::cout << keys[txn_num] << "\n";
	std::cout << txn_num << "===========\n";
	std::cout << "READ FAIL\n";
	return -1;
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
  std::cout << "url " << "read " << tput << "\n";
  //std::cout << "time elapsed = " << (end_time - start_time) << "\n";

  return 0;
}
