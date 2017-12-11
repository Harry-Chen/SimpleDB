//
// Created by Harry Chen on 2017/11/20.
//
#include <cstring>
#include <string>
#include <sstream>

#include "../io/FileManager.h"
#include "../io/BufPageManager.h"
#include "RegisterManager.h"
#include "Table.h"

bool operator<(const IndexKey &a, const IndexKey &b) {
    assert(a.permID == b.permID);
    assert(a.col == b.col);
    Table *tab = RegisterManager::getInstance().getPtr(a.permID);
    ColumnType tp = tab->getColumnType(a.col);

    //compare null
    if (a.isNull) {
        if (b.isNull) return a.rid < b.rid;
        return false;
    } else {
        if (b.isNull) return false;
    }

    //fast compare
    int tmp;
    switch (tp) {
        case CT_INT:
            tmp = sgn(a.fastCmp - b.fastCmp);
            if (tmp == -1) return true;
            if (tmp == 1) return false;
            return a.rid < b.rid;
            break;
        case CT_VARCHAR:
        case CT_FLOAT:
            tmp = sgn(a.fastCmp - b.fastCmp);
            if (tmp == -1) return true;
            if (tmp == 1) return false;
            break;
        default:
            assert(0);
    }

    char *x = tab->select(a.rid, a.col);
    char *y = tab->select(b.rid, b.col);
    int res;
    switch (tp) {
        case CT_VARCHAR:
            res = compareVarcharSgn(x, y);
            break;
        case CT_FLOAT:
            res = compareFloatSgn(*(float *) x, *(float *) y);
        default:
            assert(0);
    }
    delete[] x;
    delete[] y;
    if (res < 0) return true;
    if (res > 0) return false;
    return a.rid < b.rid;
}

void Table::initTempRecord() {
    unsigned int &notNull = *(unsigned int *) buf;
    notNull = 0;
    for (int i = 0; i < head.columnTot; i++) {
        if (head.defaultOffset[i] != -1) {
            switch (head.columnType[i]) {
                case CT_INT:
                case CT_FLOAT:
                    memcpy(buf + head.columnOffset[i], head.dataArr + head.defaultOffset[i], 4);
                    break;
                case CT_VARCHAR:
                    strcpy(buf + head.columnOffset[i], head.dataArr + head.defaultOffset[i]);
                    break;
                default:
                    assert(0);
            }
            notNull |= (1u << i);
        }
    }
}

void Table::allocPage() {
    int index = BufPageManager::getInstance().allocPage(fileID, head.pageTot);
    char *buf = BufPageManager::getInstance().access(index);
    int n = (PAGE_SIZE - PAGE_FOOTER_SIZE) / head.recordByte;
    n = (n < MAX_REC_PER_PAGE) ? n : MAX_REC_PER_PAGE;
    for (int i = 0, p = 0; i < n; i++, p += head.recordByte) {
        unsigned int &ptr = *(unsigned int *) (buf + p);
        ptr = head.nextAvail;
        head.nextAvail = head.pageTot * PAGE_SIZE + p;
    }
    memset(buf + PAGE_SIZE - PAGE_FOOTER_SIZE, 0, PAGE_FOOTER_SIZE);
    BufPageManager::getInstance().markDirty(index);
    head.pageTot++;
}

void Table::inverseFooter(char *page, int idx) {
    int u = idx / 32;
    int v = idx % 32;
    unsigned int &tmp = *(unsigned int *) (page + PAGE_SIZE - PAGE_FOOTER_SIZE + u * 4);
    tmp ^= (1u << v);
}

int Table::getFooter(char *page, int idx) {
    int u = idx / 32;
    int v = idx % 32;
    unsigned int tmp = *(unsigned int *) (page + PAGE_SIZE - PAGE_FOOTER_SIZE + u * 4);
    return (tmp >> v) & 1;
}

Table::Table() {
    ready = 0;
}

Table::~Table() {
    if (ready) close();
}

std::string Table::getTableName() {
    assert(ready);
    return tableName;
}

