#!/usr/bin/env python3
import csv, sys, os
import matplotlib.pyplot as plt

path = sys.argv[1] if len(sys.argv) > 1 else "results/benchmark.csv"
with open(path, newline="", encoding="utf-8") as f:
    rows = list(csv.DictReader(f))
if not rows:
    raise SystemExit("No benchmark rows found. Run: make benchmark")
os.makedirs("results", exist_ok=True)
names = [r["mechanism"] for r in rows]
lat = [float(r["avg_round_trip_us"]) for r in rows]
throughput = [float(r["throughput_mib_s"]) for r in rows]

def chart(values, ylabel, title, filename):
    plt.figure(figsize=(10, 5.5))
    bars = plt.bar(names, values)
    plt.ylabel(ylabel); plt.title(title); plt.xticks(rotation=20, ha="right")
    plt.grid(axis="y", alpha=.25)
    for bar, value in zip(bars, values):
        plt.text(bar.get_x()+bar.get_width()/2, bar.get_height(), f"{value:.2f}", ha="center", va="bottom", fontsize=8)
    plt.tight_layout(); plt.savefig(filename, dpi=160); plt.close()

chart(lat, "Average round-trip latency (µs)", "IPC Nexus: IPC Round-Trip Latency", "results/latency_comparison.png")
chart(throughput, "Estimated throughput (MiB/s)", "IPC Nexus: Estimated Throughput", "results/throughput_comparison.png")
print("Generated results/latency_comparison.png and results/throughput_comparison.png")
