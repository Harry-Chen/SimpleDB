#ifndef __MULTI_LIST_H__
#define __MULTI_LIST_H__

#include <cassert>

class MultiList {
private:
  struct ListNode {
    int next;
    int prev;
  };
  int cap;
  int listNum;
  ListNode* a;
  void link(int prev, int next) {
    a[prev].next = next;
    a[next].prev = prev;
  }
public:
  MultiList(int itemSize, int listSize) {
    cap = itemSize;
    listNum = listSize;
    a = new ListNode[cap + listNum];
    for (int i = 0; i < cap + listNum; i++) {
      a[i].next = i;
      a[i].prev = i;
    }
  }
  ~MultiList() {
    delete [] a;
  }
  void erase(int index) {
    assert(0 <= index && index < cap);
    if (a[index].prev == index) {
      return;
    }
    link(a[index].prev, a[index].next);
    a[index].prev = index;
    a[index].next = index;
  }
  void insert(int listID, int ele) {
    assert(0 <= ele && ele < cap);
    assert(0 <= listID && listID < listNum);
    erase(ele);
    int node = listID + cap;
    int prev = a[node].prev;
    link(prev, ele);
    link(ele, node);
  }
  void insertFirst(int listID, int ele) {
    assert(0 <= ele && ele < cap);
    assert(0 <= listID && listID < listNum);
    erase(ele);
    int node = listID + cap;
    int next = a[node].next;
    link(node, ele);
    link(ele, next);
  }
  int getFirst(int listID) {
    assert(0 <= listID && listID < listNum);
    return a[listID + cap].next;
  }
  int next(int index) {
    assert(0 <= index && index <= cap + listNum);
    return a[index].next;
  }
  bool isHead(int index) {
    assert(0 <= index && index <= cap + listNum);
    if (index < cap) {
      return false;
    } else {
      return true;
    }
  }
  bool isAlone(int index) {
    assert(0 <= index && index <= cap + listNum);
    return (a[index].next == index);
  }
};
#endif
