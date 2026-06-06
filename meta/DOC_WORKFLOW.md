# 文档工作流程（当前约定）

这份文件定义“以后怎么写文档、放哪里、什么时候更新”。

## 1. 文档分层

1. 根目录文档（操作类）
- `README.md`：编译、烧录、验证、审查脚本入口。
- `meta/PROJECT_OPERATION_SOP.md`：工程强制作业 SOP；每次改代码、远程编译、自动烧录和版本管理前必须先读。
- `meta/DOC_WORKFLOW.md`：本文档（流程规范）。
- `meta/review_feedback.md`：每次最新审查结果（覆盖写入）。

2. 阶段文档（方案类）
- `docs/v0/`：1vs2 / 1vs8 基线阶段。
- `docs/v1/`：手动 relay 批准阶段。
- `docs/v2/`：自动 relay / 自动选父 / 自愈阶段。
- `docs/v3/`：手机定位桥接与位置分发阶段。
- `docs/v4/`：WS63 模块 + ST7789 + 失联上报阶段。

3. 版本账本（审计类）
- `versions/`：每次里程碑的 `VERSION.md` + `MANIFEST.md`。
- 用于追踪“哪次改了什么、怎么验证的”，不替代阶段文档。

## 2. 写作规则

1. 新功能先判断归属阶段：v0 / v1 / v2 / v3 / v4，只改对应目录。
2. 同一主题优先“增量更新已有文档”，不新建同类重复 md。
3. 每个阶段优先保持 2-4 份核心文档，避免拆太碎。
4. 草稿、临时记录、review 中间产物不长期保留在 docs 主路径。

## 3. 审查流程

1. 审查规则文档固定：`docs/v2/review_framework.md`。
2. 执行脚本：`scripts/run_review_with_service.sh`。
3. 输出文件固定：`meta/review_feedback.md`（覆盖写入）。

## 4. 版本更新时机

在以下情况更新 `versions/`：

1. 完成一批可独立描述的功能。
2. 关键行为变更（协议、路由、配对、重连策略）。
3. 审查结论通过并准备留档。

最少动作：

1. 更新对应版本 `VERSION.md`（说清“做了什么”）。
2. 更新对应版本 `MANIFEST.md`（说清“改了哪些文件、怎么验证”）。
3. 更新 `versions/README.md` 的当前版本列表顺序（新版本放最前）。
4. 如果远程编译、自动烧录、版本管理或硬件排查流程变化，同步更新 `meta/PROJECT_OPERATION_SOP.md`。

## 5. 清理策略

1. 能合并就合并：同主题多文档合并成一份。
2. 能删除就删除：重复、过时、无引用文档直接删。
3. 删除前确保：
- 不影响根目录操作文档；
- 不影响当前阶段（v0~v4）主线理解；
- 不影响 `versions/` 的历史可追溯性。

## 6. 快速检查清单

每次文档改动后检查：

1. `find docs -maxdepth 4 -type f | sort` 是否仍简洁。
2. `README.md` 与 `docs/README.md` 入口是否可用（v0~v4）。
3. `scripts/run_review_with_service.sh --dry-run` 是否正常。
4. `rg` 检查是否还有指向已删除 md 的链接。

## 7. 给审查服务（审查服务）的执行指令

每次审查必须按这个顺序：

1. 先读 `docs/v2/review_framework.md`。
2. 根据当前 Scope 选择 Stage（不要跳步）。
3. 审查时先核对 README 声明，再看历史反馈，再看本次新增逻辑。
4. 输出必须覆盖写入`meta/review_feedback.md`。
5. 报告必须带完整头信息：Reviewer / Date / Version / Branch / Scope。
6. 结论必须给出 Blocker/Warning/Note 数量和最终结论（通过/有条件通过/拒绝）。

禁忌：

1. 不要把审查结果写到 `docs/` 子目录。
2. 不要保留多份“本次审查”副本文件。
3. 不要只给口头结论而不更新 `meta/review_feedback.md`。

## 8. 给维护者的执行指令

每次改文档我都按下面做：

1. 先判断变更属于 `v0 / v1 / v2 / v3 / v4` 哪条线，只改对应目录。
2. 同主题优先改现有文档，不新建重复 md。
3. 改完后跑链接与结构检查（第 6 节清单）。
4. 需要审查时，使用 `scripts/run_review_with_service.sh`，并确认输出落在`meta/review_feedback.md`。
5. 如果是里程碑改动，同步更新 `versions/`（`VERSION.md` + `MANIFEST.md` + `versions/README.md`）。
6. 保持根目录“操作文档少而清晰”，避免再次堆叠临时文档。
