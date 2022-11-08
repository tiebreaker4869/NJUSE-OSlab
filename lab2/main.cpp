#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

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
    uint16_t bytes_per_sector_; // 每个扇区的字节数
    uint8_t sector_per_cluster_; // 每个簇的扇区数
    uint16_t reserved_sector_; // boot record 占用的扇区数
    uint8_t FAT_num_; // FAT 的数量
    uint16_t MAX_FILE_NUM_IN_ROOT; // 根目录文件数的最大值
    uint16_t sector_count_; // 总扇区数
    uint8_t media_descriptor_; // 介质描述符
    uint16_t FAT_sector_count_; // 每个 FAT 的扇区数
    uint16_t sector_per_track_; //每个 track 的扇区数
    uint16_t num_of_heads_; // 磁头的个数
    uint32_t hidden_sector_; // 隐藏扇区的个数
    uint32_t large_sector_count_; // 如果 sector_count 为 0，该值为总扇区数


    BPB(){}

    void initialize(FILE* fat12);
};

class FileNode {
    public:
    string name;
    vector<FileNode*> next;
    string path;
    uint32_t file_size;
    bool is_file = false;;
    bool isValid = true; // "." 和 ".." 属于 invalid 的 filenode
    int directory_count = 0;
    int file_count = 0;
    char* content = new char[10000];

    FileNode(){}

    FileNode(string name, string path, bool is_file, uint32_t file_size);

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

    bool isValidNameAt(int j);

    bool isFile();

    void makeFileName(char* name);

    void makeDirName(char* name);

    uint32_t getFileSize();

    uint16_t getFirstCluster();

    void fillFileContent(FILE* fat12, uint16_t cluster_number, FileNode* file);

    void readChildrenNode(FILE* fat12, uint16_t cluster_number, FileNode* root_node);
};


// 检查参数, -l, -ll 为正确的形式, -L, -al 等都是错的
bool is_l_params(const string &s);

// 检查形如 -[config] 的 token 是否满足 l 的要求
bool check_params_l(vector<string>& cmds);

// 检查是否只指定了一个目录, 即只指定了一个，且为目录, 目标目录的 filenode 存到 target .
bool check_multiple_dir(vector<string>& cmds, FileNode* &target, FileNode* root);

// handle cmd case: ls
void handle_ls(FileNode* root);

void handle_ls_l(FileNode* root);

int getFATEntry(FILE* fat12, int index);

vector<string> tokenize(const string &str, const string &deliminator);

void do_cat(string filename, FileNode* root);

void handle_ls_cmd(vector<string> cmds, FileNode* root);

void handle_cat_cmd(vector<string> cmds, FileNode* root);

int main(){
    char* fat_path = "./fat.img";
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
            char* exit_msg = "exit\n";
            myPrintDefault(exit_msg);
            break;
        }else if(cmds[0] == "cat"){
            handle_cat_cmd(cmds, rootNode);
        }else if(cmds[0] == "ls"){
            handle_ls_cmd(cmds, rootNode);
        }else {
            char* unknown_cmd = "Unknown command!\n";
            myPrintDefault(unknown_cmd);
        }
    }

    return 0;
}

void BPB::initialize(FILE* fat12){
    fseek(fat12, 11, SEEK_SET); // 从 11 字节处开始
    fread(this, 1, 25, fat12); // BPB 长 25 个字节

    // 初始化要使用的全局变量
    byte_per_sector = this->bytes_per_sector_;
    sector_per_cluster = this->sector_per_cluster_;
    record_sector_count = this->reserved_sector_;
    nums_of_FAT = this->FAT_num_;
    root_entry_count = this->MAX_FILE_NUM_IN_ROOT;

    // cout << "bps: " << byte_per_sector << endl;
    // cout << "spc: " << sector_per_cluster << endl;
    // cout << "rsc: " << record_sector_count << endl;
    // cout << "nof: " << nums_of_FAT << endl;
    // cout << "rec: " << root_entry_count << endl;

    // FAT 占用的扇区数
    if(this->sector_count_ != 0){
        FAT_size = this->FAT_sector_count_;
    }else {
        FAT_size = this->large_sector_count_;
    }

    
    // cout << "FAT size: " << FAT_size << endl;
    // cout << "FAT NUM: " << nums_of_FAT << endl;
    

    // FAT1 的起始地址: FAT1 在 boot record 后面
    fat_base_addr = record_sector_count * byte_per_sector;
    // root 目录区的起始地址: 根目录区在 FAT2 后面
    root_base_addr = fat_base_addr + nums_of_FAT * FAT_size * byte_per_sector;
    // 数据区的起始地址: 数据区在根目录区后面
    data_base_addr = root_base_addr + (root_entry_count * 32 + byte_per_sector - 1) / byte_per_sector * byte_per_sector;
    byte_per_cluster = byte_per_sector * sector_per_cluster;

    
    // cout << "fat base: " << fat_base_addr << endl;
    // cout << "root base: " << root_base_addr << endl;
    // cout << "data base: " << data_base_addr << endl;
    // cout << "bpc: " << byte_per_cluster << endl;
    
}

