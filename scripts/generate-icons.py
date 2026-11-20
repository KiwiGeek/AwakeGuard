#!/usr/bin/env python3
"""Generate AwakeGuard application and tray icons (shield + watchful eye)."""

from __future__ import annotations

import struct
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError as exc:
    raise SystemExit(
        "Pillow is required: pip install pillow"
    ) from exc

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"

ACTIVE = (16, 185, 129, 255)  # #10B981
INACTIVE = (120, 120, 120, 255)
WHITE = (255, 255, 255, 255)
PUPIL_ACTIVE = (6, 95, 70, 255)
PUPIL_INACTIVE = (90, 90, 90, 255)

APP_SIZES = (16, 24, 32, 48, 64, 128, 256)
WEB_SIZES = (512, 1024)  # PNG for site / og:image / PWA
TRAY_SIZE = 32


def _scale(size: int, value: float) -> float:
    return value * size / 32.0


def _shield_polygon(size: int) -> list[tuple[float, float]]:
    """Rounded shield silhouette in a 32x32 coordinate space."""
    s = size / 32.0
    return [
        (16 * s, 3 * s),
        (26 * s, 7 * s),
        (26 * s, 17 * s),
        (16 * s, 29 * s),
        (6 * s, 17 * s),
        (6 * s, 7 * s),
    ]


def draw_icon(size: int, active: bool) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img, "RGBA")
    shield = _shield_polygon(size)
    stroke = ACTIVE if active else INACTIVE

    if active:
        fill = (16, 185, 129, 220)
        draw.polygon(shield, fill=fill, outline=stroke, width=max(1, round(_scale(size, 1.5))))
    else:
        draw.polygon(shield, fill=None, outline=stroke, width=max(1, round(_scale(size, 2))))

    cx, cy = size / 2.0, size / 2.0
    eye_w = _scale(size, 9)
    eye_h = _scale(size, 5.5)
    pupil_r = _scale(size, 2.2)

    sclera_box = [
        cx - eye_w,
        cy - eye_h,
        cx + eye_w,
        cy + eye_h,
    ]
    if active:
        draw.ellipse(sclera_box, fill=WHITE)
        draw.ellipse(
            [cx - pupil_r, cy - pupil_r, cx + pupil_r, cy + pupil_r],
            fill=PUPIL_ACTIVE,
        )
    else:
        draw.arc(
            sclera_box,
            start=200,
            end=340,
            fill=INACTIVE,
            width=max(1, round(_scale(size, 2))),
        )
        draw.line(
            [cx - eye_w * 0.55, cy, cx + eye_w * 0.55, cy],
            fill=INACTIVE,
            width=max(1, round(_scale(size, 1.5))),
        )

    return img


def save_png(path: Path, image: Image.Image) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="PNG")


def save_ico(path: Path, images: list[Image.Image]) -> None:
    """Write ICO with BMP frames (required for Windows XP; PNG-in-ICO is Vista+)."""
    path.parent.mkdir(parents=True, exist_ok=True)
    images[0].save(
        path,
        format="ICO",
        sizes=[(img.width, img.height) for img in images],
        append_images=images[1:],
        bitmap_format="bmp",
    )


# Standard Windows 16-colour (VGA) palette, in palette-index order.
VGA16 = [
    (0, 0, 0), (128, 0, 0), (0, 128, 0), (128, 128, 0),
    (0, 0, 128), (128, 0, 128), (0, 128, 128), (192, 192, 192),
    (128, 128, 128), (255, 0, 0), (0, 255, 0), (255, 255, 0),
    (0, 0, 255), (255, 0, 255), (0, 255, 255), (255, 255, 255),
]


def _nearest_vga16(rgb: tuple[int, int, int]) -> int:
    return min(range(16), key=lambda i: sum((a - b) ** 2 for a, b in zip(rgb, VGA16[i])))


def save_ico_win95(path: Path, images: list[Image.Image]) -> None:
    """Write a 4-bpp (16-colour) ICO with a 1-bpp AND mask.

    Windows 95 only understands 1/4/8-bpp icons; the 32-bpp alpha icons above
    (XP+) are not valid there. Pixels with alpha < 128 become transparent.
    """
    entries = []
    for img in images:
        w, h = img.width, img.height
        rgba = img.convert("RGBA")
        xor_stride = ((w * 4 + 31) // 32) * 4
        and_stride = ((w + 31) // 32) * 4
        xor_rows = []
        and_rows = []
        for y in range(h - 1, -1, -1):  # DIB rows are bottom-up
            xor_row = bytearray(xor_stride)
            and_row = bytearray(and_stride)
            for x in range(w):
                r, g, b, a = rgba.getpixel((x, y))
                index = 0
                if a < 128:
                    and_row[x // 8] |= 0x80 >> (x % 8)
                else:
                    index = _nearest_vga16((r, g, b))
                xor_row[x // 2] |= index << (4 if x % 2 == 0 else 0)
            xor_rows.append(bytes(xor_row))
            and_rows.append(bytes(and_row))
        header = struct.pack("<IiiHHIIiiII", 40, w, h * 2, 1, 4, 0, 0, 0, 0, 16, 0)
        palette = b"".join(bytes((b, g, r, 0)) for r, g, b in VGA16)
        entries.append((w, h, header + palette + b"".join(xor_rows) + b"".join(and_rows)))

    out = bytearray(struct.pack("<HHH", 0, 1, len(entries)))
    offset = 6 + 16 * len(entries)
    for w, h, data in entries:
        out += struct.pack("<BBBBHHII", w % 256, h % 256, 16, 0, 1, 4, len(data), offset)
        offset += len(data)
    for _, _, data in entries:
        out += data
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(out))


def generate_win95_icons() -> None:
    """16-colour icons for the 9x ROFLMAO build (assets/win95/)."""
    win95 = ASSETS / "win95"
    save_ico_win95(win95 / "AwakeGuard.ico", [draw_icon(32, active=True), draw_icon(16, active=True)])
    save_ico_win95(win95 / "tray-active.ico", [draw_icon(16, active=True)])
    save_ico_win95(win95 / "tray-inactive.ico", [draw_icon(16, active=False)])


def main() -> None:
    generate_win95_icons()

    app_images = [draw_icon(s, active=True) for s in APP_SIZES]
    save_ico(ASSETS / "AwakeGuard.ico", app_images)

    # Tray/shell: include 16px (notification area) and 32px (picker/alt DPI).
    tray_active = [draw_icon(16, active=True), draw_icon(32, active=True)]
    tray_inactive = [draw_icon(16, active=False), draw_icon(32, active=False)]
    save_ico(ASSETS / "tray-active.ico", tray_active)
    save_ico(ASSETS / "tray-inactive.ico", tray_inactive)
    save_png(ASSETS / "tray-active-32.png", tray_active[1])
    save_png(ASSETS / "tray-inactive-32.png", tray_inactive[1])

    for size in WEB_SIZES:
        save_png(ASSETS / f"AwakeGuard-{size}.png", draw_icon(size, active=True))

    print(f"Wrote icons under {ASSETS}")


if __name__ == "__main__":
    main()
