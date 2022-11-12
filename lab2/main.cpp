#include <iostream>
#include <vector>
#include <cstring>
#include <stack>

using namespace std;

const char* INVALID_PARAMS = "Invalid Params!\n";

const char* MULTIPLE_DIR_ERROR = "Multiple directory or invalid directory name!\n";

const char* INCORRECT_ARGS_AMOUNT = "Incorrect amount of args is provided!\n";

const char* BAD_CLUSTER = "Bad Cluster!\n";

const char* LS_CASE = "In ls: ";

const char* CAT_CASE = "In cat: ";

const char* EXIT = "exit.\n";

const char* UNK_CMD = "Unknown commands!\n";

int bytePerSector;        // 每个扇区字节数
int sectorPerCluster;     // 每簇扇区数
int recordSectorCount;    // boot 记录占用的扇区数
int numsOfFAT;            //  FAT 表个数
int rootEntryCount;       // 根目录最大文件数
int FATSize;               // FAT 扇区数

// FAT1 的起始地址
int fatBaseAddr;
int rootBaseAddr;
int dataBaseAddr;
int bytePerCluster;
extern "C"{
    void my_print_default(const char* str, int length);

    void my_print_red(const char* str, int length);
}

void myPrintDefault(const char* str);

void myPrintRed(const char* str);

#pragma pack(1)

// Boot Record 数据部分数据结构
class BPB {
    public:
    uint16_t bytePerSector_; // 每个扇区的字节数
    uint8_t sectorPerCluster_; // 每个簇的扇区数
    uint16_t reservedSector_; // boot record 占用的扇区数
    uint8_t numOfFAT_; // FAT 的数量
    uint16_t rootEntryCount_; // 根目录文件数的最大值
    uint16_t sectorCount_; // 总扇区数
    uint8_t mediaDescriptor_; // 介质描述符
    uint16_t sectorPerFAT_; // 每个 FAT 的扇区数
    uint16_t sectorPerTrack_; //每个 track 的扇区数
    uint16_t numOfHead_; // 磁头的个数
    uint32_t hiddenSector_; // 隐藏扇区的个数
    uint32_t largeSectorCount_; // 如果 sectorCount 为 0，该值为总扇区数


    BPB(){}

    void initialize(FILE* fat12);
};

class FileNode {
    public:
    string name;
    vector<FileNode*> next;
    string path;
    uint32_t fileSize_;
    bool isFile_ = false;;
    bool isValid = true; // "." 和 ".." 属于 invalid 的 filenode
    int directoryCount_ = 0;
    int fileCount_ = 0;
    char* content = new char[10000];

    FileNode(){}

    FileNode(string name, string path, bool isFile, uint32_t fileSize);

    FileNode(string name, bool isValid);

    void setPath(string path){
        this->path = path;
    }

    void setName(string name){
        this->name = name;
    }

    string getPath(){return path;}

    string getName(){return name;}

    char* getContent();

    void addFileChildNode(FileNode* child);

    void addDirChildNode(FileNode* child);

    FileNode* findChildByName(string name);
};

class RootDirEntry {
    public:
    char filename[11]; // filename, first 8 char is filename, last 3 char is extension
    uint8_t attribute; // 文件属性, READ_ONLY = 0x01, HIDDEN = 0x02, SYSTEM = 0x04,
    // VOLUME_ID = 0x08, DIRECTORY = 0x10, ARCHIVE = 0x20, 
    uint8_t reservedForWIN; // reserved for windows NT
    uint8_t createTimeInTenths; // create time in tenths of seconds
    uint16_t createTime; // hour: 5 bit, minute: 6 bit, second, 5 bit, multiply second by 2
    uint16_t createDate; // year: 7 bit, month: 4 bit, day: 5 bit
    uint16_t highBitsOfClusterNumber; // FAT12 里面总是 0
    uint16_t lastAccessDate; 
    uint16_t lastModifiedTime;
    uint16_t lastModifiedDate;
    uint16_t clusterNumber; // 簇号，用这个来找文件的第一个簇的位置
    uint32_t fileSize; // 文件大小

    RootDirEntry(){}

    void initRootDirArea(FILE* fat12, FileNode* root);

    bool isEmptyName();

    bool isInvalidName();

    bool isFile();

    void makeFileName(char* name);

    void makeDirName(char* name);

    uint32_t getFileSize();

    uint16_t getFirstCluster();

    void fillFileContent(FILE* fat12, uint16_t clusterNumber, FileNode* file);

    void readChildrenNode(FILE* fat12, uint16_t clusterNumber, FileNode* rootNode);
};


