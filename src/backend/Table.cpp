//
// Created by Harry Chen on 2017/11/20.
//
#include <cstring>
#include <string>
#include <sstream>
#include <dbms/DBMS.h>

#include "../io/FileManager.h"
#include "../io/BufPageManager.h"
#include "RegisterManager.h"

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
        case CT_DATE:
            tmp = sgn(a.fastCmp - b.fastCmp);
            if (tmp == -1) return true;
            if (tmp == 1) return false;
            return a.rid < b.rid;
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
    int res = 0;
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
                case CT_DATE:
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

void Table::inverseFooter(const char *page, int idx) {
    int u = idx / 32;
    int v = idx % 32;
    unsigned int &tmp = *(unsigned int *) (page + PAGE_SIZE - PAGE_FOOTER_SIZE + u * 4);
    tmp ^= (1u << v);
}

int Table::getFooter(const char *page, int idx) {
    int u = idx / 32;
    int v = idx % 32;
    unsigned int tmp = *(unsigned int *) (page + PAGE_SIZE - PAGE_FOOTER_SIZE + u * 4);
    return (tmp >> v) & 1;
}

Table::Table() {
    ready = false;
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
                printf(" INT(%d)", head.columnLen[i]);
                break;
            case CT_FLOAT:
                printf(" FLOAT");
                break;
            case CT_DATE:
                printf(" DATE");
                break;
            case CT_VARCHAR:
                printf(" VARCHAR(%d)", head.columnLen[i]);
                break;
            default:
                assert(0);
        }
        if (head.notNull & (1 << i)) printf(" NotNull");
        if (head.hasIndex & (1 << i)) printf(" Indexed");
        if (head.isPrimary & (1 << i)) printf(" Primary");
        printf("\n");
    }
}

bool Table::hasIndex(int col) {
    return (head.hasIndex & (1 << col)) != 0;
}

bool Table::isPrimary(int col) {
    return (head.isPrimary & (1 << col)) != 0;
}

