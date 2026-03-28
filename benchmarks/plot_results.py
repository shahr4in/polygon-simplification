#!/usr/bin/env python3
"""Generate simple SVG benchmark plots from a results CSV."""

from __future__ import annotations

import argparse
import csv
import html
import math
from pathlib import Path


WIDTH = 1220
HEIGHT = 540
PLOT_LEFT = 80
PLOT_RIGHT = 320
PLOT_TOP = 40
PLOT_BOTTOM = 70
TABLE_TOP_GAP = 28
TABLE_ROW_HEIGHT = 22
TABLE_HEADER_HEIGHT = 24

PALETTE = [
    "#d1495b",
    "#edae49",
    "#00798c",
    "#30638e",
    "#003d5b",
    "#9c6644",
    "#6a994e",
    "#7b2cbf",
    "#ef476f",
    "#118ab2",
    "#8338ec",
    "#ff7f11",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input",
        default="benchmarks/results.csv",
        help="Input benchmark results CSV.",
    )
    parser.add_argument(
        "--output-dir",
        default="benchmarks/plots",
        help="Directory for generated SVG files.",
    )
    return parser.parse_args()


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    ok_rows = [row for row in rows if row.get("status", "ok") == "ok" and row.get("input_vertices") and row.get("runtime_mean_ms")]
    ok_rows.sort(key=lambda row: int(row["input_vertices"]))
    return ok_rows


def to_float(row: dict[str, str], key: str) -> float:
    return float(row[key])