void Table::printSchema() {
    for (int i = 1; i < head.columnTot; i++) {
        printf("%s", head.columnName[i]);
        switch (head.columnType[i]) {
            case CT_INT:
                printf(" INT");
                break;
            case CT_FLOAT:
                printf(" FLOAT");
                break;
            case CT_VARCHAR:
                printf(" VARCHAR(%d)", head.columnLen[i]);
                break;
            default:
                assert(0);
        }
        if (head.notNull & (1 << i)) printf(" NotNull");
        if (head.hasIndex & (1 << i)) printf(" Indexed");
        if (i == head.primary) printf(" Primary");
        printf("\n");
    }
}

bool Table::hasIndex(int col) {
    return head.hasIndex & (1 << col);
}

unsigned int Table::getNext(unsigned int rid) {
    int pageid, id, n;
    n = (PAGE_SIZE - PAGE_FOOTER_SIZE) / head.recordByte;
    n = (n < MAX_REC_PER_PAGE) ? n : MAX_REC_PER_PAGE;
    if (rid == -1) {
        pageid = 0;
        id = n - 1;
    } else {
        pageid = rid / PAGE_SIZE;
        id = (rid % PAGE_SIZE) / head.recordByte;
    }
    int index = BufPageManager::getInstance().getPage(fileID, pageid);
    char *page = BufPageManager::getInstance().access(index);

    while (true) {
        id++;
        if (id == n) {
            pageid++;
            if (pageid >= head.pageTot) return -1;
            index = BufPageManager::getInstance().getPage(fileID, pageid);
            page = BufPageManager::getInstance().access(index);
            id = 0;
        }
        if (getFooter(page, id)) return pageid * PAGE_SIZE + id * head.recordByte;
    }
}

// return -1 if name exist, columnId otherwise
// size: maxlen for varchar, outputwidth for int
int Table::addColumn(const char *name, ColumnType type, int size,
                     bool notNull, bool hasDefault, const char *data) {
    printf("adding %s %d %d\n", name, type, size);
    assert(head.pageTot == 1);
    assert(strlen(name) < MAX_NAME_LEN);
    for (int i = 0; i < head.columnTot; i++)
        if (strcmp(head.columnName[i], name) == 0)
            return -1;
    assert(head.columnTot < MAX_COLUMN_SIZE);
    int id = head.columnTot++;
    strcpy(head.columnName[id], name);
    head.columnType[id] = type;
    head.columnOffset[id] = head.recordByte;
    head.columnLen[id] = size;
    if (notNull) head.notNull |= (1 << id);
    head.defaultOffset[id] = -1;
    switch (type) {
        case CT_INT:
        case CT_FLOAT:
        case CT_DATE:
            head.recordByte += 4;
            if (hasDefault) {
                head.defaultOffset[id] = head.dataArrUsed;
                memcpy(head.dataArr + head.dataArrUsed, data, 4);
                head.dataArrUsed += 4;
            }
            break;
        case CT_VARCHAR:
            head.recordByte += size + 1;
            head.recordByte += 4 - head.recordByte % 4;
            if (hasDefault) {
                head.defaultOffset[id] = head.dataArrUsed;
                strcpy(head.dataArr + head.dataArrUsed, data);
                head.dataArrUsed += strlen(data) + 1;
            }
            break;
        default:
            assert(0);
    }
    assert(head.dataArrUsed <= MAX_DATA_SIZE);
    assert(head.recordByte <= PAGE_SIZE);
    return id;
}

void Table::createIndex(int col) {
    assert(head.pageTot == 1);
    assert((head.hasIndex & (1 << col)) == 0);
    head.hasIndex |= 1 << col;
}

void Table::dropIndex(int col) {
    assert((head.hasIndex & (1 << col)));
    head.hasIndex ^= 1 << col;
    colIndex[col].drop(permID, col);
}

void Table::setPrimary(int columnID) {
    assert((head.notNull >> columnID) & 1);
    head.primary = columnID;
}

