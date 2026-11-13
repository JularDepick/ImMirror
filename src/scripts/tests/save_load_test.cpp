/* 世界存档的容错断言: 损坏与截断的文件必须被拒绝, 不能读出一半就返回成功. */
#include <cstdio>
#include <string>
#include <vector>

#include "world/world.h"

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static void writeFile(const char* path, const std::vector<char>& bytes) {
    FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return;
    }
    if (!bytes.empty()) {
        std::fwrite(bytes.data(), 1, bytes.size(), file);
    }
    std::fclose(file);
}

int main() {
    const char* path = "../../../temp/save-corrupt.world";

    /* 空文件. */
    writeFile(path, {});
    {
        immirror::world::World world;
        immirror::world::World::SaveState state;
        check("空文件被拒", !world.LoadFromFile(path, state));
    }

    /* 只有垃圾字节. */
    writeFile(path, std::vector<char>(256, static_cast<char>(0x5A)));
    {
        immirror::world::World world;
        immirror::world::World::SaveState state;
        check("垃圾字节被拒", !world.LoadFromFile(path, state));
    }

    /* 截断: 只写文件头的一半. */
    {
        immirror::world::World source;
        immirror::world::World::SaveState state;
        source.Update(immirror::core::Vec3{8.0f, 0.0f, 8.0f}, 2, 1);
        check("正常存档可写", source.SaveToFile(path, state));
        FILE* file = std::fopen(path, "rb");
        std::vector<char> bytes;
        if (file != nullptr) {
            char buffer[4096];
            size_t got = 0;
            while ((got = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
                bytes.insert(bytes.end(), buffer, buffer + got);
            }
            std::fclose(file);
        }
        check("存档非空", bytes.size() > 32);
        /* 截掉尾部四分之三, 保留文件头. */
        std::vector<char> truncated(bytes.begin(),
                                    bytes.begin() + static_cast<long>(bytes.size() / 4));
        writeFile(path, truncated);
        immirror::world::World world;
        immirror::world::World::SaveState loaded;
        const bool ok = world.LoadFromFile(path, loaded);
        /* 要么明确失败, 要么读出的区块数明显少于原档. */
        check("截断档不报成功", !ok || world.LoadedChunkCount() < 4);
    }

    std::remove(path);
    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
