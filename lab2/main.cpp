#include "FAT_utils.h"
#include "BPB.h"
#include "RootDirEntry.h"
#include "FileNode.h"
#include "my_print.h"

using namespace std;

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

    while(true){
        my_print_default(">", 1);

        string cmd;
        getline(cin, cmd);

        vector<string> cmds = tokenize(cmd, " ");

        if(cmds[0] == "exit"){
            // 用户输入 exit，关闭文件并退出
            fclose(fat12);
            char* exit_msg = "exit\n";
            my_print_default(exit_msg, strlen(exit_msg));
            break;
        }else if(cmds[0] == "cat"){
            handle_cat_cmd(cmds, rootNode);
        }else if(cmds[0] == "ls"){
            handle_ls_cmd(cmds, rootNode);
        }else {
            char* unknown_cmd = "Unknown command!\n";
            my_print_default(unknown_cmd, strlen(unknown_cmd));
        }
    }

    return 0;
}