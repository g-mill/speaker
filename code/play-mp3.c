/*
 * play mp3 --> wav -> bin file
 */
#include "rpi.h"
#include "pi-sd.h"
#include "fat32.h"
#include "i2s.h"

void notmain(void) {
    uart_init();
    kmalloc_init();
    pi_sd_init();

    printk("Reading the MBR.\n");
    mbr_t *mbr = mbr_read();

    printk("Loading the first partition.\n");
    mbr_partition_ent_t partition;
    memcpy(&partition, mbr->part_tab1, sizeof(mbr_partition_ent_t));
    assert(mbr_part_is_fat32(partition.part_type));

    printk("Loading the FAT.\n");
    fat32_fs_t fs = fat32_mk(&partition);

    printk("Loading the root directory.\n");
    pi_dirent_t root = fat32_get_root(&fs);

    printk("Looking for bin audio file.\n");
    char* name = "SAMPLE.BIN";  // CHANGE THIS DEPENDING ON BIN FILE
    pi_dirent_t *dirent = fat32_stat(&fs, &root, name);
    demand(dirent, "%s not found!\n", name);

    printk("Reading %s.\n", name);

    pi_dirent_t *cur_dirent = fat32_stat(&fs, &root, name);
    uint32_t length = get_cluster_chain_length(&fs, cur_dirent->cluster_id);
    uint32_t total_bytes = (&fs)->sectors_per_cluster * get_boot_sector().bytes_per_sec;
    uint32_t cur_cluster = cur_dirent->cluster_id;

    i2s_speaker_init();

    uint32_t remaining = cur_dirent->nbytes;
    for (uint32_t l = 0; l < length; l++) {
        uint8_t *direct_buf = (uint8_t *)kmalloc(total_bytes);
        cur_cluster = read_cur_cluster(&fs, cur_cluster, direct_buf);
        pi_file_t *file = kmalloc(sizeof(pi_file_t));
        uint32_t written = remaining < total_bytes ? remaining: total_bytes;
        *file = (pi_file_t) {
            .data = (char *)direct_buf,
            .n_data = written,
            .n_alloc = total_bytes,
        };
        
        long i = 0;
        int16_t *ptr = (int16_t *)file->data;  // change to int32_t if using 32 bit samples
        // int32_t *ptr= (int32_t *)file->data;
        while (i < file->n_data) {
            i2s_write_sample(*ptr >> 1);  // right-shifting by one makes the audio quieter, works better with speaker
            ptr += 1;
            i += 2;  // change to 4 if using 32 bit samples
        }
        remaining -= total_bytes;
    }       
}