FileNode::FileNode(string name, bool isValid){
    this->name = name;
    this->isValid = isValid;
}

FileNode::FileNode(string name, string path, bool is_file, uint32_t file_size){
    this->name = name;
    this->path = path;
    this->is_file = is_file; 
    this->file_size = file_size;
}

void FileNode::addFileChildNode(FileNode* child){
    this->next.push_back(child);
    this->file_count ++;
}

void FileNode::addDirChildNode(FileNode* child){
    child->next.push_back(new FileNode(".", false));
    child->next.push_back(new FileNode("..", false));
    this->next.push_back(child);
    this->directory_count ++;
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
bool is_l_params(const string &s){
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
bool check_params_l(vector<string>& cmds){
    int len = cmds.size();

    for(int i = 1; i < len; i ++){
        if(cmds[i][0] == '-'){
            if(!is_l_params(cmds[i]))return false;
        }
    }

    return true;
}

// 检查是否只指定了一个目录, 即只指定了一个，且为目录, 目标目录的 filenode 存到 target .
bool check_multiple_dir(vector<string>& cmds, FileNode* &target, FileNode* root){
    int len = cmds.size();
    int cnt = 0;
    string dir_name;

    for(int i = 1; i < len; i ++){
        // 不合法的目录名
        if(cmds[i][0] != '-' && cmds[i][0] != '/'){
            return false;
        }else if(cmds[i][0] == '/'){
            dir_name = cmds[i];
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

    vector<string> path_segs = tokenize(dir_name, "/");

    int path_len = path_segs.size();

    FileNode* current = root;

    // 找到目标目录
    for(int i = 0; i < path_len; i ++){
        FileNode* child = root->findChildByName(path_segs[i]);
        if(child == nullptr){
            //out << "Not Found" << endl;
            return false;
        }
        current = child;
    }

    // 如果目标是文件, 那么不合法
    if(current->is_file){
        return false;
    }

    target = current;

    return true;
}

// handle cmd case: ls
void handle_ls(FileNode* root){
    if(root->is_file){
        return;
    }

    string path = root->getPath() + ":\n";

    const char* path_str = path.c_str();

    myPrintDefault(path_str);

    vector<FileNode*> sub = root->next;

    int len = sub.size();

    for(int i = 0; i < len; i ++){
        if(sub[i]->is_file){
            string name = sub[i]->getName() + "  ";
            const char* name_str = name.c_str();
            myPrintDefault(name_str);
        }else {
            string name = sub[i]->getName() + "  ";
            const char* name_str = name.c_str();
            myPrintRed(name_str);
        }
    }

    myPrintDefault("\n");

    for(int i = 0; i < len; i ++){
        if(sub[i]->isValid){
            handle_ls(sub[i]);
        }
    }
}

void handle_ls_l(FileNode* root){
    if(root->is_file){
        return;
    }

    // 算一下直接子目录和子文件的个数
    int file_cnt = 0, dir_cnt = 0;

    vector<FileNode*> sub = root->next;
    int len = sub.size();

    for(int i = 0; i < len; i ++){
        // sub[i] 是 .. 或者 ., 不计入文件或者目录计数
        if(!sub[i]->isValid){
            continue;
        }

        if(sub[i]->is_file){
            file_cnt ++;
        }else {
            dir_cnt ++;
        }
    }

    // 打印当前目录, 直接子目录数, 子文件数
    string current_dir_info = root->getPath() + " " + to_string(dir_cnt) + " " + to_string(file_cnt) + ":\n";
    const char* current_dir_info_str = current_dir_info.c_str();
    myPrintDefault(current_dir_info_str);

    // 打印当前目录下的直接子目录和子文件的信息
    for(int i = 0; i < len; i ++){
        // 计数的时候不考虑 . 和 .., 但是需要打印 
        if(!sub[i]->isValid){
            const char* temp = (sub[i]->getName() + "\n").c_str();
            myPrintRed(temp);
            continue;
        }
        // 如果是文件, 名字后面跟文件大小
        if(sub[i]->is_file){
            string file_info = sub[i]->getName() + " " + to_string(sub[i]->file_size) + "\n";
            const char* file_info_str = file_info.c_str();
            myPrintDefault(file_info_str);
        }else {
            // 如果是目录, 目录名红色 直接子目录数 子文件数
            int file_cnt = 0, dir_cnt = 0;
            int sub_len = sub[i]->next.size();
            vector<FileNode*> &secondary_sub = sub[i]->next;
            for(int j = 0; j < sub_len; j ++){
                if(!secondary_sub[j]->isValid){
                    continue;
                }
                if(secondary_sub[j]->is_file){
                    file_cnt ++;
                }else {
                    dir_cnt ++;
                }
            }

            const char* dir_name = (sub[i]->getName() + " ").c_str();

            myPrintRed(dir_name);

            const char* cnt_info = (to_string(dir_cnt) + " " + to_string(file_cnt) + "\n").c_str();

            myPrintDefault(cnt_info);
        }
    }
    
    myPrintDefault("\n");

    // 对每个直接子目录(除了 . 和 .. )做递归处理

    for(int i = 0; i < len; i ++){
        // 不处理 .. 和 .
        if(!sub[i]->isValid){
            continue;
        }

        if(sub[i]->is_file){
            continue;
        }

        handle_ls_l(sub[i]);
    }
}



void handle_ls_cmd(vector<string> cmds, FileNode* root){
    // 处理 ls 不带参数的情况
    if(cmds.size() == 1){
        handle_ls(root);
        return;
    }

    int len = cmds.size();

    // 此处检查选项是否合法
    if(!check_params_l(cmds)){
        char* error_msg = "Invalid params!\n";
        myPrintDefault(error_msg);
        return;
    }

    FileNode* target = nullptr;
    // 此处检查是否只指定了小于等于一个目录
    if(!check_multiple_dir(cmds, target, root)){
        char* error_msg = "Multiple directory or invalid directory name!\n";
        myPrintDefault(error_msg);
        return;
    }

    if(target == nullptr){
        //cout << "target is nullptr" << endl;
        handle_ls_l(root);
    }else {
        //cout << "target is not nullptr" << endl;
        handle_ls_l(target);
    }

}

void outputFile(FileNode* file){
    if(!file->is_file){
        char* error_msg = "outputFile: args should be a file!\n";
        myPrintDefault(error_msg);

        return;
    }

    myPrintDefault(file->getContent());
    myPrintDefault("\n");
}

void do_cat(string filename, FileNode* root){
    vector<string> path_segs = tokenize(filename, "/");
    int len_segs = path_segs.size();

    FileNode* current = root;

    for(int i = 0; i < len_segs-1; i ++){
        FileNode* child = current->findChildByName(path_segs[i]);
        if(child == nullptr || child->is_file){
            char* error_msg = "do_cat: Invalid path!\n";
            myPrintDefault(error_msg);
            return;
        }

        current = child;
    }

    current = current->findChildByName(path_segs[len_segs-1]);

    if(current == nullptr){
        char* error_msg = "do_cat: Invalid path!\n";
        myPrintDefault(error_msg);
        return;
    }

    outputFile(current);
}

void handle_cat_cmd(vector<string> cmds, FileNode* root){
    if(cmds.size() != 2){
        char* error_msg = "cat: Incorrect amount of arguments!\n";
        myPrintDefault(error_msg);
        return;
    }

    do_cat(cmds[1], root);
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

bool RootDirEntry::isValidNameAt(int j) {
    return ((this->filename[j] >= 'a') && (this->filename[j] <= 'z'))
           ||((this->filename[j] >= 'A') && (this->filename[j] <= 'Z'))
           ||((this->filename[j] >= '0') && (this->filename[j] <= '9'))
           ||((this->filename[j] == ' '));
}

bool RootDirEntry::isInvalidName(){
    int invalid = false;
    for (int k = 0; k < 11; ++k) {
        if (!this->isValidNameAt(k)) {
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
    return this->file_size;
}

uint16_t RootDirEntry::getFirstCluster(){
    return this->cluster_number;
}



void RootDirEntry::makeFileName(char* name){
    int tmp = -1;
    for (int j = 0; j < 11; ++j) {
        if (this->filename[j] != ' ') {
            name[++tmp] = this->filename[j];
        } else {
            name[++tmp] = '.';
            while (this->filename[j] == ' ') j++;
            j--;
        }
    }
    ++tmp;
    name[tmp] = '\0';
}

void RootDirEntry::makeDirName(char* name){
    int tmp = -1;
    for (int k = 0; k < 11; ++k) {
        if (this->filename[k] != ' ') {
            name[++tmp] = this->filename[k];
        } else {
            tmp++;
            name[tmp] = '\0';
            break;
        }
    }
}

void RootDirEntry::fillFileContent(FILE* fat12, uint16_t cluster_number, FileNode* file){
    int base = data_base_addr;

    int current_cluster = cluster_number;

    int next_cluster = 0;

    char* ptr = file->getContent();

    if(cluster_number == 0){
        return;
    }

    while(next_cluster < 0xFF8){
        next_cluster = getFATEntry(fat12, current_cluster);

        // 值为 0xFF7 说明是一个坏簇
        if(next_cluster == 0xFF7){
            char* error_message = "error in fillFileContent: bad cluster, Something wrong with the file!";
           myPrintDefault(error_message);
            break;
        }

        char* content = new char[byte_per_cluster];
        // 数据区从簇 2 开始
        int cluster_start_byte = base + (current_cluster - 2) * byte_per_cluster;

        fseek(fat12, cluster_start_byte, SEEK_SET);
        fread(content, 1, byte_per_cluster, fat12);

        for(int i = 0; i < byte_per_cluster; i ++){
            *ptr = content[i];
            ptr ++;
        }

        delete[] content;

        current_cluster = next_cluster;
    }
}

void RootDirEntry::readChildrenNode(FILE* fat12, uint16_t cluster_number, FileNode* root_node){
    int base = data_base_addr;

    int current_cluster = cluster_number;

    int next_cluster = 0;

    while(next_cluster < 0xFF8){
        next_cluster = getFATEntry(fat12, current_cluster);
        if(next_cluster == 0xFF7){
            // 0xFF7 表示坏簇
            char* error_message = "error in readChildrenNode: bad cluster, something wrong in the file.";
            myPrintDefault(error_message);
            break;
        }

        int cluster_start_byte = base + (current_cluster - 2) * byte_per_cluster;

        int current_byte = 0;

        while(current_byte < byte_per_cluster){
            RootDirEntry* rootEntry = new RootDirEntry();
            fseek(fat12, cluster_start_byte + current_byte, SEEK_SET);
            fread(rootEntry, 1, 32, fat12);

            current_byte += 32;

            if(rootEntry->isEmptyName() || rootEntry->isInvalidName()){
                continue;
            }

            char name[12];

            if(rootEntry->isFile()){
                rootEntry->makeFileName(name);
                FileNode* child = new FileNode(name, root_node->getPath(), true, rootEntry->getFileSize());
                root_node->addFileChildNode(child);
                this->fillFileContent(fat12, rootEntry->getFirstCluster(), child);
            }else {
                rootEntry->makeDirName(name);
                FileNode* child = new FileNode();
                child->setName(name);
                child->setPath(root_node->getPath() + name + "/");
                root_node->addDirChildNode(child);
                this->readChildrenNode(fat12, rootEntry->getFirstCluster(), child);
            }

            delete rootEntry;
        }

        // 这个其实不需要，因为目录项只有 32 bytes
        current_cluster = next_cluster;
    }
}

void myPrintDefault(const char* str){
    my_print_default(str, strlen(str));
}

void myPrintRed(const char* str){
    my_print_red(str, strlen(str));
}