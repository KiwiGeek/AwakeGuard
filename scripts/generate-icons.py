#!/usr/bin/env python3
"""Generate AwakeGuard application and tray icons (shield + watchful eye)."""

from __future__ import annotations

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


def main() -> None:
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