// 检查参数, -l, -ll 为正确的形式, -L, -al 等都是错的
bool isLParams(const string &s);

// 检查形如 -[config] 的 token 是否满足 l 的要求
bool checkParamsL(vector<string>& cmds, bool &isL);

// 检查是否只指定了一个目录, 即只指定了一个，且为目录, 目标目录的 filenode 存到 target .
bool checkMultipleDirs(vector<string>& cmds, FileNode* &target, FileNode* root);

// handle cmd case: ls
void handleLS(FileNode* root);

void handleLSL(FileNode* root);

int getFATEntry(FILE* fat12, int index);

vector<string> tokenize(const string &str, const string &deliminator);

void doCAT(string filename, FileNode* root);

void handleLSCmd(vector<string> cmds, FileNode* root);

void handleCAT(vector<string> cmds, FileNode* root);

int main(){
    const char* fat_path = "./fat.img";
    FILE* fat12 = fopen(fat_path, "rb"); // 打开 FAT12 映像文件

    BPB* bpb = new BPB();
    bpb->initialize(fat12); // 读取 boot 记录数据部分并初始化后面要使用的全局变量
    FileNode* rootNode = new FileNode();
    rootNode->setPath("/");
    rootNode->setName("");

    RootDirEntry* rootEntry = new RootDirEntry();
    rootEntry->initRootDirArea(fat12, rootNode); 

    while(true){
        myPrintDefault(">");

        string cmd;
        getline(cin, cmd);

        vector<string> cmds = tokenize(cmd, " ");

        if(cmds[0] == "exit"){
            // 用户输入 exit，关闭文件并退出
            fclose(fat12);
            myPrintDefault(EXIT);
            break;
        }else if(cmds[0] == "cat"){
            handleCAT(cmds, rootNode);
        }else if(cmds[0] == "ls"){
            handleLSCmd(cmds, rootNode);
        }else {
            myPrintDefault(UNK_CMD);
        }
    }

    return 0;
}

void BPB::initialize(FILE* fat12){
    fseek(fat12, 11, SEEK_SET); // 从 11 字节处开始
    fread(this, 1, 25, fat12); // BPB 长 25 个字节

    // 初始化要使用的全局变量
    bytePerSector = this->bytePerSector_;
    sectorPerCluster = this->sectorPerCluster_;
    recordSectorCount = this->reservedSector_;
    numsOfFAT = this->numOfFAT_;
    rootEntryCount = this->rootEntryCount_;

    // FAT 占用的扇区数
    if(this->sectorCount_ != 0){
        FATSize = this->sectorPerFAT_;
    }else {
        FATSize = this->largeSectorCount_;
    }

    // FAT1 的起始地址: FAT1 在 boot record 后面
    fatBaseAddr = recordSectorCount * bytePerSector;
    // root 目录区的起始地址: 根目录区在 FAT2 后面
    rootBaseAddr = fatBaseAddr + numsOfFAT * FATSize * bytePerSector;
    // 数据区的起始地址: 数据区在根目录区后面
    dataBaseAddr = rootBaseAddr + (rootEntryCount * 32 + bytePerSector - 1) / bytePerSector * bytePerSector;
    bytePerCluster = bytePerSector * sectorPerCluster;
    
}

FileNode::FileNode(string name, bool isValid){
    this->name = name;
    this->isValid = isValid;
}

FileNode::FileNode(string name, string path, bool isFile, uint32_t fileSize){
    this->name = name;
    this->path = path;
    this->isFile_ = isFile; 
    this->fileSize_ = fileSize;
}

void FileNode::addFileChildNode(FileNode* child){
    this->next.push_back(child);
    this->fileCount_ ++;
}

void FileNode::addDirChildNode(FileNode* child){
    child->next.push_back(new FileNode(".", false));
    child->next.push_back(new FileNode("..", false));
    this->next.push_back(child);
    this->directoryCount_ ++;
}

FileNode* FileNode::findChildByName(string name){
    for(int i = 0; i < this->next.size(); i ++){
        if(this->next[i]->getName() == name){
            return this->next[i];
        }
    }

    return nullptr;
}

char* FileNode::getContent(){
    return this->content;
}

// 获取 FAT 第 index 个表项的值
int getFATEntry(FILE* fat12, int index){
    int base = fatBaseAddr;
    int fatEntryAddr = base + index * 3 / 2;

    int type = index % 2;

    uint16_t fatValue;
    uint16_t* ptr = &fatValue;
    fseek(fat12, fatEntryAddr, SEEK_SET);
    fread(ptr, 1, 2, fat12);

    if(type == 0){
        fatValue = fatValue << 4;
    }

    fatValue >>= 4;

    return fatValue;
}



