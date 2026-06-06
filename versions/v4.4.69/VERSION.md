# Version v4.4.69

Date: 2026-06-05

## Scope

`v4.4.69` records the resumed COM16-leader test attempt. Firmware baseline remains `v4.4.66`.

The original goal still requires a live four-board test with:

```text
COM16: leader
COM13: member/relay
COM17: member child
COM18: member child
leader direct cap: 1
```

## Current Evidence

```text
resume status log: <repo-root>\logs\serial\v4.4.68_resume_status_20260605_095007\confirm.log
COM16 manual-window burn log: <repo-root>\logs\burn\v4.4.68_com16_manual_window_20260605_095103\COM16_manual_window.log
final status log: <repo-root>\logs\serial\v4.4.69_final_status_20260605_095801\confirm.log
previous COM16 blocker doc: <repo-root>\versions\v4.4.68\COM16_BLOCKER.md
```

## Verification Result

```text
COM13 cfg status: PASS, fw=v4.4.66, route=241, suffix=E7F1
COM17 cfg status: PASS, fw=v4.4.66, route=224, suffix=E7E0
COM18 cfg status: PASS, fw=v4.4.66, route=86, suffix=5556
COM16 cfg status: FAIL, 0 bytes / no cfg-json
COM16 manual-window burn: FAIL, no boot handshake after 300 second manual retry window
four-board COM16 leader test: NOT COMPLETE, blocked by COM16 no app/bootloader response
```

## Decision

Do not rerun the same pure software reset/DTR/RTS/multi-baud/no-reset/manual-window attempts unless the external state changes. Software-side evidence now consistently shows `COM16` as a visible CH340 port with no board-side app UART or bootloader response.

Next meaningful progress requires one of:

```text
COM16 starts returning cfg-json
COM16 enters ROM boot handshake during a manual BOOT/RST action
the board/adapter/wiring for COM16 is changed
another responsive board is temporarily assigned to COM16's role for topology validation
```
