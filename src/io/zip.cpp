#include "zip.h"

#include <cstdint>

namespace io::zip {

namespace {

uint32_t crc32(const std::string& data) {
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char c : data) {
        crc ^= c;
        for (int i = 0; i < 8; ++i) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

void put16(std::string& o, uint16_t v) {
    o += static_cast<char>(v & 0xFF);
    o += static_cast<char>((v >> 8) & 0xFF);
}

void put32(std::string& o, uint32_t v) {
    for (int i = 0; i < 4; ++i) o += static_cast<char>((v >> (8 * i)) & 0xFF);
}

}

std::string build(const std::vector<Entry>& entries) {
    const uint16_t DOS_DATE = 0x0021;
    std::string out, central;

    for (const auto& e : entries) {
        uint32_t offset = static_cast<uint32_t>(out.size());
        uint32_t c      = crc32(e.data);
        uint32_t size   = static_cast<uint32_t>(e.data.size());

        put32(out, 0x04034b50);
        put16(out, 20);
        put16(out, 0);
        put16(out, 0);
        put16(out, 0);
        put16(out, DOS_DATE);
        put32(out, c);
        put32(out, size);
        put32(out, size);
        put16(out, static_cast<uint16_t>(e.name.size()));
        put16(out, 0);
        out += e.name;
        out += e.data;

        put32(central, 0x02014b50);
        put16(central, 20);
        put16(central, 20);
        put16(central, 0);
        put16(central, 0);
        put16(central, 0);
        put16(central, DOS_DATE);
        put32(central, c);
        put32(central, size);
        put32(central, size);
        put16(central, static_cast<uint16_t>(e.name.size()));
        put16(central, 0);
        put16(central, 0);
        put16(central, 0);
        put16(central, 0);
        put32(central, 0);
        put32(central, offset);
        central += e.name;
    }

    uint32_t cd_offset = static_cast<uint32_t>(out.size());
    out += central;

    put32(out, 0x06054b50);
    put16(out, 0);
    put16(out, 0);
    put16(out, static_cast<uint16_t>(entries.size()));
    put16(out, static_cast<uint16_t>(entries.size()));
    put32(out, static_cast<uint32_t>(central.size()));
    put32(out, cd_offset);
    put16(out, 0);
    return out;
}

bool extract(const std::string& archive, const std::string& entryName,
             std::string& out) {
    auto le16 = [&](size_t p) {
        return (uint16_t)((uint8_t)archive[p] | ((uint8_t)archive[p + 1] << 8));
    };
    auto le32 = [&](size_t p) {
        return (uint32_t)((uint8_t)archive[p] |
                          ((uint8_t)archive[p + 1] << 8) |
                          ((uint8_t)archive[p + 2] << 16) |
                          ((uint8_t)archive[p + 3] << 24));
    };

    size_t pos = 0;
    while (pos + 30 <= archive.size()) {
        if (le32(pos) != 0x04034b50u) break;
        uint16_t method    = le16(pos + 8);
        uint32_t compSize  = le32(pos + 18);
        uint16_t nameLen   = le16(pos + 26);
        uint16_t extraLen  = le16(pos + 28);
        size_t   nameStart = pos + 30;
        if (nameStart + nameLen + extraLen + compSize > archive.size()) break;

        if (archive.compare(nameStart, nameLen, entryName) == 0) {
            if (method != 0) return false;
            out.assign(archive, nameStart + nameLen + extraLen, compSize);
            return true;
        }
        pos = nameStart + nameLen + extraLen + compSize;
    }
    return false;
}

}