void Table::loadIndex() {
    for (int i = 0; i < head.columnTot; i++)
        if (head.hasIndex & (1 << i)) {
            colIndex[i].load(permID, i);
        }
}

void Table::storeIndex() {
    for (int i = 0; i < head.columnTot; i++)
        if (head.hasIndex & (1 << i)) {
            colIndex[i].store(permID, i);
        }
}

void Table::dropIndex() {
    for (int i = 0; i < head.columnTot; i++)
        if (head.hasIndex & (1 << i)) {
            colIndex[i].drop(permID, i);
        }
}

int Table::getFastCmp(int rid, int col) {
    char *p = select(rid, col);
    if (p == 0) return 0;
    int res = 0;
    float tmp;
    switch (head.columnType[col]) {
        case CT_INT:
        case CT_DATE:
            res = *(int *) p;
            break;
        case CT_FLOAT:
            tmp = *(float *) p;
            if (tmp > 2e9)
                res = 2e9;
            else if (tmp < -2e9)
                res = -2e9;
            else
                res = tmp;
            break;
        case CT_VARCHAR:
            res = 0;
            for (int i = 0; i < 4; i++) {
                res = res * 256;
                if (i < strlen(p)) res += i;
            }
            break;
        default:
            assert(0);
    }
    if (p) delete[] p;
    return res;
}

bool Table::getIsNull(int rid, int col) {
    char *p = select(rid, col);
    delete[] p;
    return p == 0;
}

void Table::eraseColIndex(int rid, int col) {
    if (head.hasIndex & (1 << col)) {
        colIndex[col].erase(IndexKey(permID, rid, col, getFastCmp(rid, col), getIsNull(rid, col)));
    }
}

void Table::insertColIndex(int rid, int col) {
    if (head.hasIndex & (1 << col)) {
        colIndex[col].insert(IndexKey(permID, rid, col, getFastCmp(rid, col), getIsNull(rid, col)));
    }
}

void Table::create(const char *tableName) {
    assert(ready == 0);
    this->tableName = std::string(tableName);
    BufPageManager::getFileManager().createFile(tableName);
    fileID = BufPageManager::getFileManager().openFile(tableName);
    permID = BufPageManager::getFileManager().getFilePermID(fileID);
    BufPageManager::getInstance().allocPage(fileID, 0);
    RegisterManager::getInstance().checkin(permID, this);
    ready = 1;
    head.pageTot = 1;
    head.recordByte = 4; // reserve first 4 bytes for notnull info
    head.rowTot = 0;
    head.columnTot = 0;
    head.dataArrUsed = 0;
    head.nextAvail = -1;
    head.notNull = 0;
    head.checkTot = 0;
    addColumn("RID", CT_INT, 10, true, false, 0);
    setPrimary(0);
    buf = 0;
    for (int i = 0; i < MAX_COLUMN_SIZE; i++) colIndex[i].clear();
}

void Table::open(const char *tableName) {
    assert(ready == 0);
    this->tableName = std::string(tableName);
    fileID = BufPageManager::getFileManager().openFile(tableName);
    permID = BufPageManager::getFileManager().getFilePermID(fileID);
    RegisterManager::getInstance().checkin(permID, this);
    int index = BufPageManager::getInstance().getPage(fileID, 0);
    memcpy(&head, BufPageManager::getInstance().access(index), sizeof(TableHead));
    ready = 1;
    buf = 0;
    for (int i = 0; i < MAX_COLUMN_SIZE; i++) colIndex[i].clear();
    loadIndex();
}

void Table::close() {
    assert(ready == 1);
    storeIndex();
    int index = BufPageManager::getInstance().getPage(fileID, 0);
    memcpy(BufPageManager::getInstance().access(index), &head, sizeof(TableHead));
    BufPageManager::getInstance().markDirty(index);
    RegisterManager::getInstance().checkout(permID);
    BufPageManager::getInstance().closeFile(fileID);
    BufPageManager::getFileManager().closeFile(fileID);
    ready = 0;
    if (buf) {
        delete[] buf;
        buf = 0;
    }
}

