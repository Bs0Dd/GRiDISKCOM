#include "omf_record.h"

OmfRecord readOmfRecord(const uint8_t* p, const uint8_t* end) {
    OmfRecord r;
    if (end - p < 3) {
        return r;  // not even a header fits
    }
    r.type = p[0];
    r.length = omfU16LE(p + 1);
    r.payload = p + 3;
    if (r.payload + r.length > end) {
        return OmfRecord{};  // payload doesn't fit
    }
    return r;
}

OmfMetadata parseOmfMetadata(const uint8_t* data, size_t fileSize, uint32_t propLength) {
    OmfMetadata md;

    const size_t metaEnd = (propLength <= fileSize) ? static_cast<size_t>(propLength) : fileSize;
    md.content = data + metaEnd;
    md.contentSize = fileSize - metaEnd;

    const uint8_t* p = data;
    const uint8_t* end = data + metaEnd;
    for (OmfRecord rec = readOmfRecord(p, end);
         rec.payload != nullptr;
         rec = readOmfRecord(p, end)) {
        md.records.push_back(rec);
        p = rec.payload + rec.length;
    }

    return md;
}

const OmfRecord* OmfMetadata::find(uint8_t type, uint8_t subtype) const {
    for (const OmfRecord& r : records) {
        if (r.type == type && r.subtype() == subtype) {
            return &r;
        }
    }
    return nullptr;
}

const OmfRecord* OmfMetadata::findFirst(uint8_t type) const {
    for (const OmfRecord& r : records) {
        if (r.type == type) {
            return &r;
        }
    }
    return nullptr;
}

std::vector<const OmfRecord*> OmfMetadata::findAll(uint8_t type, uint8_t subtype) const {
    std::vector<const OmfRecord*> out;
    for (const OmfRecord& r : records) {
        if (r.type == type && r.subtype() == subtype) {
            out.push_back(&r);
        }
    }
    return out;
}
