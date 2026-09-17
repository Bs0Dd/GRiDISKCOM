#ifndef MBR_H
#define MBR_H

#include <cstdint>
#include <optional>
#include <vector>

struct MbrPartition {
    size_t   index;
    bool     isGRiD;
    bool     isActive;
    uint64_t offset;
    uint64_t size;
};

struct MbrImage {
    std::vector<uint8_t> data;
    MbrPartition firstPartition;
};

// Creates a GRiD-OS MBR image from up to four sector-aligned partition images.
std::optional<MbrImage> buildMbrImage(const std::vector<std::vector<uint8_t>>& partitions);

bool isMbrDisk(const uint8_t* data, size_t size);
std::vector<MbrPartition> parseMbr(const uint8_t* data, size_t size);

#endif // MBR_H
