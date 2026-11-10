import { defineConfig } from "vitest/config";

// 测试只针对 server 目录下的接口实现, 使用线程池避免依赖子进程
export default defineConfig({
  test: {
    include: ["tests/**/*.test.ts"],
    environment: "node",
    pool: "threads",
    poolOptions: {
      threads: {
        singleThread: true,
      },
    },
    fileParallelism: false,
    testTimeout: 20000,
    hookTimeout: 20000,
    reporters: "default",
  },
});
