#include "microbench.hh"

int main(int argc, char *argv[]) {
  std::ifstream infile_load("workloads/loada_zipf_int_100M.dat");

  HybridType hybrid;
  hybrid.setup(KEY_LEN, false);

  std::string op;
  uint64_t key;

  std::vector<uint64_t> init_keys;
  std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2
  std::vector<uint64_t> keys;

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
  int merge_ratio = 10;
  int merge_threshold = 100;

  int ic = 0;
  int sic = 0;
  count = 0;
  double start_time = 0.0;
  double end_time = 0.0;
  bool isMerge = false;
  while (count < init_keys.size()) {
    uint64_t* key_ptr = &(init_keys[count]);
    uint64_t* value_ptr = &value;
    ic++;
    if ((((ic * merge_ratio) >= sic) || (merge_ratio == 0)) && (ic >= merge_threshold))
      isMerge = true;

    if (isMerge)
      start_time = get_now();
    if (!hybrid.put_uv((const char*)key_ptr, 8, (const char*)value_ptr, 8)) {
      std::cout << "LOAD FAIL!\n";
      return -1;
    }
    if (isMerge) {
      end_time = get_now();      
      std::cout << hybrid.memory_consumption() << " " << (end_time - start_time) << "\n";
      sic += ic;
      ic = 0;
    }
    count++;
    value++;
    isMerge = false;
  }

  return 0;
}
