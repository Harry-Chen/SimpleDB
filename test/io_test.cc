#include "gtest/gtest.h"
#include "../src/io/FileManager.h"
#include "../src/io/BufPageManager.h"
#include <cstdlib>

TEST(FILE_MANAGER, CREATE_FILE) {
  FileManager::getInstance().createFile("helloworld.txt");
  FILE *file = fopen("helloworld.txt", "r");
  ASSERT_TRUE(file);
  fclose(file);
  system("rm helloworld.txt");
}

TEST(FILE_MANAGER, FILE_MANAGER_RDWR) {
  FileManager::getInstance().createFile("helloworld.txt");
  char *buf = new char[PAGE_SIZE];
  char *buf2 = new char[PAGE_SIZE];
  for (int i = 0; i < PAGE_SIZE; i++) buf[i] = rand() & 0xFF;
  int fileId = FileManager::getInstance().openFile("helloworld.txt");
  ASSERT_EQ(fileId, 0);
  FileManager::getInstance().writePage(fileId, 15, buf);
  memset(buf2, 0, PAGE_SIZE);
  FileManager::getInstance().readPage(fileId, 15, buf2);
  ASSERT_EQ(memcmp(buf, buf2, PAGE_SIZE), 0);
  FileManager::getInstance().closeFile(fileId);

  delete [] buf;
  delete [] buf2;
  system("rm helloworld.txt");
}

TEST(FILE_MENAGER, FILE_MANAGER_MULTIFILE) {
  FileManager::getInstance().createFile("1.txt");
  FileManager::getInstance().createFile("2.txt");
  FileManager::getInstance().createFile("3.txt");

  int fileId = FileManager::getInstance().openFile("1.txt");
  ASSERT_EQ(fileId, 0);
  fileId = FileManager::getInstance().openFile("2.txt");
  ASSERT_EQ(fileId, 1);
  FileManager::getInstance().closeFile(0);
  fileId = FileManager::getInstance().openFile("3.txt");
  ASSERT_EQ(fileId, 0);
  FileManager::getInstance().closeFile(1);
  fileId = FileManager::getInstance().openFile("1.txt");
  ASSERT_EQ(fileId, 1);

  FileManager::getInstance().close();
  system("rm 1.txt 2.txt 3.txt");
}
