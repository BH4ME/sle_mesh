# Versions

这个目录用于保存协议和代码版本记录。详细阶段说明统一在 `docs/v0`、`docs/v1`、`docs/v2`。

当前版本：

- [v3.0.0-alpha1](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v3.0.0-alpha1/VERSION.md)
- [v2.0.0-alpha18](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha18/VERSION.md)
- [v2.0.0-alpha17](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha17/VERSION.md)
- [v2.0.0-alpha16](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha16/VERSION.md)
- [v2.0.0-alpha15](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha15/VERSION.md)
- [v2.0.0-alpha14](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha14/VERSION.md)
- [v2.0.0-alpha13](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha13/VERSION.md)
- [v2.0.0-alpha12](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha12/VERSION.md)
- [v2.0.0-alpha11](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha11/VERSION.md)
- [v2.0.0-alpha10](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha10/VERSION.md)
- [v2.0.0-alpha9](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha9/VERSION.md)
- [v2.0.0-alpha8](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha8/VERSION.md)
- [v2.0.0-alpha7](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha7/VERSION.md)
- [v2.0.0-alpha6](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha6/VERSION.md)
- [v2.0.0-alpha5](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha5/VERSION.md)
- [v2.0.0-alpha4](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha4/VERSION.md)
- [v2.0.0-alpha3](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha3/VERSION.md)
- [v2.0.0-alpha2](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha2/VERSION.md)
- [v2.0.0-alpha1](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v2.0.0-alpha1/VERSION.md)
- [v1.2.10](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.10/VERSION.md)
- [v1.2.9](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.9/VERSION.md)
- [v1.2.8](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.8/VERSION.md)
- [v1.2.7](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.7/VERSION.md)
- [v1.2.6](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.6/VERSION.md)
- [v1.2.5](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.5/VERSION.md)
- [v1.2.4](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.4/VERSION.md)
- [v1.2.3](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.3/VERSION.md)
- [v1.2.2](/Users/bh4me_macair/Documents/Codex/sle_intercom/versions/v1.2.2/VERSION.md)

## 规则

- `VERSION.md`：版本说明
- `MANIFEST.md`：版本内容清单
- `VERSION.md` 只写这个版本解决了什么。
- `MANIFEST.md` 只写这个版本涉及哪些文件和怎么验证。
- 不再在每个版本目录复制大量源码快照，避免版本目录膨胀。

### V3 起版本管理约束

- 命名规则：`v3.0.0-alphaN`（功能开发阶段），后续按 `beta/rc/stable` 演进。
- 每完成一批功能都要提交一次并更新对应版本目录。
- 每次提交前至少执行本次变更影响范围内的自动化验证：
  - WebUI 变更：`npm --prefix webui test` 与 `npm --prefix webui run build`
  - 协议/状态机变更：补充并执行对应 C 侧验证用例
- `versions/README.md` 顶部“当前版本”必须指向最新版本。
