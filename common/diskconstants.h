#ifndef DISKCONSTANTS_H
#define DISKCONSTANTS_H

#include <cstdint>

constexpr uint16_t GRID_BUBBLE_SECTOR_SIZE = 256;
constexpr uint16_t GRID_BUBBLE_SUPERBLOCK_FID = 0x3FE;
constexpr uint16_t GRID_BUBBLE_BITMAP_FID = 0x3FD;

constexpr uint16_t GRID_FLOPPY_SECTOR_SIZE = 512;
constexpr uint16_t GRID_FLOPPY_SUPERBLOCK_FID = 0x121;
constexpr uint16_t GRID_FLOPPY_BITMAP_FID = 0x120;

constexpr uint16_t GRID_HDD_SECTOR_SIZE = 512;
constexpr uint16_t GRID_HDD_SUPERBLOCK_FID = 0x2420;
constexpr uint16_t GRID_HDD_BITMAP_FID = 0x2400;

#endif  // DISKCONSTANTS_H
