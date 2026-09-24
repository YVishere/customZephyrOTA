#ifndef OTAP_H
#define OTAP_H

#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>

#include <stdint.h>
#include <stddef.h>

void initSwapping();
int writeToBackup(uint8_t * new_data,size_t	len, bool flush);
void setWriteToBackupDone();

#endif
