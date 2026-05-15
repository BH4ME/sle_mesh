#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEMO = ROOT / "xc" / "ws63_st7789_135x240_demo"


def read_text(path):
    return path.read_text(encoding="utf-8")


def test_demo_contains_expected_artifacts():
    expected = [
        DEMO / "README.md",
        DEMO / "CMakeLists.txt",
        DEMO / "Kconfig",
        DEMO / "src" / "ws63_st7789_demo.c",
    ]

    missing = [path for path in expected if not path.exists()]
    assert missing == []


def test_demo_defaults_match_requested_wiring_and_panel_size():
    kconfig = read_text(DEMO / "Kconfig")
    source = read_text(DEMO / "src" / "ws63_st7789_demo.c")
    readme = read_text(DEMO / "README.md")

    for token in [
        "default 7",
        "default 9",
        "default 8",
        "default 10",
        "default 6",
        "135",
        "240",
    ]:
        assert token in kconfig

    for token in [
        "CONFIG_ST7789_TFT_SCL_PIN",
        "CONFIG_ST7789_TFT_SDA_PIN",
        "CONFIG_ST7789_TFT_CS_PIN",
        "CONFIG_ST7789_TFT_DC_PIN",
        "CONFIG_ST7789_TFT_RESET_PIN",
        "ST7789_WIDTH 135",
        "ST7789_HEIGHT 240",
        "uapi_spi_master_write",
        "app_run(st7789_demo_entry)",
    ]:
        assert token in source

    for line in [
        "TFT VCC      -> WS63 3V3",
        "TFT GND      -> WS63 GND",
        "TFT SCL      -> WS63 GPIO7",
        "TFT SDA      -> WS63 GPIO9",
        "TFT CS       -> WS63 GPIO8",
        "TFT RS       -> WS63 GPIO10",
        "TFT RESET    -> WS63 GPIO6",
    ]:
        assert line in readme
