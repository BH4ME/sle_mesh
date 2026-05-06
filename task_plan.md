# Task Plan: SLE 1vs20 and Auto-Routing Feasibility

## Goal
Define a practical implementation path for this WS63 SLE project to support logical 1vs20 networking with automatic routing or forwarding while preserving the current proven 1vs8 baseline.

## Current Phase
Phase 3

## Phases

### Phase 1: Requirements & Discovery
- [x] Understand user intent
- [x] Identify constraints and requirements
- [x] Document findings in findings.md
- **Status:** complete

### Phase 2: Planning & Structure
- [x] Define technical approach
- [x] Compare true 1vs20 vs pseudo 1vs20 vs hierarchical routing
- [x] Document decisions with rationale
- **Status:** complete

### Phase 3: Implementation
- [x] Choose first implementation slice
- [x] Gate relay activation on leader approval / joined state
- [x] Split logical team capacity from physical SLE connection capacity
- [x] Add pending downstream candidate table for callback direction race
- [x] Add first-hop route table for indirect return paths
- [x] Add explicit upstream parent state + route reselection flag
- [x] Emit `HELLO` + `ROUTE_UPDATE` after upstream reconnect
- [x] Default approvals to no-relay unless explicitly granted
- [x] Fail closed when unicast route resolution has no next hop
- [x] Keep tests and logs aligned with new behavior
- **Status:** in_progress

### Phase 4: Testing & Verification
- [ ] Verify routing/forwarding behavior
- [ ] Verify coexistence impact on WiFi + SLE
- [ ] Document test results in progress.md
- **Status:** pending

### Phase 5: Delivery
- [ ] Summarize recommended architecture
- [ ] Deliver implementation plan or code changes
- [ ] Note remaining risks and next steps
- **Status:** pending

## Key Questions
1. Does the current WS63 + SDK stack reliably support more than 8 simultaneous SLE links in this product shape?
2. If not, which architecture best satisfies the user's "1vs20" requirement: polling, hierarchical relays, or hybrid routing?
3. What is the smallest code-first slice that creates visible progress without destabilizing the existing 1vs8 baseline?

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| Treat current system as a proven 1vs8 star baseline | The repo, sample code, and field notes all center on official `sle_uart_1_vs_8` behavior and validated multi-member joins. |
| Separate "automatic routing" from "true 20 concurrent links" | The user goal can likely be met by routing/forwarding or scheduling even if the SDK cannot hold 20 active links at once. |
| Use file-based planning for this effort | The task spans architecture, SDK constraints, and staged implementation decisions, so persistent planning files reduce context loss. |
| Implement approve-gated hierarchical relay as the next slice | The latest review says current relay forwarding exists but does not yet enforce the user's trust model: leader approval first, then relay behavior. |

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| GitHub curated skill listing returned HTTP 403 | 1 | Installed needed skills directly from GitHub repo paths instead of using the curated list API. |
| `python` command not found for planning catchup | 1 | Re-ran the same script with `python3`. |

## Notes
- Re-read this plan before choosing between polling, hierarchical routing, and true concurrent expansion.
- Preserve the current verified 1vs8 path as fallback during any implementation work.
