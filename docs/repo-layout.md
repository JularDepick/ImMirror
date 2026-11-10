# 仓库结构规范

本项目由四个彼此独立的 git 仓库组成。四个仓库之间没有公共祖先, 各自维护忽略规则与文档, 不允许把某个仓库的内容复制进另一个仓库的目录树。

## 一、顶层结构

```
ImMirror/
├── .gitignore      # git忽略规则
├── COPYRIGHT       # 版权文件
├── LICENSE         # 许可证文件
├── README.md       # 项目说明
├── docs/           # 项目文档
└── single-branch/  # 三个业务仓库 (主仓库忽略)
```

`single-branch/` 只是三个独立仓库的挂载点, 它不是任何仓库的内容; 主仓库与业务仓库都不追踪该目录。

## 二、各仓库职责

| 仓库 | 路径 | 职责 | 追踪内容 |
|:---:|:---|:---|:---|
| main | 根 | 展示窗口与项目文档 | README, docs, 许可证, 忽略规则 |
| client | `single-branch/client` | 客户端实现 | 源码, 构建脚本, 运行时资源子集 |
| server | `single-branch/server` | 服务端实现 | 源码, 配置样例, 文档 |
| assets | `single-branch/assets` | 素材转码管线 | 工具代码, 管线文档, 清点报告 |

## 三、仓库内部结构

### client

```
client/
├── .gitignore
├── README.md
└── src/
    ├── android/    # Gradle 工程
    ├── native/     # C++ 引擎
    ├── assets/     # 运行时资源子集
    ├── scripts/    # 构建与资源脚本
    └── README.md
```

### server

```
server/
├── .gitignore
├── README.md
└── src/
    ├── server/     # 路由, 服务, 数据层
    ├── docs/       # 接口与部署文档
    └── README.md
```

### assets

```
assets/
├── .gitignore
├── README.md
├── forge/          # 管线工具代码
├── docs/           # 管线文档与格式结论
├── tests/          # 管线单元测试
└── reports/        # 清点与验证报告
```

## 四、规则

1. 每个仓库的根目录只放 README, 许可证类文件, 忽略规则与一级目录, 不放散落的构建产物
2. 构建中间产物与解码产物都不进版本库, 按需重建
3. 源码只放在各自仓库的 `src/` 或 `forge/` 下, 脚本只放在 `scripts/` 下
4. 不允许在业务仓库内部出现 `single-branch/` 嵌套目录
5. 业务仓库之间不互相复制目录; 资源由 `assets` 管线生成后复制文件, 不复制整棵目录树
6. 新增一级目录前先更新本规范与对应仓库的 `README.md`

## 五、当前状态

| 检查项 | 状态 |
|:---:|:---|
| 四个仓库相互独立 | 是 |
| 客户端资源只保留运行时子集 | 是 |
| 解码工程树与构建产物不入库 | 是 |
| 业务仓库内无嵌套仓库目录 | 是 |
