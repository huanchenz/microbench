#include "microbench.hh"

#define NUM_INTERVALS 50

int main(int argc, char *argv[]) {
  std::ifstream infile_load("workloads/loada_zipf_url_100M.dat");

  HybridType hybrid;
  hybrid.setup(KEY_LEN_STR, false);

  std::string op;
  std::string key;

  std::vector<std::string> init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  std::vector<std::string> keys;

  std::string insert("INSERT");

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
  //INSERT ONLY TEST
  double times[NUM_INTERVALS];
  double tputs[NUM_INTERVALS];
  double mem[NUM_INTERVALS];
  count = 0;
  int interval_count = 0;
  int size = init_keys.size();
  double exact_start_time = get_now();
  double start_time = get_now();
  while (count < size) {
    uint64_t* value_ptr = &value;
    if (!hybrid.put_uv((const char*)(init_keys[count].c_str()), init_keys[count].size(), (const char*)value_ptr, 8)) {
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    count++;
    value++;
    if (count % (INIT_LIMIT / NUM_INTERVALS) == 0) {
      times[interval_count] = get_now() - exact_start_time;
      tputs[interval_count] = (INIT_LIMIT / NUM_INTERVALS) / (get_now() - start_time) / 1000000; //Mops/sec
      mem[interval_count] = (hybrid.memory_consumption() + 0.0) / 1000000; //MB
      interval_count++;
      start_time = get_now();      
    }
  }

  //double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  //double memory = (hybrid.memory_consumption() + 0.0) /1000000; //MB
  //std::cout << "hybrid " << "int " << "insert " << tput << "\n";
  //std::cout << "hybrid " << "int " << "memory " << memory << "\n";

  for (int i = 0; i < (NUM_INTERVALS - 1); i++) {
    std::cout << times[i] << " " << tputs[i] << " " << mem[i] << "\n";
  }

  return 0;
}
