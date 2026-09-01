#ifndef GRIDISKCOM_OMF_RECORD_H
#define GRIDISKCOM_OMF_RECORD_H

#include <cstddef>
#include <cstdint>

#include <vector>

constexpr uint8_t kOmfMetadataRecord = 0xFE;
constexpr uint8_t kOmfLabelRecord    = 0xFD;

inline uint16_t omfU16LE(const uint8_t* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

struct OmfRecord {
    uint8_t type = 0;
    const uint8_t* payload = nullptr;
    uint16_t length = 0;

    uint8_t subtype() const {
        return length ? payload[0] : 0;
    }
};

struct OmfMetadata {
    const uint8_t* content = nullptr;
    size_t contentSize = 0;

    std::vector<OmfRecord> records;

    const OmfRecord* find(uint8_t type, uint8_t subtype) const;
    const OmfRecord* findFirst(uint8_t type) const;
    std::vector<const OmfRecord*> findAll(uint8_t type, uint8_t subtype) const;
};

OmfRecord readOmfRecord(const uint8_t* p, const uint8_t* end);

OmfMetadata parseOmfMetadata(const uint8_t* data, size_t fileSize, uint32_t propLength);

#endif  // GRIDISKCOM_OMF_RECORD_H