def svg_plot(
    rows: list[dict[str, str]],
    x_key: str,
    y_key: str,
    title: str,
    y_label: str,
    output_path: Path,
    *,
    log_y: bool = False,
) -> None:
    if not rows:
        raise ValueError("No benchmark rows available for plotting.")

    xs = [to_float(row, x_key) for row in rows]
    ys = [to_float(row, y_key) for row in rows]

    x_min = min(xs)
    x_max = max(xs)
    log_x_min = math.log10(x_min)
    log_x_max = math.log10(x_max)
    if log_y:
        y_min = min(ys)
        y_max = max(ys)
        log_y_min = math.log10(y_min)
        log_y_max = math.log10(y_max)
    else:
        y_min = 0.0
        y_max = max(ys) * 1.1 if max(ys) > 0 else 1.0

    plot_width = WIDTH - PLOT_LEFT - PLOT_RIGHT
    plot_height = HEIGHT - PLOT_TOP - PLOT_BOTTOM
    table_columns = [
        ("Case", "case"),
        ("Vertices", "input_vertices"),
        ("Rings", "rings"),
        ("Runtime (ms)", y_key if y_key == "runtime_mean_ms" else "runtime_mean_ms"),
        ("Memory (KB)", "max_rss_mean_kb"),
    ]
    table_width = WIDTH - PLOT_LEFT - PLOT_RIGHT
    table_x = PLOT_LEFT
    table_y = HEIGHT + TABLE_TOP_GAP
    total_height = table_y + TABLE_HEADER_HEIGHT + TABLE_ROW_HEIGHT * len(rows) + 20

    def scale_x(value: float) -> float:
        if x_max == x_min:
            return PLOT_LEFT + plot_width / 2.0
        log_value = math.log10(value)
        return PLOT_LEFT + (log_value - log_x_min) / (log_x_max - log_x_min) * plot_width

    def scale_y(value: float) -> float:
        if log_y:
            if y_max == y_min:
                return PLOT_TOP + plot_height / 2.0
            log_value = math.log10(value)
            return PLOT_TOP + plot_height - (log_value - log_y_min) / (log_y_max - log_y_min) * plot_height
        if y_max == y_min:
            return PLOT_TOP + plot_height / 2.0
        return PLOT_TOP + plot_height - (value - y_min) / (y_max - y_min) * plot_height

    points = [
        (scale_x(x), scale_y(y), row["case"], PALETTE[index % len(PALETTE)])
        for index, (x, y, row) in enumerate(zip(xs, ys, rows))
    ]
    polyline = " ".join(f"{x:.2f},{y:.2f}" for x, y, _, _ in points)

    y_ticks = 5
    x_ticks = min(6, len(rows))

    lines: list[str] = []
    lines.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{total_height}" viewBox="0 0 {WIDTH} {total_height}">')
    lines.append('<rect width="100%" height="100%" fill="#fffdf7"/>')
    lines.append(f'<text x="{WIDTH / 2:.0f}" y="24" text-anchor="middle" font-family="Georgia, serif" font-size="20" fill="#1b1b1b">{html.escape(title)}</text>')

    lines.append(f'<line x1="{PLOT_LEFT}" y1="{PLOT_TOP + plot_height}" x2="{WIDTH - PLOT_RIGHT}" y2="{PLOT_TOP + plot_height}" stroke="#444" stroke-width="1.5"/>')
    lines.append(f'<line x1="{PLOT_LEFT}" y1="{PLOT_TOP}" x2="{PLOT_LEFT}" y2="{PLOT_TOP + plot_height}" stroke="#444" stroke-width="1.5"/>')

    if log_y:
        y_tick_values: list[float] = []
        for exponent in range(math.floor(log_y_min), math.ceil(log_y_max) + 1):
            tick = 10 ** exponent
            if y_min <= tick <= y_max:
                y_tick_values.append(float(tick))
        for y_value in y_tick_values:
            y_pos = scale_y(y_value)
            lines.append(f'<line x1="{PLOT_LEFT}" y1="{y_pos:.2f}" x2="{WIDTH - PLOT_RIGHT}" y2="{y_pos:.2f}" stroke="#e2dccf" stroke-width="1"/>')
            lines.append(f'<text x="{PLOT_LEFT - 10}" y="{y_pos + 4:.2f}" text-anchor="end" font-family="monospace" font-size="12" fill="#444">{y_value:.1f}</text>')
    else:
        for i in range(y_ticks + 1):
            y_value = y_min + (y_max - y_min) * i / y_ticks
            y_pos = scale_y(y_value)
            lines.append(f'<line x1="{PLOT_LEFT}" y1="{y_pos:.2f}" x2="{WIDTH - PLOT_RIGHT}" y2="{y_pos:.2f}" stroke="#e2dccf" stroke-width="1"/>')
            lines.append(f'<text x="{PLOT_LEFT - 10}" y="{y_pos + 4:.2f}" text-anchor="end" font-family="monospace" font-size="12" fill="#444">{y_value:.1f}</text>')

    tick_values: list[float] = []
    for exponent in range(math.floor(log_x_min), math.ceil(log_x_max) + 1):
        tick = 10 ** exponent
        if x_min <= tick <= x_max:
            tick_values.append(float(tick))
    for x_value in tick_values:
        x_pos = scale_x(x_value)
        lines.append(f'<line x1="{x_pos:.2f}" y1="{PLOT_TOP}" x2="{x_pos:.2f}" y2="{PLOT_TOP + plot_height}" stroke="#f0ebe0" stroke-width="1"/>')
        lines.append(f'<text x="{x_pos:.2f}" y="{PLOT_TOP + plot_height + 20}" text-anchor="middle" font-family="monospace" font-size="12" fill="#444">{int(x_value)}</text>')

    lines.append(f'<polyline fill="none" stroke="#1f6f8b" stroke-width="2.5" points="{polyline}"/>')
    for x_pos, y_pos, case_name, color in points:
        lines.append(f'<circle cx="{x_pos:.2f}" cy="{y_pos:.2f}" r="4.5" fill="{color}"/>')

    lines.append(f'<text x="{WIDTH / 2:.0f}" y="{HEIGHT - 18}" text-anchor="middle" font-family="Georgia, serif" font-size="15" fill="#1b1b1b">Input vertices (log scale)</text>')
    y_axis_label = y_label + (" (log scale)" if log_y else "")
    lines.append(
        f'<text x="22" y="{HEIGHT / 2:.0f}" text-anchor="middle" font-family="Georgia, serif" font-size="15" fill="#1b1b1b" transform="rotate(-90 22 {HEIGHT / 2:.0f})">{html.escape(y_axis_label)}</text>'
    )

    legend_x = WIDTH - PLOT_RIGHT + 20
    legend_y = PLOT_TOP + 12
    legend_width = PLOT_RIGHT - 40
    legend_line_height = 18
    legend_height = 36 + legend_line_height * len(points)
    lines.append(
        f'<rect x="{legend_x}" y="{legend_y}" width="{legend_width}" height="{legend_height}" rx="8" ry="8" fill="#fffaf0" stroke="#d8cfbf" stroke-width="1"/>'
    )
    lines.append(
        f'<text x="{legend_x + 12}" y="{legend_y + 20}" font-family="Georgia, serif" font-size="14" fill="#1b1b1b">Legend</text>'
    )
    for index, (_, _, case_name, color) in enumerate(points):
        y_pos = legend_y + 40 + index * legend_line_height
        lines.append(f'<circle cx="{legend_x + 14}" cy="{y_pos - 4}" r="4.5" fill="{color}"/>')
        lines.append(
            f'<text x="{legend_x + 28}" y="{y_pos}" font-family="monospace" font-size="11" fill="#333">{html.escape(case_name)}</text>'
        )

    column_widths = [240, 110, 90, 150, 150]
    lines.append(
        f'<rect x="{table_x}" y="{table_y}" width="{table_width}" height="{TABLE_HEADER_HEIGHT + TABLE_ROW_HEIGHT * len(rows)}" fill="#fffaf0" stroke="#d8cfbf" stroke-width="1"/>'
    )
    lines.append(
        f'<rect x="{table_x}" y="{table_y}" width="{table_width}" height="{TABLE_HEADER_HEIGHT}" fill="#f1e9d8" stroke="#d8cfbf" stroke-width="1"/>'
    )

    x_cursor = table_x
    for (header, _), width in zip(table_columns, column_widths):
        lines.append(
            f'<text x="{x_cursor + 8}" y="{table_y + 16}" font-family="monospace" font-size="12" fill="#1b1b1b">{html.escape(header)}</text>'
        )
        x_cursor += width
        if x_cursor < table_x + table_width:
            lines.append(
                f'<line x1="{x_cursor}" y1="{table_y}" x2="{x_cursor}" y2="{table_y + TABLE_HEADER_HEIGHT + TABLE_ROW_HEIGHT * len(rows)}" stroke="#d8cfbf" stroke-width="1"/>'
            )

    for row_index, row in enumerate(rows):
        row_top = table_y + TABLE_HEADER_HEIGHT + row_index * TABLE_ROW_HEIGHT
        if row_index > 0:
            lines.append(
                f'<line x1="{table_x}" y1="{row_top}" x2="{table_x + table_width}" y2="{row_top}" stroke="#e8e1d2" stroke-width="1"/>'
            )
        values = [
            row["case"],
            row["input_vertices"],
            row["rings"],
            f'{float(row["runtime_mean_ms"]):.3f}',
            f'{float(row["max_rss_mean_kb"]):.1f}',
        ]
        x_cursor = table_x
        for value, width in zip(values, column_widths):
            lines.append(
                f'<text x="{x_cursor + 8}" y="{row_top + 15}" font-family="monospace" font-size="12" fill="#333">{html.escape(str(value))}</text>'
            )
            x_cursor += width

    lines.append("</svg>")

    output_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    args = parse_args()
    input_path = Path(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    rows = load_rows(input_path)
    svg_plot(
        rows,
        x_key="input_vertices",
        y_key="runtime_mean_ms",
        title="Runtime vs Input Size",
        y_label="Mean runtime (ms)",
        output_path=output_dir / "runtime_vs_input_vertices.svg",
        log_y=True,
    )
    svg_plot(
        rows,
        x_key="input_vertices",
        y_key="max_rss_mean_kb",
        title="Memory vs Input Size",
        y_label="Mean max RSS (KB)",
        output_path=output_dir / "memory_vs_input_vertices.svg",
    )

    print(f"Wrote plots to {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
