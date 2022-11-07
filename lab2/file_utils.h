#ifndef FILE_UTILS
#define FILE_UTILS
#include <iostream>
#include <vector>
#include "variables.h"
#include <memory>
#include <cstring>

using namespace std;

int getFATEntry(FILE* fat12, int index);

vector<string> tokenize(const string &str, const string &deliminator);
#endif