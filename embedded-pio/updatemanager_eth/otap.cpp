#include "otap.h"
#include <cstdint>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(OTAP_LOG, LOG_LEVEL_INF);


struct flash_img_context ctx;

void initSwapping()
{
    int err;

    uint8_t area_id=flash_img_get_upload_slot();

    err = flash_img_init_id(&ctx,
                            area_id);
    if (err) {
        LOG_ERR("flash_img_init_id failed: %d", err);
        return;
    }
 
    err = boot_erase_img_bank(area_id);
    if (err) {
        LOG_ERR("Failed to erase image bank: %d", err);
        return;
    }
}




int writeToBackup(uint8_t * new_data,size_t	len, bool flush){
    
    size_t bytes_written = flash_img_bytes_written(&ctx);

    if (bytes_written + len >ctx.flash_area->fa_size) {
        return -1;
    }

    return flash_img_buffered_write(&ctx, new_data, len, flush);

}


void setWriteToBackupDone(){

    boot_request_upgrade(BOOT_UPGRADE_TEST);

}
