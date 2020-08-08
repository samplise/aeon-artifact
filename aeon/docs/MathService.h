#ifndef _MATH_SERVICE_H
#define _MATH_SERVICE_H

#include "mhash_map.h"
#include "mdeque.h"
#include "mstring.h"
#include "Collections.h"

typedef mace::hash_map<int, mace::string> IntMap;

class MathService {
public:
  virtual ~MathService() { }
  virtual int add(mace::deque<int> values);
  virtual StringIntHMap sumAndDifference(int x, int y);
};

#endif // _MATH_SERVICE_H
