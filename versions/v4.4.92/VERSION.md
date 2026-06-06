# Version v4.4.92

Date: 2026-06-05

## Scope

`v4.4.92` fixes the v4.4.91 four-board relay recovery failure from the current logs, not the older v4.4.74 feedback trail.

The observed failure was:

- COM13 relay received child `HELLO child->leader` packets.
- The relay-side node handler returned `ret=-4` before `relay forwarded packet`.
- The leader later showed the relay role dropping, which removed the recovery path that offline known children needed to rejoin through the relay.

## Root Cause

Relay rebalance demand was still driven primarily by currently online members plus temporary pending members.

That misses the exact failure case: if a child is offline and its `HELLO` is stopped at the relay, the leader never sees it as pending. A topology with known deployed children can therefore shrink the computed relay target and demote the only relay path needed for recovery.

## Fix

- Firmware visible version bumped to `v4.4.92`.
- Relay target now uses deployment demand: `max(online_count, known_member_count) + pending_count`.
- `known_member_count` includes leader member records, explicit allowlist members, and active relay-failover watched members.
- Pending members are counted only if they do not already have a member record, avoiding double count.
- Rebalance diagnostics now print `online`, `known`, `pending`, `demand`, `direct_cap`, current `relay`, and computed `target`.
- Existing relay rejection diagnostics remain in place so future logs show whether a child `HELLO` was blocked before forwarding.

## Verification

Planned:

- Python unit tests for WS63 automation.
- WebUI contract tests.
- `git diff --check`.
- Remote Ubuntu firmware build.
- Hardware flash and four-board validation only after explicit continue.
