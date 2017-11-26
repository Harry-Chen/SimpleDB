//
// Created by Harry Chen on 2017/11/20.
//
#include <string>
#include <cstring>
#include <cassert>

#include "Compare.h"

bool compareFloat(float x, OpType op, float y) {
    switch (op) {
        case OP_EQ:
            return x == y;
        case OP_GE:
            return x >= y;
        case OP_LE:
            return x <= y;
        case OP_GT:
            return x > y;
        case OP_LT:
            return x < y;
        default:
            assert(0);
    }
}

bool compareInt(int x, OpType op, int y) {
    switch (op) {
        case OP_EQ:
            return x == y;
        case OP_GE:
            return x >= y;
        case OP_LE:
            return x <= y;
        case OP_GT:
            return x > y;
        case OP_LT:
            return x < y;
        default:
            assert(0);
    }
}

bool compareVarchar(const char *x, OpType op, const char *y) {
    switch (op) {
        case OP_EQ:
            return strcmp(x, y) == 0;
        case OP_GE:
            return strcmp(x, y) >= 0;
        case OP_LE:
            return strcmp(x, y) <= 0;
        case OP_GT:
            return strcmp(x, y) > 0;
        case OP_LT:
            return strcmp(x, y) < 0;
        default:
            assert(0);
    }
}

int sgn(int x) {
    if (x == 0) return 0;
    if (x > 0) return 1;
    return -1;
}

int compareIntSgn(int x, int y) {
    return sgn(x - y);
}

int compareVarcharSgn(char *x, char *y) {
    return sgn(strcmp(x, y));
}

int fsgn(float x) {
    if (x == 0) return 0;
    if (x > 0) return 1;
    return -1;
}

int compareFloatSgn(float x, float y) {
    return fsgn(x - y);
}

std::string opTypeToString(OpType op) {
    switch (op) {
        case OP_EQ:
            return "==";
        case OP_GE:
            return ">=";
        case OP_LE:
            return "<=";
        case OP_GT:
            return ">";
        case OP_LT:
            return "<";
        default:
            assert(0);
    }
}