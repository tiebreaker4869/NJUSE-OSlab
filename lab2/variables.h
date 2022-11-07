// 全局变量

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