#include "FileNode.h"

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
    this->next.push_back(new FileNode(".", false));
    this->next.push_back(new FileNode("..", false));
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