vector<string> tokenize(const string &str, const string &deliminator){
    vector<string> res;

    if(str == ""){
        return res;
    }

    char* s = new char[str.size() + 1];
    strcpy(s, str.c_str());

    char* delim = new char[deliminator.size() + 1];
    strcpy(delim, deliminator.c_str());

    char* p = strtok(s, delim);

    while(p){
        string token = p;
        res.push_back(token);
        p = strtok(NULL, delim);
    }

    delete[] delim;
    delete[] s;
    
    return res;
}

// 检查参数, -l, -ll 为正确的形式, -L, -al 等都是错的
bool isLParams(const string &s){
    if(s[0] != '-'){
        return false;
    }else {
        int len = s.size();

        for(int i = 1; i < len; i ++){
            if(s[i] != 'l'){
                return false;
            }
        }
    }

    return true;
}

// 检查形如 -[config] 的 token 是否满足 l 的要求
bool checkParamsL(vector<string>& cmds, bool& isL){
    int len = cmds.size();

    for(int i = 1; i < len; i ++){
        if(cmds[i][0] == '-'){
            if(!isLParams(cmds[i]))return false;
            else isL = true;
        }
    }

    return true;
}

// 检查是否只指定了一个目录, 即只指定了一个，且为目录, 目标目录的 filenode 存到 target .
bool checkMultipleDirs(vector<string>& cmds, FileNode* &target, FileNode* root){
    int len = cmds.size();
    int cnt = 0;
    string dirName;

    for(int i = 1; i < len; i ++){
        // 不合法的目录名
        if(cmds[i][0] != '-' && cmds[i][0] != '/' && cmds[i][0] != '.' && (cmds[i][0] < 'A' || cmds[i][0] > 'Z')){
            return false;
        }else if(cmds[i][0] == '/' || cmds[i][0] == '.' || (cmds[i][0] >= 'A' && cmds[i][0] <= 'Z')){
            dirName = cmds[i];
            cnt ++;
        }
    }

    // 指定了多个目录名
    if(cnt > 1){
        return false;
    }

    // 不带目录名参数，默认为根目录
    if(cnt == 0){
        return true;
    }

    vector<string> pathSegs = tokenize(dirName, "/");

    int pathLen = pathSegs.size();

    FileNode* current = root;

    stack<FileNode*> visited;

    // 找到目标目录
    for(int i = 0; i < pathLen; i ++){
        if(pathSegs[i] == "."){
            continue;
        }else if(pathSegs[i] == ".."){
            if(visited.empty()){
                continue;
            }
            FileNode* parent = visited.top();
            visited.pop();
            current = parent;
            continue;
        }
        FileNode* child = current->findChildByName(pathSegs[i]);
        if(child == nullptr){
            myPrintDefault("File Not Found!\n");
            return false;
        }
        
        visited.push(current);

        current = child;
    }

    // 如果目标是文件, 那么不合法
    if(current->isFile_){
        return false;
    }

    target = current;

    return true;
}

// handle cmd case: ls
void handleLS(FileNode* root){
    if(root->isFile_){
        return;
    }

    string path = root->getPath() + ":\n";

    const char* pathStr = path.c_str();

    myPrintDefault(pathStr);

    vector<FileNode*> sub = root->next;

    int len = sub.size();

    for(int i = 0; i < len; i ++){
        if(sub[i]->isFile_){
            string name = sub[i]->getName() + "  ";
            const char* nameStr = name.c_str();
            myPrintDefault(nameStr);
        }else {
            string name = sub[i]->getName() + "  ";
            const char* nameStr = name.c_str();
            myPrintRed(nameStr);
        }
    }

    myPrintDefault("\n");

    for(int i = 0; i < len; i ++){
        if(sub[i]->isValid){
            handleLS(sub[i]);
        }
    }
}

