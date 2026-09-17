#include "mbr.h"

#include <algorithm>

namespace {

constexpr size_t kMbrSectorSize = 512;
constexpr size_t kMbrPartitionTableOffset = 0x1BE;
constexpr uint8_t kGridPartitionType = 0x47;

void writeLittleEndian32(uint8_t* destination, uint32_t value) {
    for (int byte = 0; byte < 4; ++byte) {
        destination[byte] = uint8_t(value >> (byte * 8));
    }
}

}  // namespace

std::optional<MbrImage> buildMbrImage(const std::vector<std::vector<uint8_t>>& partitions) {
    if (partitions.empty() || partitions.size() > 4) return std::nullopt;

    size_t totalPartitionBytes = 0;
    for (const auto& partition : partitions) {
        if (partition.empty() || partition.size() % kMbrSectorSize != 0) return std::nullopt;
        totalPartitionBytes += partition.size();
    }

    MbrImage image;
    image.data.resize(kMbrSectorSize + totalPartitionBytes, 0);
    size_t offset = kMbrSectorSize;
    for (size_t index = 0; index < partitions.size(); ++index) {
        const auto& partition = partitions[index];
        std::copy(partition.begin(), partition.end(), image.data.begin() + offset);

        uint8_t* entry = image.data.data() + kMbrPartitionTableOffset + index * 16;
        entry[0] = index == 0 ? 0x80 : 0x00;
        entry[4] = kGridPartitionType;
        writeLittleEndian32(entry + 8, uint32_t(offset / kMbrSectorSize));
        writeLittleEndian32(entry + 12, uint32_t(partition.size() / kMbrSectorSize));
        if (index == 0) {
            image.firstPartition = {0, true, true, offset, partition.size()};
        }
        offset += partition.size();
    }
    image.data[0x1FE] = 0x55;
    image.data[0x1FF] = 0xAA;
    return image;
}

bool isMbrDisk(const uint8_t* data, size_t size) {
    return size > 0x200 && data[0x1FE] == 0x55 && data[0x1FF] == 0xAA;
}

std::vector<MbrPartition> parseMbr(const uint8_t* data, size_t size) {
    std::vector<MbrPartition> partitions;
    partitions.reserve(4);

    if (size < 512) {
        return partitions;
    }

    const uint8_t* table = data + 0x1BE;
    for (size_t i = 0; i < 4; i++) {
        const uint8_t* entry = table + i * 16;

        bool isGRiD = entry[4] == 0x47;
        bool isActive = (entry[0] & 0x80) != 0;

        uint64_t part_offset = (entry[8] | entry[9] << 8 | entry[10] << 16 | entry[11] << 24) * 512;
        uint64_t part_size = (entry[12] | entry[13] << 8 | entry[14] << 16 | entry[15] << 24) * 512;

        // A zeroed entry marks the end of the used partition table.
        if (part_offset == 0 && part_size == 0 && !isGRiD && !isActive) {
            break;
        }

        partitions.push_back(MbrPartition {
            i,
            isGRiD,
            isActive,
            part_offset,
            part_size
        });
    }

    return partitions;
}
