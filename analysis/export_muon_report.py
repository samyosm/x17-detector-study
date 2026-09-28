import argparse
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import numpy as np
import pandas as pd
import uproot


WINDOWS = {"Be signal": (16, 20), "Be control": (12, 16),
           "He signal": (18, 22), "He control": (14, 18)}
FIELDS = ["event_id", "n_hit", "arm_1", "arm_2", "energy_sum_MeV",
          "energy_asymmetry", "opening_angle_deg", "opening_angle_z25mm_deg"]


def read_coincidences(path):
    multiplicity = np.zeros(17, dtype=np.int64)
    frames = []
    with uproot.open(path, handler=uproot.source.file.MemmapSource) as source:
        for batch in source["cosmic_events"].iterate(FIELDS, step_size="64 MB", library="np"):
            counts = np.bincount(batch["n_hit"], minlength=len(multiplicity))
            if len(counts) > len(multiplicity):
                multiplicity = np.pad(multiplicity, (0, len(counts) - len(multiplicity)))
            multiplicity += counts
            selected = batch["n_hit"] == 2
            if selected.any():
                frames.append(pd.DataFrame({name: batch[name][selected] for name in FIELDS}))
    pairs = pd.concat(frames, ignore_index=True) if frames else pd.DataFrame(columns=FIELDS)
    if pairs["event_id"].duplicated().any():
        raise ValueError("Duplicate coincidence event IDs: select one merged run")
    valid = np.isfinite(pairs[FIELDS[4:]].to_numpy(dtype=float)).all(axis=1)
    return multiplicity, pairs.loc[valid].copy(), int((~valid).sum())


def select_window(pairs, bounds):
    energy = pairs["energy_sum_MeV"]
    return pairs.loc[energy.ge(bounds[0]) & energy.lt(bounds[1])]


def summarize(source, multiplicity, pairs, invalid):
    manifest = Path(str(source) + ".run.txt")
    record = manifest.read_text() if manifest.is_file() else ""
    rates = [line.split(":", 1)[1] for line in record.splitlines() if line.startswith("Source rate Hz:")]
    total = int(multiplicity.sum())
    summary = {
        "source_file": source.name, "source_bytes": source.stat().st_size,
        "run_manifest": record or None,
        "incident_rate_hz": float(rates[0]) if rates else None,
        "total": total, "multiplicity": multiplicity.tolist(),
        "pairs": len(pairs), "invalid_pairs": invalid,
        "symmetric": int(pairs["energy_asymmetry"].lt(0.5).sum()),
        "two_arm_efficiency": int(multiplicity[2]) / total if total else None,
        "windows": {},
    }
    for name, bounds in WINDOWS.items():
        selected = select_window(pairs, bounds)
        symmetric = int(selected["energy_asymmetry"].lt(0.5).sum())
        summary["windows"][name] = {
            "all": len(selected), "symmetric": symmetric,
            "asymmetric": len(selected) - symmetric,
            "events": selected.drop(columns="n_hit").to_dict(orient="records"),
        }
    for label, column in [("energy", "energy_sum_MeV"), ("angle", "opening_angle_deg")]:
        summary[f"{label}_quantiles"] = (
            pairs[column].quantile([0, 0.01, 0.1, 0.5, 0.9, 0.99, 1]).to_dict()
            if len(pairs) else {}
        )
    if len(pairs):
        maximum = max(1, int(np.ceil(pairs["energy_sum_MeV"].max())))
        energy_edges = np.arange(maximum + 1)
        energy_counts, _ = np.histogram(pairs["energy_sum_MeV"], bins=energy_edges)
        modal = int(np.argmax(energy_counts))
        summary["energy_modal_bin"] = energy_edges[modal:modal + 2].tolist()
    summary["angle_120_160"] = int(pairs["opening_angle_deg"].between(120, 160).sum())
    return summary


def draw_counts(axis, values, edges, **style):
    counts, _ = np.histogram(values, bins=edges)
    axis.stairs(counts, edges, **style)


