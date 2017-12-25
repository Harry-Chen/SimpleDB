#ifndef __COMPARE_H__
#define __COMPARE_H__

enum ColumnType {
    CT_INT, CT_VARCHAR, CT_FLOAT, CT_DATE
};
enum OpType {
    OP_EQ, OP_GE, OP_LE, OP_GT, OP_LT
};
enum RelType {
    RE_OR, RE_AND
};

bool compareFloat(float x, OpType op, float y);

bool compareInt(int x, OpType op, int y);

bool compareVarchar(const char *x, OpType op, const char *y);

int sgn(int x);

int compareIntSgn(int x, int y);

int compareVarcharSgn(char *x, char *y);

int floatSgn(float x);

int compareFloatSgn(float x, float y);

std::string opTypeToString(OpType op);

#endif // __COMPARE_H__