void Table::drop() {
    assert(ready == 1);
    dropIndex();
    RegisterManager::getInstance().checkout(permID);
    BufPageManager::getInstance().closeFile(fileID, false);
    BufPageManager::getFileManager().closeFile(fileID);
    ready = 0;
}

std::string Table::genCheckError(int checkId) {
    unsigned int &notNull = *(unsigned int *) buf;
    int ed = checkId + 1, st = checkId;
    while (head.checkList[st - 1].col == head.checkList[checkId].col &&
           head.checkList[st - 1].rel == RE_OR && head.checkList[checkId].rel == RE_OR) {
        checkId++;
    }
    std::ostringstream stm;
    stm << "Insert Error: Col " << head.columnName[head.checkList[checkId].col];
    stm << " CHECK ";

    for (int i = st; i < ed; i++) {
        if (i != st) stm << " OR ";
        Check chk = head.checkList[i];
        switch (head.columnType[chk.col]) {
            case CT_INT:
            case CT_DATE:
                if (notNull & (1 << chk.col)) {
                    stm << *(int *) (buf + head.columnOffset[chk.col]);
                } else {
                    stm << "null";
                } // TODO parse date to string here
                stm << opTypeToString(chk.op) << *(int *) (head.dataArr + chk.offset);
                break;
            case CT_FLOAT:
                if (notNull & (1 << chk.col)) {
                    stm << *(float *) (buf + head.columnOffset[chk.col]);
                } else {
                    stm << "null";
                }
                stm << opTypeToString(chk.op) << *(float *) (head.dataArr + chk.offset);
                break;
            case CT_VARCHAR:
                if (notNull & (1 << chk.col)) {
                    stm << *(int *) (buf + head.columnOffset[chk.col]);
                } else {
                    stm << "null";
                }
                stm << "'" << buf + head.columnOffset[chk.col] << "''" << opTypeToString(chk.op) << "'"
                    << head.dataArr + chk.offset << "'";
                break;
            default:
                assert(0);
        }
    }
    return stm.str();
}

bool Table::checkPrimary() {
    if (head.primary == 0) return true;
    int rid = colIndex[head.primary].lowerboundEqual(
            IndexKey(permID, -1, head.primary, getFastCmp(-1, head.primary), getIsNull(-1, head.primary)));
    if (rid == -1) return true;
    if (rid == *(int *) (buf + head.columnOffset[0])) return true;
    char *conf;
    switch (head.columnType[head.primary]) {
        case CT_INT:
        case CT_DATE:
            return false;
            break;
        case CT_FLOAT:
            conf = select(rid, head.primary);
            if (*(float *) conf == *(float *) (buf + head.columnOffset[head.primary])) return false;
            break;
        case CT_VARCHAR:
            conf = select(rid, head.primary);
            if (strcmp(conf, buf + head.columnOffset[head.primary]) == 0) return false;
            break;
        default:
            assert(0);
    }
    return true;
}

std::string Table::checkRecord() {
    unsigned int &notNull = *(unsigned int *) buf;
    if ((notNull & head.notNull) != head.notNull) {
        return "Insert Error: not null column is null.";
    }

    bool flag = true, tokresult = false;

    if (!checkPrimary()) {
        return "ERROR: Primary Key Conflict";
    }

    for (int i = 0; i < head.checkTot; i++) {
        Check chk = head.checkList[i];
        if (chk.offset == -1) {
            tokresult |= (chk.op == OP_EQ) && (((~notNull) & (1 << chk.col)) == 0);
        } else {
            switch (head.columnType[chk.col]) {
                case CT_INT:
                case CT_DATE:
                    tokresult |= compareInt(*(int *) (buf + head.columnOffset[chk.col]), chk.op,
                                            *(int *) (head.dataArr + chk.offset));
                    break;
                case CT_FLOAT:
                    tokresult |= compareFloat(*(float *) (buf + head.columnOffset[chk.col]), chk.op,
                                              *(float *) (head.dataArr + chk.offset));
                    break;
                case CT_VARCHAR:
                    tokresult |= compareVarchar(buf + head.columnOffset[chk.col], chk.op,
                                                head.dataArr + chk.offset);
                    break;
                default:
                    assert(0);
            }
        }
        if (i == head.checkTot - 1 || chk.rel == RE_AND ||
            !(head.checkList[i + 1].rel == RE_OR && chk.col == head.checkList[i + 1].col)) {
            flag &= tokresult;
            tokresult = false;
        }
        if (!flag) return genCheckError(i);
    }

    return "";
}