void handleLSL(FileNode* root){
    if(root->isFile_){
        return;
    }

    // 算一下直接子目录和子文件的个数
    int fileCnt = 0, dirCnt = 0;

    vector<FileNode*> sub = root->next;
    int len = sub.size();

    for(int i = 0; i < len; i ++){
        // sub[i] 是 .. 或者 ., 不计入文件或者目录计数
        if(!sub[i]->isValid){
            continue;
        }

        if(sub[i]->isFile_){
            fileCnt ++;
        }else {
            dirCnt ++;
        }
    }

    // 打印当前目录, 直接子目录数, 子文件数
    string currentDirInfo = root->getPath() + " " + to_string(dirCnt) + " " + to_string(fileCnt) + ":\n";
    const char* currentDirInfoStr = currentDirInfo.c_str();
    myPrintDefault(currentDirInfoStr);

    // 打印当前目录下的直接子目录和子文件的信息
    for(int i = 0; i < len; i ++){
        // 计数的时候不考虑 . 和 .., 但是需要打印 
        if(!sub[i]->isValid){
            const char* temp = (sub[i]->getName() + "\n").c_str();
            myPrintRed(temp);
            continue;
        }
        // 如果是文件, 名字后面跟文件大小
        if(sub[i]->isFile_){
            string fileInfo = sub[i]->getName() + " " + to_string(sub[i]->fileSize_) + "\n";
            const char* fileInfoStr = fileInfo.c_str();
            myPrintDefault(fileInfoStr);
        }else {
            // 如果是目录, 目录名红色 直接子目录数 子文件数
            int fileCnt = 0, dirCnt = 0;
            int subLen = sub[i]->next.size();
            vector<FileNode*> &secondarySub = sub[i]->next;
            for(int j = 0; j < subLen; j ++){
                if(!secondarySub[j]->isValid){
                    continue;
                }
                if(secondarySub[j]->isFile_){
                    fileCnt ++;
                }else {
                    dirCnt ++;
                }
            }

            const char* dirName = (sub[i]->getName() + " ").c_str();

            myPrintRed(dirName);

            const char* cntInfo = (to_string(dirCnt) + " " + to_string(fileCnt) + "\n").c_str();

            myPrintDefault(cntInfo);
        }
    }
    
    myPrintDefault("\n");

    // 对每个直接子目录(除了 . 和 .. )做递归处理

    for(int i = 0; i < len; i ++){
        // 不处理 .. 和 .
        if(!sub[i]->isValid){
            continue;
        }

        if(sub[i]->isFile_){
            continue;
        }

        handleLSL(sub[i]);
    }
}



void handleLSCmd(vector<string> cmds, FileNode* root){
    // 处理 ls 不带参数的情况
    if(cmds.size() == 1){
        handleLS(root);
        return;
    }

    int len = cmds.size();

    // 此处检查选项是否合法
    bool isL = false;
    if(!checkParamsL(cmds, isL)){
        myPrintDefault(LS_CASE);
        myPrintDefault(INVALID_PARAMS);
        return;
    }
    FileNode* target = nullptr;
    // 此处检查是否只指定了小于等于一个目录
    if(!checkMultipleDirs(cmds, target, root)){
        myPrintDefault(LS_CASE);
        myPrintDefault(MULTIPLE_DIR_ERROR);
        return;
    }

    if(target == nullptr){
        if(isL){
            handleLSL(root);
        }else handleLS(root);
    }else {
        if(isL){
            handleLSL(target);
        }else handleLS(target);
    }

}

void outputFile(FileNode* file){
    if(!file->isFile_){
        myPrintDefault(CAT_CASE);
        myPrintDefault(INVALID_PARAMS);

        return;
    }

    myPrintDefault(file->getContent());
    myPrintDefault("\n");
}

void doCAT(string filename, FileNode* root){
    vector<string> pathSegs = tokenize(filename, "/");
    int segLen = pathSegs.size();

    FileNode* current = root;

    stack<FileNode*> visited;

    for(int i = 0; i < segLen-1; i ++){
        if(pathSegs[i] == "."){
            continue;
        }else if(pathSegs[i] == ".."){
            if(visited.empty()){
                continue;
            }
            FileNode* parent = visited.top();
            visited.pop();
            current = parent;
            continue;
        }
        FileNode* child = current->findChildByName(pathSegs[i]);
        if(child == nullptr || child->isFile_){
            myPrintDefault(CAT_CASE);
            myPrintDefault(INVALID_PARAMS);
            return;
        }

        visited.push(current);

        current = child;
    }

    current = current->findChildByName(pathSegs[segLen-1]);

    if(current == nullptr){
        myPrintDefault(CAT_CASE);
        myPrintDefault(INVALID_PARAMS);
        return;
    }

    outputFile(current);
}

void handleCAT(vector<string> cmds, FileNode* root){
    if(cmds.size() != 2){
        myPrintDefault(LS_CASE);
        myPrintDefault(INCORRECT_ARGS_AMOUNT);
        return;
    }

    doCAT(cmds[1], root);
}

