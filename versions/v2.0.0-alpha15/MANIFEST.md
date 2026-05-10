# v2.0.0-alpha15 Manifest

## 变更范围

- `examples/relay_failover_suite.c`
- `scripts/simulate_v2.sh`
- `README.md`
- `versions/README.md`
- `versions/v2.0.0-alpha15/VERSION.md`
- `versions/v2.0.0-alpha15/MANIFEST.md`

## 关键改动

1. relay failover 仿真套件：
- 新增六类高风险场景；
- 验证 relay 断链补选、重连身份、失败回滚、边界抖动与回绕边界。

2. 场景化脚本：
- `simulate_v2.sh` 增加 `--suite`；
- `core`、`failover`、`all` 三种模式可独立执行。

3. 文档同步：
- README 补充 failover 日志路径；
- versions 索引增加 alpha15。

## 验证

```sh
./scripts/simulate_v2.sh --suite=failover --stress=50
./scripts/simulate_v2.sh --suite=core --stress=5
./scripts/simulate_20_members.sh --stress=50
```

结果：通过。
