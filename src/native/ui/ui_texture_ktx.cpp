#include "ui/ui_texture_ktx.h"

#include <cstring>

namespace immirror::ui {
namespace {

/* 容器标识: 12 字节, 前 4 字节为魔数, 后续为版本与换行标记. */
constexpr uint8_t kIdentifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
                                     0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
constexpr size_t kHeaderSize = 64;

uint32_t ReadU32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

}  /* namespace */

size_t Etc1LevelBytes(uint32_t width, uint32_t height) {
    /* ETC1 每 4x4 像素一个 8 字节块, 不足一块按一块算. */
    const size_t blocksX = (width + 3u) / 4u;
    const size_t blocksY = (height + 3u) / 4u;
    return blocksX * blocksY * 8u;
}

KtxImage ParseKtx(const uint8_t* bytes, size_t size) {
    KtxImage image;
    if (bytes == nullptr || size < kHeaderSize) {
        return image;
    }
    if (std::memcmp(bytes, kIdentifier, sizeof(kIdentifier)) != 0) {
        return image;
    }
    /* 头部固定 13 个 32 位字段, 紧接标识之后. */
    const uint8_t* fields = bytes + 12;
    const uint32_t endianness = ReadU32(fields + 0);
    /* 小端记为 0x04030201; 大端容器的字节序不同, 这里只接受小端. */
    if (endianness != 0x04030201u) {
        return image;
    }
    image.internalFormat = ReadU32(fields + 16);
    image.width = ReadU32(fields + 24);
    image.height = ReadU32(fields + 28);
    image.mipLevels = ReadU32(fields + 44);
    const uint32_t keyValueBytes = ReadU32(fields + 48);
    if (image.width == 0 || image.height == 0) {
        return image;
    }
    /* 键值段按 4 字节对齐后才是第一层的长度字段. */
    size_t offset = kHeaderSize + static_cast<size_t>(keyValueBytes);
    if (offset + 4u > size) {
        return image;
    }
    const uint32_t levelBytes = ReadU32(bytes + offset);
    offset += 4u;
    if (levelBytes == 0 || offset + levelBytes > size) {
        return image;
    }
    image.dataOffset = offset;
    image.dataSize = levelBytes;
    image.valid = true;
    return image;
}

}  /* namespace immirror::ui */
