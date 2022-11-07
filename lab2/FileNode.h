#include <iostream>
#include <vector>

using namespace std;

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