# 项目文档索引

本目录收录面向开发者与协作者的项目文档。文档结论以各仓库的实际代码与配置为准: 与代码不一致时先核对代码事实, 再修正文档或代码。

| 文档 | 内容 |
|:---:|:---:|
| [repo-layout.md](repo-layout.md) | 四个仓库的结构规范与目录约定 |
| [requirements.md](requirements.md) | 需求基线 R1 至 R18, 含明确剔除项, 平台约束与非目标 |
| [architecture.md](architecture.md) | 四仓关系, 客户端与服务端分层, 双端边界与数据流 |
| [assets-pipeline.md](assets-pipeline.md) | 素材明文工程文件与运行时二进制的转换流程 |
| [roadmap.md](roadmap.md) | 里程碑 M0 至 M10, 状态与验收标准 |
| [decisions.md](decisions.md) | 关键技术决策记录 |
| [dev-setup.md](dev-setup.md) | 开发环境要求, 构建命令与模拟器联调 |

## 一、阅读顺序

| 步骤 | 文档 | 目的 |
|:---:|:---:|:---:|
| 1 | [repo-layout.md](repo-layout.md) | 明确四个仓库的边界与目录约定 |
| 2 | [requirements.md](requirements.md) | 掌握不可违背的约束与明确剔除项 |
| 3 | [architecture.md](architecture.md) | 理解客户端与服务端的层次划分与接口面 |
| 4 | [assets-pipeline.md](assets-pipeline.md) | 掌握素材从工程文件到运行时二进制的路径 |
| 5 | [decisions.md](decisions.md) | 查阅既有决策, 避免重复讨论已定事项 |
| 6 | [dev-setup.md](dev-setup.md) | 搭好环境并跑通构建 |
| 7 | [roadmap.md](roadmap.md) | 对照里程碑目标与验收标准 |

## 二、维护约定

| 项 | 约定 |
|:---:|:---:|
| 内容来源 | 只写代码与配置能支撑的结论; 不确定处先读代码再落笔 |
| 标点 | 中文段落使用半角括号, 方括号, 逗号与分号; 不使用 emoji |
| 表格 | 分隔行统一使用居中对齐标记, 形如 `\|:---:\|` |
| 目录树 | 用代码块包裹, 使用 Unicode 框线字符 |
| 版本管理边界 | 只描述纳入版本管理的内容, 不描述工作目录与构建产物 |
| 交叉引用 | 需求, 决策与里程碑之间按编号引用, 不复制整段描述 |
