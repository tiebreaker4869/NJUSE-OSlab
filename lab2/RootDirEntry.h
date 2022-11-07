#include <iostream>
#include "FileNode.h"
#include "variables.h"
#include "file_utils.h"
#include "my_print.h"
#include <cstring>
#include <memory>
using namespace std;
// root directory entry, 32 bytes
class RootDirEntry {
    public:
    char filename[11]; // filename, first 8 char is filename, last 3 char is extension
    uint8_t attribute; // 文件属性, READ_ONLY = 0x01, HIDDEN = 0x02, SYSTEM = 0x04,
    // VOLUME_ID = 0x08, DIRECTORY = 0x10, ARCHIVE = 0x20, 
    uint8_t reserved_for_WIN; // reserved for windows NT
    uint8_t creation_time_in_tenths; // creation time in tenths of seconds
    uint16_t creation_time; // hour: 5 bit, minute: 6 bit, second, 5 bit, multiply second by 2
    uint16_t creation_date; // year: 7 bit, month: 4 bit, day: 5 bit
    uint16_t high_bits_of_cluster_number; // FAT12 里面总是 0
    uint16_t last_access_date; 
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint16_t cluster_number; // 簇号，用这个来找文件的第一个簇的位置
    uint32_t file_size; // 文件大小

    RootDirEntry(){}

    void initRootDirArea(FILE* fat12, FileNode* root);

    bool isEmptyName();

    bool isInvalidName();

    bool isFile();

    void makeFileName(char* name);

    void makeDirName(char* name);

    uint32_t getFileSize();

    uint16_t getFirstCluster();

    void fillFileContent(FILE* fat12, uint16_t cluster_number, FileNode* file);

    void readChildrenNode(FILE* fat12, uint16_t cluster_number, FileNode* root_node);
};