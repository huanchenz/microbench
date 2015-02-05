#include <iostream>
#include "hybrid_index/mtIndexAPI.hh"

int main () {
  mt_index<Masstree::default_table> mti;
  mti.setup(false);

  mti.put_uv("huanchenhuanchen", 16, "yingjie0", 8);
  mti.put_uv("zhuozhuozhuo", 12, "yangzi00", 8);
  mti.put_uv("julianjulianjulian", 18, "wenlu000", 8);
  mti.put_uv("peterpeterpeter", 15, "beibei00", 8);
  mti.put_uv("davedavedave", 12, "erica000", 8);

  Str value;
  bool get_success;
  get_success = mti.get("huanchenhuanchen", 16, value);
  std::cout << get_success << "\t" << value.s << "\n";
  get_success = mti.get("zhuozhuozhuo", 12, value);
  std::cout << get_success << "\t" << value.s << "\n";
  get_success = mti.get("julianjulianjulian", 18, value);
  std::cout << get_success << "\t" << value.s << "\n";
  get_success = mti.get("peterpeterpeter", 15, value);
  std::cout << get_success << "\t" << value.s << "\n";
  get_success = mti.get("davedavedave", 12, value);
  std::cout << get_success << "\t" << value.s << "\n";
  get_success = mti.get("michael", 7, value);
  std::cout << get_success << "\t" << value.s << "\n";

  return 0;
}