void RootDirEntry::initRootDirArea(FILE* fat12, FileNode* root){
    int curAddr = rootBaseAddr;
    char name[12];

    for(int i = 0; i < rootEntryCount; i ++){
        // 依次读取每个根目录区表项
        fseek(fat12, curAddr, SEEK_SET);
        fread(this, 1, 32, fat12);

        curAddr += 32;

        if(!this->isEmptyName() && !this->isInvalidName()){
            if(this->isFile()){
                this->makeFileName(name);
                
                FileNode* child = new FileNode(name, root->getPath(), this->isFile(), this->fileSize);
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

static bool checkValidName(char c) {
    return ((c >= 'a') && (c <= 'z'))
           ||((c >= 'A') && (c <= 'Z'))
           ||((c >= '0') && (c <= '9'))
           ||((c == ' '));
}

bool RootDirEntry::isInvalidName(){
    int invalid = false;
    for (int k = 0; k < 11; ++k) {
        if (!checkValidName(this->filename[k])) {
            invalid = true;
            break;
        }
    }
    return invalid;
}

bool RootDirEntry::isFile(){
    uint8_t bits = this->attribute;

    return (bits & 0x10) == 0;
}

uint32_t RootDirEntry::getFileSize(){
    return this->fileSize;
}

uint16_t RootDirEntry::getFirstCluster(){
    return this->clusterNumber;
}

void RootDirEntry::makeFileName(char* name){
    int i = 0;

    // 前 8 位是文件名
    while(i < 8 && this->filename[i] != ' '){
        name[i] = this->filename[i];
        i ++;
    }

    name[i] = '.';

    i ++;

    //最后三位是扩展名
    int k = 8;

    while(k < 11 && this->filename[k] != ' '){
        name[i] = this->filename[k];
        i ++;
        k ++;
    }
    name[i] = '\0';
}

void RootDirEntry::makeDirName(char* name){
    int i = 0;
    while(i < 11 && this->filename[i] != ' '){
        name[i] = this->filename[i];
        i ++;
    }
    name[i] = '\0';
}

void RootDirEntry::fillFileContent(FILE* fat12, uint16_t clusterNumber, FileNode* file){
    int base = dataBaseAddr;

    int currentCluster = clusterNumber;

    int nextCluster = 0;

    char* ptr = file->getContent();

    if(clusterNumber == 0){
        return;
    }

    while(nextCluster < 0xFF8){
        nextCluster = getFATEntry(fat12, currentCluster);

        // 值为 0xFF7 说明是一个坏簇
        if(nextCluster == 0xFF7){
           myPrintDefault(BAD_CLUSTER);
            break;
        }

        char* content = new char[bytePerCluster];
        // 数据区从簇 2 开始
        int clusterStartByte = base + (currentCluster - 2) * bytePerCluster;

        fseek(fat12, clusterStartByte, SEEK_SET);
        fread(content, 1, bytePerCluster, fat12);

        for(int i = 0; i < bytePerCluster; i ++){
            *ptr = content[i];
            ptr ++;
        }

        delete[] content;

        currentCluster = nextCluster;
    }
}

void RootDirEntry::readChildrenNode(FILE* fat12, uint16_t clusterNumber, FileNode* rootNode){
    int base = dataBaseAddr;

    int currentCluster = clusterNumber;

    int nextCluster = 0;

    while(nextCluster < 0xFF8){
        nextCluster = getFATEntry(fat12, currentCluster);
        if(nextCluster == 0xFF7){
            // 0xFF7 表示坏簇
            myPrintDefault(BAD_CLUSTER);
            break;
        }

        int clusterStartByte = base + (currentCluster - 2) * bytePerCluster;

        int currentByte = 0;

        while(currentByte < bytePerCluster){
            RootDirEntry* rootEntry = new RootDirEntry();
            fseek(fat12, clusterStartByte + currentByte, SEEK_SET);
            fread(rootEntry, 1, 32, fat12);

            currentByte += 32;

            if(rootEntry->isEmptyName() || rootEntry->isInvalidName()){
                continue;
            }

            char name[12];

            if(rootEntry->isFile()){
                rootEntry->makeFileName(name);
                FileNode* child = new FileNode(name, rootNode->getPath(), true, rootEntry->getFileSize());
                rootNode->addFileChildNode(child);
                this->fillFileContent(fat12, rootEntry->getFirstCluster(), child);
            }else {
                rootEntry->makeDirName(name);
                FileNode* child = new FileNode();
                child->setName(name);
                child->setPath(rootNode->getPath() + name + "/");
                rootNode->addDirChildNode(child);
                this->readChildrenNode(fat12, rootEntry->getFirstCluster(), child);
            }

            delete rootEntry;
        }

        // 这个其实不需要，因为目录项只有 32 bytes
        currentCluster = nextCluster;
    }
}

void myPrintDefault(const char* str){
    my_print_default(str, strlen(str));
}

void myPrintRed(const char* str){
    my_print_red(str, strlen(str));
}