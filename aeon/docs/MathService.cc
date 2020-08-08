#include "MathService.h"

#include <sstream>

using namespace std;

int MathService::add(mace::deque<int> values) {
  int sum = 0;
  for (size_t i = 0; i < values.size(); i++) {
    sum += values[i];
  }

  return sum;
}

StringIntHMap MathService::sumAndDifference(int x, int y) {
  StringIntHMap r;
  r["sum"] = x + y;
  r["difference"] = x - y;
  return r;
}
