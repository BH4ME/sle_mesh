# SLE PCB Enclosure v2.1.18 No Screen Top Lock Harder

Compact PCB-locking top-cover version: this STL is intentionally tight so the upper shell can clamp and retain the PCB.

This revision keeps the same outer top-cover shape and the same bottom-shell interface as `v2.1.17_no_screen_top_lock`, but increases the PCB retention force again so the board is more firmly trapped by the upper shell.

## What Changed

- Bottom shell stays unchanged and compatible with the already-printed `v2.1.12_no_screen` lower shell.
- Top cover outer dimensions stay unchanged at `37.8 x 57.6 x 14.05 mm`.
- Tightened the PCB slot again:
  - bottom clearance reduced to `0.00 mm`;
  - top clearance reduced to `0.04 mm`.
- Increased upper rail thickness and extended rail coverage farther along Y.
- Extended the rear support shelves so the PCB rear edge is better supported before it snaps under the latch faces.
- Deepened the rear latch engagement and increased the latch chamfer entry.
- Retention points at the PCB rear edge increased from three blocks to five total blocks:
  - left outer latch;
  - left inner latch;
  - center latch;
  - right inner latch;
  - right outer latch.

## Recommended Print File

Print this upper shell only:

- `sle_enclosure_v2_1_18_no_screen_top_lock_harder_top_cover.stl`

Reference files:

- `sle_enclosure_v2_1_18_no_screen_top_lock_harder_top_cover.step`
- `sle_enclosure_v2_1_18_no_screen_top_lock_harder_real_pcb_simulation.step`

## Validation

- Top cover STEP facts: `37.8 x 57.6 x 14.05 mm`.
- Top skin thickness remains `1.5 mm`.
- Real PCB STEP used: `E:/codex_documents/sle/3D/3D_PCB1_2_2026-05-31.step`.
- Real PCB import still reports the same nonfatal message during assembly generation:
  - `StepReaderData : Unresolved Reference : Fails Count : 2`
- Saved review snapshots:
  - `top_cover_iso_20260617T143005Z.png`
  - `top_cover_bottom_20260617T143005Z.png`
  - `real_pcb_simulation_iso_20260617T143005Z.png`

## Notes

- This revision is intentionally more aggressive than `v2.1.17`.
- First install will likely need a firmer push at the rear/antenna side so the PCB edge snaps under all latch blocks.
- Because the clearances are now extremely tight, small printer/material variance may matter. If PLA prints too hard to assemble, the next fallback is to slightly ease only the upper latch height rather than changing the whole shell shape.
