#pragma once

#include <cstddef>
#include <cstdint>

namespace immirror::ui {

/*
 * KTX 容器里的第一层图像.
 * 界面子图集以 ETC1 压缩纹理随包下发, 容器头之后按层依次存放 "长度 + 数据".
 * 这里只解析第一层, 其余层留给上传时按需读取.
 */
struct KtxImage {
    bool valid = false;
    uint32_t width = 0;
    uint32_t height = 0;
    /* 容器里记录的压缩格式编号, 目前素材全部为 ETC1. */
    uint32_t internalFormat = 0;
    uint32_t mipLevels = 0;
    /* 第一层数据在原始字节里的偏移与长度. */
    size_t dataOffset = 0;
    size_t dataSize = 0;
};

/* 压缩格式编号: 未压缩以外的取值由容器原样给出. */
constexpr uint32_t kKtxEtc1Rgb8 = 0x8D64u;

/*
 * 解析 KTX 头部并定位第一层数据.
 * 结构不符, 字节序异常或长度越界时返回 valid 为 false 的结果.
 */
KtxImage ParseKtx(const uint8_t* bytes, size_t size);

/* 按 ETC1 的块尺寸推算某一层的字节数. */
size_t Etc1LevelBytes(uint32_t width, uint32_t height);

}  /* namespace immirror::ui */
