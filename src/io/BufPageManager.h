#ifndef __BUF_PAGE_MANAGER__
#define __BUF_PAGE_MANAGER__

#include "FileManager.h"
#include "FindReplace.h"
#include "../util/HashMap.h"

class BufPageManager {
private:
    int last;
    HashMap *hash;
    MultiList *list;
    FindReplace *replace;
    int id2key[BUF_CAPACITY][2];
    bool dirty[BUF_CAPACITY];
    char *buf;
    FileManager *fileManager;

    char *getBuf(int index) {
        return buf + index * PAGE_SIZE;
    }

    int fetchPage(int fileID, int pageID) {
        int index = replace->find();
        if (dirty[index]) {
            int k1, k2;
            hash->getKeys(index, k1, k2);
            fileManager->writePage(k1, k2, getBuf(index));
            dirty[index] = false;
        }
        hash->replace(index, fileID, pageID);
        list->insert(fileID, index);
        return index;
    }

    BufPageManager() {
        buf = new char[BUF_CAPACITY * PAGE_SIZE];
        fileManager = new FileManager;
        replace = new FindReplace();
        hash = new HashMap(BUF_CAPACITY);
        list = new MultiList(BUF_CAPACITY, MAX_FILE_NUM);
        last = -1;
        memset(dirty, 0, sizeof(dirty));
    }

    BufPageManager(BufPageManager const &);

    BufPageManager &operator=(BufPageManager const &);

    ~BufPageManager() {
        delete replace;
        delete hash;
        delete list;
        delete fileManager;
        delete buf;
    }

public:
    static BufPageManager &getInstance() {
        static BufPageManager instance;
        return instance;
    }

    static FileManager &getFileManager() {
        return *(getInstance().fileManager);
    }

    int allocPage(int fileID, int pageID, bool ifRead = false) {
        int index = fetchPage(fileID, pageID);
        if (ifRead) {
            fileManager->readPage(fileID, pageID, getBuf(index));
        }
        return index;
    }

    int getPage(int fileID, int pageID) {
        int index = hash->findIndex(fileID, pageID);
        if (index != -1) {
            access(index);
        } else {
            index = fetchPage(fileID, pageID);
            fileManager->readPage(fileID, pageID, getBuf(index));
        }
        return index;
    }

    char *access(int index) {
        if (index != last) {
            replace->access(index);
            last = index;
        }
        return getBuf(index);
    }

    void markDirty(int index) {
        dirty[index] = true;
        access(index);
    }

    // withdraw without writeback
    void release(int index) {
        dirty[index] = false;
        replace->free(index);
        hash->erase(index);
        list->erase(index);
    }

    void writeBack(int index) {
        if (dirty[index]) {
            int f, p;
            hash->getKeys(index, f, p);
            fileManager->writePage(f, p, getBuf(index));
            dirty[index] = false;
        }
        replace->free(index);
        hash->erase(index);
        list->erase(index);
    }

    void closeFile(int fileID, bool ifWrite = true) {
        int index;
        while (!list->isHead(index = list->getFirst(fileID))) {
            if (ifWrite) {
                writeBack(index);
            } else {
                release(index);
            }
        }
    }

    void close() {
        for (int i = 0; i < BUF_CAPACITY; ++i) {
            writeBack(i);
        }
    }
};

#endif
