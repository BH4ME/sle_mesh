# COM16 Leader Blocker

`COM16` remains the blocker for the requested four-board leader test.

Latest resumed evidence:

```text
<repo-root>\logs\serial\v4.4.68_resume_status_20260605_095007\confirm.log
<repo-root>\logs\burn\v4.4.68_com16_manual_window_20260605_095103\COM16_manual_window.log
<repo-root>\logs\serial\v4.4.69_final_status_20260605_095801\confirm.log
```

Observed:

```text
COM16 is visible as a CH340 port.
cfg status returns 0 bytes.
manual-window burn sends reboot/reset/AT+RST commands.
auto handshake times out.
300 second manual retry window times out.
no boot handshake is observed.
```

Previous software-side probes are recorded in:

```text
<repo-root>\versions\v4.4.68\COM16_BLOCKER.md
<repo-root>\versions\v4.4.67\COM16_BLOCKER.md
```

Do not mark the four-board test complete until COM16 returns:

```text
[cfg-json] ... "fw":"v4.4.66"
```
