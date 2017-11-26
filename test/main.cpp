#include <stx/btree_multiset.h>

class A {
  int x, y, z;
};

stx::btree_multiset<A> s;

int main() {
  A a;
  a.x = 0;
  a.y = 0;
  a.z = 0;
  s.insert(a);
  return 0;
}
