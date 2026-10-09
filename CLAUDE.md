# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository overview

This is a fork of [vial-qmk](https://github.com/vial-kb/vial-qmk) (QMK firmware with Vial support) on the `vial` branch. The upstream tree (`quantum/`, `tmk_core/`, `platforms/`, `drivers/`, `keyboards/*`, `docs/`, etc.) is vanilla and should generally be left alone. The fork-specific work lives in:

- `keyboards/keebart/` — Keebart keyboards: `3w6_pro`, `corne_choc_pro`, `corne_mx_pro`, `sofle_choc_pro` (all RP2040).
- `users/manna-harbour_miryoku/` — Miryoku userspace, used by the `miryoku*` keymaps via `USER_NAME := manna-harbour_miryoku` in the keymap's `rules.mk`.

## Build commands

Use the QMK CLI (`qmk`) from the repo root; output is a `.uf2` file in the root. Keyboard path is `keebart/<board>[/<variant>]`:

```sh
qmk compile -kb keebart/corne_choc_pro/standard -km vial
qmk compile -kb keebart/corne_choc_pro/mini -km vial_mini_oled
# or: make keebart/corne_choc_pro/standard:miryoku_oled
```

Other tooling: `qmk lint -kb <kb>`, `qmk format-c`, `qmk format-json`, and `qmk test:all` / `make test:<name>` for the unit tests in `tests/`. Python deps: `requirements*.txt`.

## Keebart keyboard structure

- Boards with variants (`corne_choc_pro`, `corne_mx_pro`) have a shared `config.h`/`rules.mk`/`halconf.h`/`mcuconf.h` at the board level plus `standard/` (3x6+3, `LAYOUT_split_3x6_3`) and `mini/` (3x5+3, `LAYOUT_split_3x5_3`) subdirectories that each contain a `keyboard.json` (layout, RGB matrix LED map, default RGB animation, USB PID). `3w6_pro` and `sofle_choc_pro` have a single `keyboard.json` at the board level.
- Keymaps are shared across variants and sit in the board's `keymaps/` dir. Naming convention: `default`, `vial` (standard), `vial_mini`, `vial_oled`, `vial_mini_oled`, `miryoku`, `miryoku_oled`. Vial keymaps have a `vial.json` (keyboard layout for the Vial GUI) and enable `VIA_ENABLE`/`VIAL_ENABLE`/`VIALRGB_ENABLE` in `rules.mk`. OLED keymaps carry their own `bitmaps.h` and OLED code in `keymap.c`, and enable `OLED_ENABLE`, `WPM_ENABLE`, etc.
- Miryoku keymaps are thin: `keymap.c` just contains a license header, and `config.h` defines a `LAYOUT_miryoku` macro mapping Miryoku's 3x5+thumb grid onto the board's real layout (e.g. `LAYOUT_split_3x5_3`). The layers themselves come from the userspace.
- Changes affecting several boards (e.g. the default RGB animation) are normally applied to every `keyboard.json` across variants/boards, and per-board `readme.md` files list the keymaps and `qmk compile` commands — update them when adding a keymap or variant.