int Table::getColumnCount() {
    return head.columnTot;
}

// return -1 if not found
int Table::getColumnID(const char *name) {
    for (int i = 1; i < head.columnTot; i++)
        if (strcmp(head.columnName[i], name) == 0)
            return i;
    return -1;
}

void Table::clearTempRecord() {
    if (buf == 0) {
        buf = new char[head.recordByte];
        initTempRecord();
    }
}

// RelType is used to create Inset operation.
// Dirty interface.
// With `relation` RE_OR and same `col`, the result will OR together
// others will AND together
// data == 0 if data is null
// Example:
//   List: [1, >, 10, AND], [2, ==, 'a', OR], [2, ==, 'b', OR], [3, ==, 'c', OR]
//   Result: col[1]>10 AND (col[2]=='a' OR col[2]=='b') AND col[3]=='c'
// Example:
//   List: [1, >, 10, AND], [2, ==, 'a', OR], [3, ==, 'c', OR], [2, ==, 'b', OR]
//   Result: col[1]>10 AND col[2]=='a' AND col[3]=='c' AND col[2]=='b'

void Table::addCheck(int col, OpType op, char *data, RelType relation) {
    assert(head.pageTot == 1);
    assert(head.checkTot < MAX_CHECK);
    int id = head.checkTot;
    head.checkList[id].col = col;
    head.checkList[id].offset = head.dataArrUsed;
    head.checkList[id].rel = relation;
    if (data == 0) {
        head.checkList[id].offset = -1;
        head.checkTot++;
        return;
    }
    switch (head.columnType[col]) {
        case CT_INT:
        case CT_FLOAT:
        case CT_DATE:
            memcpy(head.dataArr + head.dataArrUsed, data, 4);
            head.dataArrUsed += 4;
            break;
        case CT_VARCHAR:
            strcpy(head.dataArr + head.dataArrUsed, data);
            head.dataArrUsed += strlen(data) + 1;
            head.dataArrUsed += (4 - head.dataArrUsed % 4) % 4;
            break;
        default:
            assert(0);
    }
    assert(head.dataArrUsed <= MAX_DATA_SIZE);
    head.checkTot++;
}

std::string Table::setTempRecord(int col, const char *data) {
    if (data == 0) {
        setTempRecordNull(col);
        return "";
    }
    if (buf == 0) {
        buf = new char[head.recordByte];
        initTempRecord();
    }
    unsigned int &notNull = *(unsigned int *) buf;
    switch (head.columnType[col]) {
        case CT_INT:
        case CT_DATE:
        case CT_FLOAT:
            memcpy(buf + head.columnOffset[col], data, 4);
            break;
        case CT_VARCHAR:
            if (head.columnLen[col] < strlen(data)) {
                printf("%d %s\n", head.columnLen[col], data);
            }
            if (strlen(data) > head.columnLen[col]) {
                return "ERROR: varchar too long";
            }
            strcpy(buf + head.columnOffset[col], data);
            break;
        default:
            assert(0);
    }
    notNull |= (1u << col);
    return "";
}

void Table::setTempRecordNull(int col) {
    if (buf == 0) {
        buf = new char[head.recordByte];
        initTempRecord();
    }
    unsigned int &notNull = *(unsigned int *) buf;
    if (notNull & (1u << col)) notNull ^= (1u << col);
}

