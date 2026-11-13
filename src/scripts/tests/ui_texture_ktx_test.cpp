/* KTX 容器解析断言: 逐个体检界面子图集的压缩纹理. */
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "ui/ui_texture_ktx.h"

using namespace immirror::ui;

namespace {

int failures = 0;

void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

bool ReadAll(const std::filesystem::path& path, std::vector<uint8_t>& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.seekg(0, std::ios::end);
    const std::streamoff length = file.tellg();
    file.seekg(0, std::ios::beg);
    if (length <= 0) {
        return false;
    }
    out.resize(static_cast<size_t>(length));
    file.read(reinterpret_cast<char*>(out.data()), length);
    return file.good() || file.eof();
}

}  /* namespace */

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("用法: %s <子图集目录>\n", argv[0]);
        return 2;
    }
    int total = 0;
    int parsed = 0;
    int sized = 0;
    int etc1 = 0;
    std::string firstBad;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(argv[1])) {
        if (!entry.is_regular_file() || entry.path().extension() != ".ktx") {
            continue;
        }
        total += 1;
        std::vector<uint8_t> bytes;
        if (!ReadAll(entry.path(), bytes)) {
            firstBad = entry.path().string();
            continue;
        }
        const KtxImage image = ParseKtx(bytes.data(), bytes.size());
        if (!image.valid) {
            if (firstBad.empty()) {
                firstBad = entry.path().string();
            }
            continue;
        }
        parsed += 1;
        if (image.internalFormat == kKtxEtc1Rgb8) {
            etc1 += 1;
        }
        /* 第一层字节数必须与按块尺寸推算的一致, 否则上传时会读到别层的数据. */
        if (image.dataSize == Etc1LevelBytes(image.width, image.height)) {
            sized += 1;
        } else if (firstBad.empty()) {
            firstBad = entry.path().string();
        }
    }
    std::printf("纹理 %d 个, 解析成功 %d, 压缩格式为 ETC1 %d, 首层字节数吻合 %d\n", total,
                parsed, etc1, sized);
    if (!firstBad.empty()) {
        std::printf("首个异常: %s\n", firstBad.c_str());
    }
    check("纹理数量大于零", total > 0);
    check("全部解析成功", parsed == total);
    check("全部为 ETC1", etc1 == total);
    check("首层字节数全部吻合", sized == total);
    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
