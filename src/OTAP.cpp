#include "OTAP.h"




// We need to create a set of methods thaat allow for somone to write to flash howver



int offsset =0;
 struct flash_area *my_area;
 struct flash_area *my_area_main;
struct flash_img_context ctx;

// uint8_t area_id;


// intilizee the  flahs imiage context and 3
void initSwapping(void)
{
    int err;

    err = flash_img_init_id(&ctx,
                            FIXED_PARTITION_ID(slot1_partition));
    if (err) {
        printf("flash_img_init_id failed: %d\n", err);
        return;
    }



  err = flash_area_open(FIXED_PARTITION_ID(slot1_partition),
                          &my_area);
    if (err) {
        printf("Failed to open flash area: %d\n", err);
        return;
    }

  err = flash_area_open(FIXED_PARTITION_ID(slot0_partition),
                          &my_area_main);
    if (err) {
        printf("Failed to open flash area: %d\n", err);
        return;
    }

    
    err = boot_erase_img_bank(FIXED_PARTITION_ID(slot1_partition));
    if (err) {
        printf("Failed to erase image bank: %d\n", err);
        return;
    }
}



int writeToBackup(uint8_t * new_data,size_t	len, bool last){
    
    size_t bytes_written = flash_img_bytes_written(&ctx);

    if (bytes_written + len > my_area_main->fa_size) {
        return -1;
    }

    return flash_img_buffered_write(&ctx, new_data, len, last);

}



void writeToBackupDone(){


    boot_request_upgrade(BOOT_UPGRADE_TEST);


    //

}