def plot_energy_sharing(pairs, energy_edges, output):
    figure, axes = plt.subplots(2, 1, figsize=(3.6, 3.5), layout="constrained")
    draw_counts(axes[0], pairs["energy_sum_MeV"], energy_edges, color="black")
    for low, high, color, label in [(16, 20, "tab:blue", "Be: 16–20 MeV"),
                                    (18, 22, "tab:orange", "He: 18–22 MeV")]:
        axes[0].axvspan(low, high, color=color, alpha=0.2, label=label)
    axes[0].set(xlabel="Summed energy [MeV]", ylabel="Events / 1 MeV",
                xlim=(0, energy_edges[-1]))
    axes[0].legend()
    draw_counts(axes[1], pairs["energy_asymmetry"], np.linspace(0, 1, 51), color="black")
    axes[1].axvline(0.5, color="tab:red", linestyle="--")
    axes[1].set(xlabel="Absolute energy asymmetry", ylabel="Events / 0.02", xlim=(0, 1))
    figure.savefig(output / "energy_asymmetry.pdf")
    plt.close(figure)


def plot_energy_angle(pairs, energy_edges, angle_edges, output):
    figure, axis = plt.subplots(figsize=(3.6, 2.8), layout="constrained")
    counts, _, _ = np.histogram2d(pairs["opening_angle_deg"], pairs["energy_sum_MeV"],
                                 bins=(angle_edges, energy_edges))
    mesh = axis.pcolormesh(angle_edges, energy_edges, np.ma.masked_equal(counts.T, 0),
                          cmap="viridis", norm=LogNorm(vmin=1, vmax=max(2, counts.max())))
    for low, high, color, label in [(16, 20, "tab:blue", "Be"), (18, 22, "tab:orange", "He")]:
        axis.axhspan(low, high, color=color, alpha=0.2)
        axis.text(3, (low + high) / 2, label, fontsize=7, va="center")
    axis.set(xlabel="Opening angle [°]", ylabel="Summed energy [MeV]",
             xlim=(0, 180), ylim=(0, energy_edges[-1]))
    figure.colorbar(mesh, ax=axis, label="Events / bin (log scale)")
    figure.savefig(output / "energy_angle.pdf")
    plt.close(figure)


def plot_beryllium_angles(pairs, angle_edges, output):
    figure, axes = plt.subplots(2, 1, figsize=(3.6, 3.5), layout="constrained", sharex=True, sharey=True)
    for axis, name in zip(axes, ["Be signal", "Be control"]):
        bounds = WINDOWS[name]
        selected = select_window(pairs, bounds)
        for symmetric, color, style, label in [(True, "tab:blue", "-", "A < 0.5"),
                                                (False, "tab:red", "--", "A ≥ 0.5")]:
            sample = selected.loc[selected["energy_asymmetry"].lt(0.5) == symmetric]
            draw_counts(axis, sample["opening_angle_deg"], angle_edges, color=color,
                        linestyle=style, label=f"{label}: {len(sample)} events")
        axis.set(title=f"{name}: {bounds[0]}–{bounds[1]} MeV", ylabel="Events / 5°", xlim=(0, 180))
        axis.legend(loc="upper left")
    axes[-1].set_xlabel("Opening angle [°]")
    figure.savefig(output / "beryllium_angles.pdf")
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description="Export the three cosmic-muon report figures and their counts.")
    parser.add_argument("root_file", type=Path)
    parser.add_argument("--output", type=Path, default=Path("docs/site/assets/muon_report"))
    args = parser.parse_args()
    multiplicity, pairs, invalid = read_coincidences(args.root_file)
    if not multiplicity.sum():
        raise ValueError("The cosmic event tree is empty")
    args.output.mkdir(parents=True, exist_ok=True)
    summary = summarize(args.root_file, multiplicity, pairs, invalid)
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2, allow_nan=False) + "\n")
    pairs.to_csv(args.output / "coincidences.csv", index=False)
    maximum = max(90, int(np.ceil(pairs["energy_sum_MeV"].max()))) if len(pairs) else 90
    energy_edges, angle_edges = np.arange(maximum + 1), np.arange(0, 181, 5)
    plt.rcParams.update({"font.size": 9, "axes.spines.top": False, "axes.spines.right": False,
                         "pdf.fonttype": 42, "axes.titlesize": 10, "legend.fontsize": 7.5})
    plot_energy_sharing(pairs, energy_edges, args.output)
    plot_energy_angle(pairs, energy_edges, angle_edges, args.output)
    plot_beryllium_angles(pairs, angle_edges, args.output)
    print(f"Read {summary['total']:,} incident muons; {len(pairs):,} finite two-arm coincidences.")
    print(f"Saved three PDFs, summary.json and coincidences.csv in {args.output}")


if __name__ == "__main__":
    main()