// return value change. Urgly interface.
// return "" if success.
// return error description otherwise.
std::string Table::insertTempRecord() {
    assert(buf != 0);
    if (head.nextAvail == -1) {
        allocPage();
    }
    int rid = head.nextAvail;
    setTempRecord(0, (char *) &head.nextAvail);
    std::string error = checkRecord();
    if (error != "") {
        return error;
    }
    int pageID = head.nextAvail / PAGE_SIZE;
    int offset = head.nextAvail % PAGE_SIZE;
    int index = BufPageManager::getInstance().getPage(fileID, pageID);
    char *page = BufPageManager::getInstance().access(index);
    head.nextAvail = *(unsigned int *) (page + offset);
    memcpy(page + offset, buf, head.recordByte);
    BufPageManager::getInstance().markDirty(index);
    inverseFooter(page, offset / head.recordByte);
    for (int i = 0; i < head.columnTot; i++) insertColIndex(rid, i);
    return "";
}

void Table::dropRecord(int rid) {
    int pageID = rid / PAGE_SIZE;
    int offset = rid % PAGE_SIZE;
    for (int i = 0; i < head.columnTot; i++) {
        if (head.hasIndex & (1 << i)) eraseColIndex(rid, i);
    }
    int index = BufPageManager::getInstance().getPage(fileID, pageID);
    char *page = BufPageManager::getInstance().access(index);
    char *record = page + offset;
    unsigned int &next = *(unsigned int *) record;
    next = head.nextAvail;
    head.nextAvail = rid;
    inverseFooter(page, offset / head.recordByte);
    BufPageManager::getInstance().markDirty(index);
}

std::string Table::loadRecordToTemp(int rid, char *page, int offset) {
    if (buf == 0) {
        buf = new char[head.recordByte];
    }
    char *record = page + offset;
    if (!getFooter(page, offset / head.recordByte)) {
        return "ERROR: RID invalid";
    }
    memcpy(buf, record, head.recordByte);
    return "";
}

std::string Table::modifyRecord(int rid, int col, char *data) {
    if (data == 0) {
        return modifyRecordNull(rid, col);
    }
    int pageID = rid / PAGE_SIZE;
    int offset = rid % PAGE_SIZE;
    int index = BufPageManager::getInstance().getPage(fileID, pageID);
    char *page = BufPageManager::getInstance().access(index);
    char *record = page + offset;
    std::string err = loadRecordToTemp(rid, page, offset);
    if (err != "") {
        return err;
    }
    assert(col != 0);
    err = setTempRecord(col, data);
    if (err != "") {
        return err;
    }
    err = checkRecord();
    if (err != "") {
        return err;
    }
    eraseColIndex(rid, col);
    memcpy(record, buf, head.recordByte);
    BufPageManager::getInstance().markDirty(index);
    insertColIndex(rid, col);
    return "";
}

std::string Table::modifyRecordNull(int rid, int col) {
    int pageID = rid / PAGE_SIZE;
    int offset = rid % PAGE_SIZE;
    int index = BufPageManager::getInstance().getPage(fileID, pageID);
    char *page = BufPageManager::getInstance().access(index);
    char *record = page + offset;
    std::string err = loadRecordToTemp(rid, page, offset);
    if (err != "") {
        return err;
    }
    assert(col != 0);
    setTempRecordNull(col);
    err = checkRecord();
    if (err != "") {
        return err;
    }
    eraseColIndex(rid, col);
    memcpy(record, buf, head.recordByte);
    BufPageManager::getInstance().markDirty(index);
    insertColIndex(rid, col);
    return "";
}

/* remove interface.
void Table::replaceRecordWithTemp(int rid) {
  assert(buf != 0);
  unsigned int &notNull = *(unsigned int*)buf;
  assert((notNull & head.notNull) == head.notNull);
  int pageID = rid / PAGE_SIZE;
  int offset = rid % PAGE_SIZE;
  int index = BufPageManager::getInstance().getPage(fileID, pageID);
  char *page = BufPageManager::getInstance().access(index);
  head.nextAvail = *(unsigned int*)(page + offset);
  memcpy(page + offset, buf, head.recordByte);
  BufPageManager::getInstance().markDirty(index);
}
*/
int Table::getRecordBytes() {
    return head.recordByte;
}

