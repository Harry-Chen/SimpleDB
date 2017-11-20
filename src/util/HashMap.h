#ifndef __HASH_MAP_H__
#define __HASH_MAP_H__
#include "MultiList.h"

class HashMap {
private:
	struct DataNode {
		int key1, key2;
	};
	static const int base = 97;
	static const int mod = 48611; // prime[5000]
	int cap;
	MultiList* list;
	DataNode* a;

	int hash(int k1, int k2) {
		return (k1 + (long long)k2 * base) % mod;
	}

public:
	HashMap(int c) {
		cap = c;
		a = new DataNode[c];
		for (int i = 0; i < cap; i++) {
			a[i].key1 = -1;
			a[i].key2 = -1;
		}
		list = new MultiList(cap, mod);
	}

	~HashMap() {
		delete list;
		delete [] a;
	}

	// return -1 when fail
	int findIndex(int k1, int k2) {
		int h = hash(k1, k2);
		int p = list->getFirst(h);
		while (!list->isHead(p)) {
			if (a[p].key1 == k1 && a[p].key2 == k2) {
				return p;
			}
			p = list->next(p);
		}
		return -1;
	}

	void replace(int index, int k1, int k2) {
		int h = hash(k1, k2);
		list->insertFirst(h, index);
		a[index].key1 = k1;
		a[index].key2 = k2;
	}

	void erase(int index) {
		list->erase(index);
		a[index].key1 = -1;
		a[index].key2 = -1;
	}

	void getKeys(int index, int& k1, int& k2) {
		k1 = a[index].key1;
		k2 = a[index].key2;
	}

};
#endif
