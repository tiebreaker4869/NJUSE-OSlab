#pragma once
#include "FileNode.h"
#include "RootDirEntry.h"
#include "variables.h"
#include "BPB.h"
#include "file_utils.h"
#include "my_print.h"


using namespace std;

// 检查参数, -l, -ll 为正确的形式, -L, -al 等都是错的
bool is_l_params(const string &s);

// 检查形如 -[config] 的 token 是否满足 l 的要求
bool check_params_l(vector<string>& cmds);

// 检查是否只指定了一个目录, 即只指定了一个，且为目录, 目标目录的 filenode 存到 target .
bool check_multiple_dir(vector<string>& cmds, FileNode* target, FileNode* root);

// handle cmd case: ls
void handle_ls(FileNode* root);

void handle_ls_l(FileNode* root);


void handle_ls_cmd(vector<string> cmds, FileNode* root);

void handle_cat_cmd(vector<string> cmds, FileNode* root);