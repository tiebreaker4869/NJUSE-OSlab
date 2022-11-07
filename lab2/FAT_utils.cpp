#include "FAT_utils.h"

using namespace std;

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
bool check_multiple_dir(vector<string>& cmds, FileNode* target, FileNode* root){
    int len = cmds.size();
    int cnt = 0;
    string dir_name;

    for(int i = 1; i < len; i ++){
        // 不合法的目录名
        if(cmds[i][0] != '-' && cmds[i][0] != '/'){
            return false;
        }else if(cmds[i][0] == '-'){
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

    my_print_default(path_str, strlen(path_str));

    vector<FileNode*> sub = root->next;

    int len = sub.size();

    for(int i = 0; i < len; i ++){
        if(sub[i]->is_file){
            string name = sub[i]->getName() + " ";
            const char* name_str = name.c_str();
            my_print_default(name_str, strlen(name_str));
        }else {
            string name = sub[i]->getName() + " ";
            const char* name_str = name.c_str();
            my_print_red(name_str, strlen(name_str));
        }
    }

    my_print_default("\n", 1);

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
    my_print_default(current_dir_info_str, strlen(current_dir_info_str));

    // 打印当前目录下的直接子目录和子文件的信息
    for(int i = 0; i < len; i ++){
        // 计数的时候不考虑 . 和 .., 但是需要打印 
        if(!sub[i]->isValid){
            const char* temp = (sub[i]->getName() + "\n").c_str();
            my_print_red(temp, strlen(temp));
            continue;
        }
        // 如果是文件, 名字后面跟文件大小
        if(sub[i]->is_file){
            string file_info = sub[i]->getName() + " " + to_string(sub[i]->file_size) + "\n";
            const char* file_info_str = file_info.c_str();
            my_print_default(file_info_str, strlen(file_info_str));
        }else {
            // 如果是目录, 目录名红色 直接子目录数 子文件数
            int file_cnt = 0, dir_cnt = 0;

            for(int j = 0; j < len; j ++){
                if(!sub[j]->isValid){
                    continue;
                }
                if(sub[j]->is_file){
                    file_cnt ++;
                }else {
                    dir_cnt ++;
                }
            }

            const char* dir_name = (sub[i]->getName() + " ").c_str();

            my_print_red(dir_name, strlen(dir_name));

            const char* cnt_info = (to_string(dir_cnt) + " " + to_string(file_cnt) + "\n").c_str();

            my_print_default(cnt_info, strlen(cnt_info));
        }
    }

    my_print_default("\n", 1);

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
    }

    int len = cmds.size();

    // 此处检查选项是否合法
    if(!check_params_l(cmds)){
        char* error_msg = "Invalid params!\n";
        my_print_default(error_msg, strlen(error_msg));
        return;
    }

    FileNode* target = nullptr;
    // 此处检查是否只指定了小于等于一个目录
    if(!check_multiple_dir(cmds, target, root)){
        char* error_msg = "Multiple directory or invalid directory name!\n";
        my_print_default(error_msg, strlen(error_msg));
        return;
    }

    if(target == nullptr){
        handle_ls_l(root);
    }else {
        handle_ls_l(target);
    }

}

void handle_cat_cmd(vector<string> cmds, FileNode* root){

}