char *Table::getRecordTempPtr(unsigned int rid) {
    int pageID = rid / PAGE_SIZE;
    int offset = rid % PAGE_SIZE;
    assert(1 <= pageID && pageID < head.pageTot);
    int index = BufPageManager::getInstance().getPage(fileID, pageID);
    char *page = BufPageManager::getInstance().access(index);
    assert(getFooter(page, offset / head.recordByte));
    return page + offset;
}

void Table::getRecord(unsigned int rid, char *buf) {
    char *ptr = getRecordTempPtr(rid);
    memcpy(buf, ptr, head.recordByte);
}

int Table::getColumnOffset(int col) {
    return head.columnOffset[col];
}

ColumnType Table::getColumnType(int col) {
    return head.columnType[col];
}
/*
  unsigned int Table::select(unsigned int rid, int col, OpType op, const char *data) {
    printf("table::select: depreciate op!\n");
    switch (head.columnType[col]) {
      case CT_INT:
        while (true) {
          rid = getNext(rid);
          if (rid == -1) return -1;
          char *backend = getRecordTempPtr(rid);
          if (compareInt(*(int*)(backend + head.columnOffset[col]), op, *(int*)data)) {
            return *(unsigned int*)(backend + head.columnOffset[0]);
          }
        }
        break;
      case CT_VARCHAR:
        while (true) {
          rid = getNext(rid);
          if (rid == -1) return -1;
          char *backend = getRecordTempPtr(rid);
          if (compareVarchar(backend + head.columnOffset[col], op, data)) {
            return *(unsigned int*)(backend + head.columnOffset[0]);
          }
        }
        break;
      default:
        assert(0);
    }
  }
*/
//return 0 when null
//return value in tempbuf when rid = -1
char *Table::select(unsigned int rid, int col) {
    char *ptr;
    if (rid != -1) {
        ptr = getRecordTempPtr(rid);
    } else {
        ptr = buf;
    }
    unsigned int &notNull = *(unsigned int *) ptr;
    char *buf;
    if ((~notNull) & (1 << col)) {
        return 0;
    }
    switch (head.columnType[col]) {
        case CT_INT:
        case CT_DATE:
        case CT_FLOAT:
            buf = new char[4];
            memcpy(buf, ptr + getColumnOffset(col), 4);
            return buf;
        case CT_VARCHAR:
            buf = new char[head.columnLen[col] + 1];
            strcpy(buf, ptr + getColumnOffset(col));
            return buf;
        default:
            assert(0);
    }
}

int Table::selectIndexLowerBound(int col, const char *data) {
    if (data == 0) {
        return selectIndexLowerBoundNull(col);
    }
    assert(hasIndex(col));
    setTempRecord(col, data);
    return colIndex[col].lowerbound(IndexKey(permID, -1, col, getFastCmp(-1, col), getIsNull(-1, col)));
}

int Table::selectIndexLowerBoundNull(int col) {
    assert(hasIndex(col));
    return colIndex[col].begin();
}

int Table::selectIndexNext(int col) {
    assert(hasIndex(col));
    return colIndex[col].next();
}

int Table::selectIndexUpperBound(int col, const char *data) {
    if (data == 0) {
        return selectIndexUpperBoundNull(col);
    }
    assert(hasIndex(col));
    setTempRecord(col, data);
    return colIndex[col].upperbound(IndexKey(permID, -1, col, getFastCmp(-1, col), getIsNull(-1, col)));
}

int Table::selectIndexUpperBoundNull(int col) {
    assert(hasIndex(col));
    return colIndex[col].end();
}

int Table::selectReveredIndexNext(int col) {
    assert(hasIndex(col));
    return colIndex[col].reversedNext();
}

char *Table::getColumnName(int col) {
    assert(0 <= col && col < head.columnTot);
    return head.columnName[col];
}