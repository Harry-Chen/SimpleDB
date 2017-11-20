#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "../constants.h"
#include <cstdio>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <fstream>

//class BufPageManager;

class FileManager {
  friend class BufPageManager;
  int fileList[MAX_FILE_NUM];
  int filePermID[MAX_FILE_NUM];
  int idStack[MAX_FILE_NUM];
  bool isOpen[MAX_FILE_NUM];
  int idStackTop;
  std::map<std::string, int> permID;
  std::map<int, int> perm2temp;
  int nextID;

  FileManager() {
    idStackTop = 0;
    for (int i = MAX_FILE_NUM - 1; i >= 0; i--) {
      idStack[idStackTop++] = i;
    }
    memset(isOpen, 0, sizeof(isOpen));
    nextID = 0;

    std::ifstream stm("perm.id");
    if (stm.is_open()) {
      std::string tab;
      int id;
      while (stm >> tab >> id) {
        permID[tab] = id;
        if (nextID <= id) nextID = id + 1;
      }
    }
  }
  ~FileManager() {
    std::ofstream stm("perm.id");
    for (auto itm : permID) {
      stm << itm.first << " " << itm.second << "\n";
    }
  }
public:
  void writePage(int fileID, int pageID, char* buf) {
		assert(0 <= fileID && fileID < MAX_FILE_NUM && isOpen[fileID]);
		int file = fileList[fileID];
    off_t offset = pageID;
    offset <<= PAGE_IDX;
    assert(lseek(file, offset, SEEK_SET) == offset);
    assert(write(file, (void*) buf, PAGE_SIZE) == PAGE_SIZE);
  }

  void readPage(int fileID, int pageID, char* buf) {
		assert(0 <= fileID && fileID < MAX_FILE_NUM && isOpen[fileID]);
    int file = fileList[fileID];
    off_t offset = pageID;
    offset <<= PAGE_IDX;
    assert(lseek(file, offset, SEEK_SET) == offset);
    assert(read(file, (void*) buf, PAGE_SIZE) == PAGE_SIZE);
  }

  void createFile(const char* name) {
    FILE* file = fopen(name, "a+");
    assert(file);
    fclose(file);
    permID[name] = nextID++;
  }

  void closeFile(int fileID) {
    assert(isOpen[fileID]);
    isOpen[fileID] = 0;
    int file = fileList[fileID];
    perm2temp.erase(filePermID[fileID]);
    ::close(file);
		idStack[idStackTop++] = fileID;
  }

  // void close() {
  //   for (int i = 0; i < MAX_FILE_NUM; i++) {
  //     if (isOpen[i]) closeFile(i);
  //     isOpen[i] = 0;
  //     idStack[idStackTop++] = i;
  //   }
  // }

  int openFile(const char* name) {
    assert(idStackTop);
    int fileID = idStack[--idStackTop];
    isOpen[fileID] = 1;
    filePermID[fileID] = permID[name];
    perm2temp[filePermID[fileID]] = fileID;
    int file = open(name, O_RDWR);
    assert(file != -1);
    fileList[fileID] = file;
    return fileID;
  }

  int getFilePermID(int fileID) {
    assert(isOpen[fileID]);
    return filePermID[fileID];
  }

  int getFileTempID(int permID) {
    return perm2temp[permID];
  }
};
#endif