unsigned int Table::getNext(unsigned int rid) {
    int page_id, id, n;
    n = (PAGE_SIZE - PAGE_FOOTER_SIZE) / head.recordByte;
    n = (n < MAX_REC_PER_PAGE) ? n : MAX_REC_PER_PAGE;
    if (rid == -1) {
        page_id = 0;
        id = n - 1;
    } else {
        page_id = rid / PAGE_SIZE;
        id = (rid % PAGE_SIZE) / head.recordByte;
    }
    int index = BufPageManager::getInstance().getPage(fileID, page_id);
    char *page = BufPageManager::getInstance().access(index);

    while (true) {
        id++;
        if (id == n) {
            page_id++;
            if (page_id >= head.pageTot) return -1;
            index = BufPageManager::getInstance().getPage(fileID, page_id);
            page = BufPageManager::getInstance().access(index);
            id = 0;
        }
        if (getFooter(page, id)) return page_id * PAGE_SIZE + id * head.recordByte;
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
    //assert(head.pageTot == 1);
    assert((head.hasIndex & (1 << col)) == 0);
    head.hasIndex |= 1 << col;
}

void Table::dropIndex(int col) {
    assert((head.hasIndex & (1 << col)));
    head.hasIndex &= ~(1 << col);
    colIndex[col].drop(permID, col);
}

void Table::setPrimary(int columnID) {
    assert((head.notNull >> columnID) & 1);
    head.isPrimary |= (1 << columnID);
    ++head.primaryCount;
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
    if (p == nullptr) return 0;
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
                res = (int) 2e9;
            else if (tmp < -2e9)
                res = (int) -2e9;
            else
                res = (int) tmp;
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
    delete[] p;
    return res;
}

bool Table::getIsNull(int rid, int col) {
    char *p = select(rid, col);
    delete[] p;
    return p == 0;
}

void Table::eraseColIndex(int rid, int col) {
    if (hasIndex(col)) {
        colIndex[col].erase(IndexKey(permID, rid, col, getFastCmp(rid, col), getIsNull(rid, col)));
    }
}

void Table::insertColIndex(int rid, int col) {
    if (hasIndex(col)) {
        colIndex[col].insert(IndexKey(permID, rid, col, getFastCmp(rid, col), getIsNull(rid, col)));
    }
}

void Table::create(const char *tableName) {
    assert(!ready);
    this->tableName = std::string(tableName);
    BufPageManager::getFileManager().createFile(tableName);
    fileID = BufPageManager::getFileManager().openFile(tableName);
    permID = BufPageManager::getFileManager().getFilePermID(fileID);
    BufPageManager::getInstance().allocPage(fileID, 0);
    RegisterManager::getInstance().checkIn(permID, this);
    ready = true;
    head.pageTot = 1;
    head.recordByte = 4; // reserve first 4 bytes for notnull info
    //head.rowTot = 0;
    head.columnTot = 0;
    head.dataArrUsed = 0;
    head.nextAvail = -1;
    head.notNull = 0;
    head.checkTot = 0;
    head.foreignKeyTot = 0;
    head.primaryCount = 0;
    addColumn("RID", CT_INT, 10, true, false, nullptr);
    setPrimary(0);
    buf = nullptr;
    for (auto &col: colIndex) {
        col.clear();
    }
}

void Table::open(const char *tableName) {
    assert(ready == 0);
    this->tableName = std::string(tableName);
    fileID = BufPageManager::getFileManager().openFile(tableName);
    permID = BufPageManager::getFileManager().getFilePermID(fileID);
    RegisterManager::getInstance().checkIn(permID, this);
    int index = BufPageManager::getInstance().getPage(fileID, 0);
    memcpy(&head, BufPageManager::getInstance().access(index), sizeof(TableHead));
    ready = true;
    buf = nullptr;
    for (auto &col: colIndex) {
        col.clear();
    }
    loadIndex();
}

void Table::close() {
    assert(ready);
    storeIndex();
    int index = BufPageManager::getInstance().getPage(fileID, 0);
    memcpy(BufPageManager::getInstance().access(index), &head, sizeof(TableHead));
    BufPageManager::getInstance().markDirty(index);
    RegisterManager::getInstance().checkOut(permID);
    BufPageManager::getInstance().closeFile(fileID);
    BufPageManager::getFileManager().closeFile(fileID);
    ready = false;
    if (buf) {
        delete[] buf;
        buf = 0;
    }
}

void Table::drop() {
    assert(ready == 1);
    dropIndex();
    RegisterManager::getInstance().checkOut(permID);
    BufPageManager::getInstance().closeFile(fileID, false);
    BufPageManager::getFileManager().closeFile(fileID);
    ready = false;
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
    if (head.primaryCount == 1) return true;
    int conflictCount = 0;
    int firstPrimary = 1;
    while (!isPrimary(firstPrimary)) {
        ++firstPrimary;
    }
    auto equalFirstIndex = IndexKey(permID, -1, firstPrimary, getFastCmp(-1, firstPrimary),
                                    getIsNull(-1, firstPrimary));
    auto rid = colIndex[firstPrimary].lowerBoundEqual(equalFirstIndex);
    while (rid != -1) {
        if (rid == *(int *) (buf + head.columnOffset[0])) {
            // hit the record it self (when updating)
            return true;
        }
        conflictCount = 1;
        for (int col = firstPrimary + 1; col < head.columnTot; ++col) {
            if (!isPrimary(col)) {
                continue;
            }
            char *tmp;
            //char *new_record = getRecordTempPtr();
            switch (head.columnType[col]) {
                case CT_INT:
                case CT_DATE:
                    tmp = select(rid, col);
                    if (*(int *) tmp == *(int *) (buf + head.columnOffset[col])) {
                        ++conflictCount;
                    }
                    free(tmp);
                    break;
                case CT_FLOAT:
                    tmp = select(rid, col);
                    if (*(float *) tmp == *(float *) (buf + head.columnOffset[col])) {
                        ++conflictCount;
                    }
                    free(tmp);
                    break;
                case CT_VARCHAR:
                    tmp = select(rid, col);
                    if (strcmp(tmp, buf + head.columnOffset[col]) == 0) {
                        ++conflictCount;
                    }
                    free(tmp);
                    break;
                default:
                    assert(0);
            }
        }
        if (conflictCount == head.primaryCount - 1) {
            return false;
        }
        rid = colIndex[firstPrimary].nextEqual(equalFirstIndex);
    }
    return true;
}

std::string Table::checkValueConstraint() {
    unsigned int &notNull = *(unsigned int *) buf;
    bool flag = true, checkResult = false;
    for (int i = 0; i < head.checkTot; i++) {
        auto chk = head.checkList[i];
        if (chk.offset == -1) {
            checkResult |= (chk.op == OP_EQ) && (((~notNull) & (1 << chk.col)) == 0);
        } else {
            switch (head.columnType[chk.col]) {
                case CT_INT:
                case CT_DATE:
                    checkResult |= compareInt(*(int *) (buf + head.columnOffset[chk.col]), chk.op,
                                              *(int *) (head.dataArr + chk.offset));
                    break;
                case CT_FLOAT:
                    checkResult |= compareFloat(*(float *) (buf + head.columnOffset[chk.col]), chk.op,
                                                *(float *) (head.dataArr + chk.offset));
                    break;
                case CT_VARCHAR:
                    checkResult |= compareVarchar(buf + head.columnOffset[chk.col], chk.op,
                                                  head.dataArr + chk.offset);
                    break;
                default:
                    assert(0);
            }
        }
        if (i == head.checkTot - 1 || chk.rel == RE_AND ||
            !(head.checkList[i + 1].rel == RE_OR && chk.col == head.checkList[i + 1].col)) {
            flag &= checkResult;
            checkResult = false;
        }
        if (!flag) return genCheckError(i);
    }
    return std::string();
}

std::string Table::checkForeignKeyConstraint() {
    for (int i = 0; i < head.foreignKeyTot; ++i) {
        auto check = head.foreignKeyList[i];
        auto localData = (buf + head.columnOffset[check.col]);
        auto dbms = DBMS::getInstance();
        if (!dbms->valueExistInTable(localData, check)) {
            return "Insert Error: Value of column " + std::string(head.columnName[i])
                   + " does not meet foreign key constraint";
        }
    }
    return std::string();
}

std::string Table::checkRecord() {
    unsigned int &notNull = *(unsigned int *) buf;
    if ((notNull & head.notNull) != head.notNull) {
        return "Insert Error: not null column is null.";
    }

    if (!initMode) {
        if (!checkPrimary()) {
            return "ERROR: Primary Key Conflict";
        }
        auto valueCheck = checkValueConstraint();
        if (!valueCheck.empty()) {
            return valueCheck;
        }

        auto foreignKeyCheck = checkForeignKeyConstraint();
        if (!foreignKeyCheck.empty()) {
            return foreignKeyCheck;
        }
    }

    return std::string();
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
    if (buf == nullptr) {
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
    if (data == nullptr) {
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

void Table::addForeignKeyConstraint(int col, int foreignTableId, int foreignColId) {
    assert(head.foreignKeyTot < MAX_FOREIGN_KEY);
    head.foreignKeyList[head.foreignKeyTot].col = col;
    head.foreignKeyList[head.foreignKeyTot].foreign_table_id = foreignTableId;
    head.foreignKeyList[head.foreignKeyTot].foreign_col = foreignColId;
    head.foreignKeyTot++;
}

std::string Table::setTempRecord(int col, const char *data) {
    if (data == nullptr) {
        setTempRecordNull(col);
        return "";
    }
    if (buf == nullptr) {
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
    if (buf == nullptr) {
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
    assert(buf != nullptr);
    if (head.nextAvail == -1) {
        allocPage();
    }
    int rid = head.nextAvail;
    setTempRecord(0, (char *) &head.nextAvail);
    auto error = checkRecord();
    if (!error.empty()) {
        printf("Error occurred when inserting record, aborting...\n");
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

void Table::dropRecord(unsigned int rid) {
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
    if (buf == nullptr) {
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
    if (data == nullptr) {
        return modifyRecordNull(rid, col);
    }
    int pageID = rid / PAGE_SIZE;
    int offset = rid % PAGE_SIZE;
    int index = BufPageManager::getInstance().getPage(fileID, pageID);
    char *page = BufPageManager::getInstance().access(index);
    char *record = page + offset;
    std::string err = loadRecordToTemp(rid, page, offset);
    if (!err.empty()) {
        return err;
    }
    assert(col != 0);
    err = setTempRecord(col, data);
    if (!err.empty()) {
        return err;
    }
    err = checkRecord();
    if (!err.empty()) {
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
    if (!err.empty()) {
        return err;
    }
    assert(col != 0);
    setTempRecordNull(col);
    err = checkRecord();
    if (!err.empty()) {
        return err;
    }
    eraseColIndex(rid, col);
    memcpy(record, buf, head.recordByte);
    BufPageManager::getInstance().markDirty(index);
    insertColIndex(rid, col);
    return "";
}

int Table::getRecordBytes() {
    return head.recordByte;
}

char *Table::getRecordTempPtr(unsigned int rid) {
    int pageID = rid / PAGE_SIZE;
    int offset = rid % PAGE_SIZE;
    assert(1 <= pageID && pageID < head.pageTot);
    auto index = BufPageManager::getInstance().getPage(fileID, pageID);
    auto page = BufPageManager::getInstance().access(index);
    assert(getFooter(page, offset / head.recordByte));
    return page + offset;
}

void Table::getRecord(unsigned int rid, char *buf) {
    auto ptr = getRecordTempPtr(rid);
    memcpy(buf, ptr, head.recordByte);
}

int Table::getColumnOffset(int col) {
    return head.columnOffset[col];
}

ColumnType Table::getColumnType(int col) {
    return head.columnType[col];
}

//return 0 when null
//return value in tempbuf when rid = -1
char *Table::select(int rid, int col) {
    char *ptr;
    if (rid != -1) {
        ptr = getRecordTempPtr(rid);
    } else {
        ptr = buf;
    }
    unsigned int &notNull = *(unsigned int *) ptr;
    char *buf;
    if ((~notNull) & (1 << col)) {
        return nullptr;
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
    if (data == nullptr) {
        return selectIndexLowerBoundNull(col);
    }
    assert(hasIndex(col));
    setTempRecord(col, data);
    return colIndex[col].lowerBound(IndexKey(permID, -1, col, getFastCmp(-1, col), getIsNull(-1, col)));
}

int Table::selectIndexLowerBoundEqual(int col, const char *data) {
    if (data == nullptr) {
        return selectIndexLowerBoundNull(col);
    }
    assert(hasIndex(col));
    setTempRecord(col, data);
    return colIndex[col].lowerBoundEqual(IndexKey(permID, -1, col, getFastCmp(-1, col), getIsNull(-1, col)));
}

int Table::selectIndexLowerBoundNull(int col) {
    assert(hasIndex(col));
    return colIndex[col].begin();
}

int Table::selectIndexNext(int col) {
    assert(hasIndex(col));
    return colIndex[col].next();
}

int Table::selectIndexNextEqual(int col) {
    assert(hasIndex(col));
    return colIndex[col].nextEqual(IndexKey(permID, -1, col, getFastCmp(-1, col), getIsNull(-1, col)));
}

int Table::selectIndexUpperBound(int col, const char *data) {
    if (data == nullptr) {
        return selectIndexUpperBoundNull(col);
    }
    assert(hasIndex(col));
    setTempRecord(col, data);
    return colIndex[col].upperBound(IndexKey(permID, -1, col, getFastCmp(-1, col), getIsNull(-1, col)));
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