#pragma once
#include <iostream>
#include "variables.h"

using namespace std;

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