#include "file_utils.h"

using namespace std;

// 获取 FAT 第 index 个表项的值
int getFATEntry(FILE* fat12, int index){
    int base = fat_base_addr;
    int fat_entry_addr = base + index * 3 / 2;

    int type = index % 2;

    uint16_t fat_value;
    uint16_t* ptr = &fat_value;
    fseek(fat12, fat_entry_addr, SEEK_SET);
    fread(ptr, 1, 2, fat12);

    if(type == 0){
        fat_value = fat_value << 4;
    }

    fat_value >>= 4;

    return fat_value;
}



vector<string> tokenize(const string &str, const string &deliminator){
    vector<string> res;

    if(str == ""){
        return res;
    }

    char* s = new char[str.size() + 1];
    unique_ptr<char> ptr = make_unique<char>();
    ptr.reset(s);
    strcpy(ptr.get(), str.c_str());

    char* delim = new char[deliminator.size() + 1];
    unique_ptr<char> dptr = make_unique<char>();
    dptr.reset(delim);
    strcpy(dptr.get(), deliminator.c_str());

    char* p = strtok(ptr.get(), dptr.get());

    while(p){
        string token = p;
        res.push_back(token);
        p = strtok(NULL, dptr.get());
    }

    return res;
}