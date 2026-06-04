# Version v4.4.42

Date: 2026-06-03

## Scope

`v4.4.42` fixes the state-machine split between signal loss and manual leave.

## Root Cause

`v4.4.41` correctly made manual `leave` enter IDLE and clear the active leader so the member would not silently auto-rejoin. One firmware disconnect path still reused `sle_team_node_member_leave()` when an upstream parent/link failed. After the leave semantic changed, that path could treat signal loss like an operator leave, clearing the active leader and blocking automatic recovery.

## What Changed

1. Added `sle_team_node_member_link_lost()` for signal loss / upstream link loss recovery.
2. Link-lost recovery keeps the saved leader and relay permission, disables only active relay forwarding, and returns to DISCOVERING so tick retries HELLO automatically.
3. Firmware SLE disconnect handling now uses link-lost recovery instead of manual leave semantics.
4. Manual `leave` remains explicit: notify leader, enter IDLE, clear active leader/NV via firmware cleanup, and require manual rejoin.

## Expected Behavior

- Member reboot/power loss: leader sees offline, member restores saved leader from flash/NV, then rejoins without serial `join`.
- Member manual leave: leader sees offline immediately, member stays idle, and manual role/member configuration is required before rejoin.
- Relay/upstream parent loss: child node enters reselect/recovery, keeps leader config, and can reconnect through another parent or direct leader path.
