#include "RootDirEntry.h"

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
            my_print_default(error_message, strlen(error_message));
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
            my_print_default(error_message, strlen(error_message));
            break;
        }

        int cluster_start_byte = base + (current_cluster - 2) * byte_per_cluster;

        int current_byte = 0;

        while(current_byte < byte_per_cluster){
            unique_ptr<RootDirEntry> rootEntry = make_unique<RootDirEntry>();
            fseek(fat12, cluster_start_byte, SEEK_SET);
            fread(rootEntry.get(), 1, 32, fat12);

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
        }

        // 这个其实不需要，因为目录项只有 32 bytes
        current_cluster = next_cluster;
    }
}