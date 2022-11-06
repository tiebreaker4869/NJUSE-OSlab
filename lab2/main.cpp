#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>

using namespace std;

// extern function from nasm
void my_print_red(char* str, int length);
void my_print_default(char* str, int length);

// 全局变量

int byte_per_sector;        // 每个扇区字节数
int sector_per_cluster;     // 每簇扇区数
int record_sector_count;    // boot 记录占用的扇区数
int nums_of_FAT;            //  FAT 表个数
int root_entry_count;       // 根目录最大文件数
int FAT_size;               // FAT 扇区数

// FAT1 的起始地址
int fat_base_addr;
int root_base_addr;
int data_base_addr;
int byte_per_cluster;

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

// Boot Record 数据部分数据结构
class BPB {
    public:
    uint16_t bytes_per_sector; // 每个扇区的字节数
    uint8_t sector_per_cluster; // 每个簇的扇区数
    uint16_t reserved_sector; // boot record 占用的扇区数
    uint8_t FAT_num; // FAT 的数量
    uint16_t MAX_FILE_NUM_IN_ROOT; // 根目录文件数的最大值
    uint16_t sector_count; // 总扇区数
    uint8_t media_descriptor; // 介质描述符
    uint16_t FAT_sector_count; // 每个 FAT 的扇区数
    uint16_t sector_per_track; //每个 track 的扇区数
    uint16_t num_of_heads; // 磁头的个数
    uint32_t hidden_sector; // 隐藏扇区的个数
    uint32_t large_sector_count; // 如果 sector_count 为 0，该值为总扇区数


    BPB(){}

    void initialize(FILE* fat12);
};


class FileNode {
    public:
    string name;
    vector<FileNode*> next;
    string path;
    uint32_t fileSize;
    bool isFile = false;;
    bool isValid = true;
    int directory_count = 0;
    int file_count = 0;
    char* content = new char[10000];

    FileNode(){}

    FileNode(string name, string path, bool is_file, uint32_t file_size);

    void setPath(string path){
        this->path = path;
    }

    void setName(string name){
        this->name = name;
    }

    string getPath(){return path;}

    void addFileChildNode(FileNode* child);

    void addDirChildNode(FileNode* child);
};

void BPB::initialize(FILE* fat12){
    fseek(fat12, 11, SEEK_SET); // 从 11 字节处开始
    fread(this, 1, 25, fat12); // BPB 长 25 个字节

    // 初始化要使用的全局变量
    byte_per_sector = this->bytes_per_sector;
    sector_per_cluster = this->sector_per_cluster;
    record_sector_count = this->reserved_sector;
    nums_of_FAT = this->FAT_num;
    root_entry_count = this->MAX_FILE_NUM_IN_ROOT;

    // FAT 占用的扇区数
    if(this->sector_count != 0){
        FAT_size = this->sector_count;
    }else {
        FAT_size = this->large_sector_count;
    }

    // FAT1 的起始地址: FAT1 在 boot record 后面
    fat_base_addr = record_sector_count * byte_per_sector;
    // root 目录区的起始地址: 根目录区在 FAT2 后面
    root_base_addr = fat_base_addr + nums_of_FAT * FAT_size * byte_per_sector;
    // 数据区的起始地址: 数据区在根目录区后面
    data_base_addr = root_base_addr + (root_entry_count * 32 + byte_per_sector - 1) / byte_per_sector * byte_per_sector;
    byte_per_cluster = byte_per_sector * sector_per_cluster;

}

void RootDirEntry::initRootDirArea(FILE* fat12, FileNode* root){
    int cur_addr = root_base_addr;
    char name[12];

    for(int i = 0; i < root_entry_count; i ++){
        // 依次读取每个根目录区表项
        fseek(fat12, cur_addr, SEEK_SET);
        fread(this, 1, 32, fat12);

        cur_addr += 32;

        if(!this->isEmptyName() && !this->isInvalidName()){
            if(this->isFile()){
                this->makeFileName(name);
                FileNode* child = new FileNode(name, root->getPath(), this->isFile(), this->file_size);
                root->addFileChildNode(child);
                this->fillFileContent(fat12, this->getFirstCluster(), child);
            }else {
                this->makeDirName(name);
                FileNode* child = new FileNode();
                child->setName(name);
                child->setPath(root->getPath() + name + "/");
                root->addDirChildNode(child);
                this->readChildrenNode(fat12, this->getFirstCluster(), child);
            }
        }
    }
}

bool RootDirEntry::isEmptyName(){
    return this->filename[0] == '\0';
}

bool isCapitalAlphaOrDigit(char c){
    if(c >= '0' && c <= '9'){
        return true;
    }

    if(c >= 'A' && c <= 'Z'){
        return true;
    }

    return false;
}

bool RootDirEntry::isInvalidName(){

    for(int i = 0; i < 11; i ++){
        char c = this->filename[i];
        if(!isCapitalAlphaOrDigit(c)){
            return true;
        }
    }

    return false;
}

bool RootDirEntry::isFile(){
    uint8_t bits = this->attribute;

    return (bits & 0x10) == 0;
}

uint32_t RootDirEntry::getFileSize(){
    return this->file_size;
}

uint16_t RootDirEntry::getFirstCluster(){
    return this->cluster_number;
}

void RootDirEntry::makeFileName(char* name){

    for(int i = 0; i < 11; i ++){
        name[i] = this->filename[i];
    }

    name[11] = '\0';
}

void RootDirEntry::makeDirName(char* name){
    for(int i = 0; i < 11; i ++){
        name[i] = this->filename[i];
    }

    name[11] = '\0';
}

int main(){

    char* fat_path = "";
    FILE* fat12 = fopen(fat_path, "rb"); // 打开 FAT12 映像文件

    BPB* bpb = new BPB();
    bpb->initialize(fat12); // 读取 boot 记录数据部分并初始化后面要使用的全局变量

    FileNode* rootNode = new FileNode();
    rootNode->setPath("/");
    rootNode->setName("");

    RootDirEntry* rootEntry = new RootDirEntry();
    rootEntry->initRootDirArea(fat12, rootNode);

    char* cmd = new char[128];

    while(true){
        // print prompt char
        //my_print_default(">", 1);
        putchar('>');

        // read command
        cin.getline(cmd, 128);

        // quit if user type 'exit'
        if(strcmp(cmd, "exit") == 0){
            break;
        }

        
    }

    return 0;
}