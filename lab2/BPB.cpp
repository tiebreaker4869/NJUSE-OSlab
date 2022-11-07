#include "BPB.h"


using namespace std;

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