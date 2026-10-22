"""Построение графиков по CSV, экспортированному ctm_acq --verify.

Ожидаемые колонки: time_s, truth_c, raw_c, filtered_c.
"""

import argparse
import csv


def load(path):
    with open(path, newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    time = [float(r["time_s"]) for r in rows]
    truth = [float(r["truth_c"]) for r in rows]
    raw = [float(r["raw_c"]) for r in rows]
    filtered = [float(r["filtered_c"]) for r in rows]
    return time, truth, raw, filtered


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv")
    parser.add_argument("--out", default="telemetry.png")
    parser.add_argument("--start", type=float, default=18.0)
    parser.add_argument("--end", type=float, default=40.0)
    args = parser.parse_args()

    time, truth, raw, filtered = load(args.csv)

    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(9, 4))
    ax.plot(time, raw, color="#9aa0a6", lw=0.7, label="сырой сигнал")
    ax.plot(time, truth, color="black", lw=1.4, label="истинное значение")
    ax.plot(time, filtered, color="#c0392b", lw=1.6, label="после фильтрации")
    ax.set_xlim(args.start, args.end)
    ax.set_xlabel("время, с")
    ax.set_ylabel("температура, °C")
    ax.grid(ls=":", lw=0.6)
    ax.legend()
    fig.tight_layout()
    fig.savefig(args.out, dpi=200)
    print("saved", args.out)


if __name__ == "__main__":
    main()
