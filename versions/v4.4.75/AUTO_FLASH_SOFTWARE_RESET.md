# v4.4.75 Auto Flash Procedure

## Rule

Use software reset only. Do not ask for manual reset unless the board cannot
answer serial commands at all.

Current reliable reset path:

```text
software-reset-only + single reboot command + post-package handshake
```

Success evidence:

```text
Auto reset: sending CLI command 'reboot'
Establishing ymodem session...
Transferring ws63-liteos-app-sign.bin...
Done. Reseting device...
exit=0
```

## Single Board

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16 `
  -ExpectedVersion v4.4.75 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

## Four Boards In Parallel

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.75 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

## Logs

Every run creates:

```text
logs\burn\v4.4.75_<timestamp>\run_summary.txt
logs\burn\v4.4.75_<timestamp>\<PORT>.log
logs\burn\v4.4.75_<timestamp>\<PORT>.command.txt
```

If a flash fails, inspect the port log first. Do not retry with manual reset as
the first response; use the log to identify whether the board accepted `reboot`,
entered ymodem, transferred all images, or failed after reset.
