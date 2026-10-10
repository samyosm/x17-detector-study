import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from model_runs import COSINE_EDGES, ENERGY_EDGES_GEV, WINDOWS, read_run, select_window

SOURCES = {
    "b4": ("B4 generator", "#D55E00", "o"),
    "guan_b4_plane": ("Joint flux, B4 plane", "#0072B2", "s"),
    "guan_production_plane": ("Joint flux, production plane", "#009E73", "^"),
}
COSINE_CENTERS = (COSINE_EDGES[1:] + COSINE_EDGES[:-1]) / 2
ENERGY_CENTERS = np.sqrt(ENERGY_EDGES_GEV[1:] * ENERGY_EDGES_GEV[:-1])
PAIR_PANELS = [("energy_sum_MeV", np.arange(0, 81, 2), "Summed energy [MeV]"),
               ("energy_asymmetry", np.linspace(0, 1, 21), "Energy asymmetry"),
               ("opening_angle_deg", np.arange(0, 181, 10), "Opening angle [°]")]


def density(values, edges):
    counts = np.histogram(values, bins=edges)[0].astype(float)
    norm = max(counts.sum(), 1) * np.diff(edges)
    return counts / norm, np.sqrt(counts) / norm


def binomial(selected, total):
    with np.errstate(divide="ignore", invalid="ignore"):
        fraction = np.where(selected > 0, selected / total, np.nan)
        return fraction, np.sqrt(fraction * (1 - fraction) / total)


def finish(figure, axes, output, name, legend_axis=0):
    for axis in np.ravel(axes):
        axis.grid(alpha=0.2, linewidth=0.5)
    np.ravel(axes)[legend_axis].legend(frameon=False, fontsize=7.5)
    figure.savefig(output / name)
    plt.close(figure)


def draw_source(runs, output):
    figure, axes = plt.subplots(1, 2, figsize=(7.2, 2.7), layout="constrained")
    for key in ["b4", "guan_production_plane"]:
        label, color, _ = SOURCES[key]
        incident = runs[key]["incident"]
        by_cosine, by_energy = incident.sum(axis=1), incident.sum(axis=0)
        axes[0].stairs(by_cosine / by_cosine.sum() / np.diff(COSINE_EDGES), COSINE_EDGES,
                       color=color, label=label.replace(", production plane", ""))
        axes[1].stairs(by_energy / by_energy.sum() / np.diff(np.log10(ENERGY_EDGES_GEV)),
                       ENERGY_EDGES_GEV, color=color)
    pure = COSINE_CENTERS**2 / np.sum(COSINE_CENTERS**2 * np.diff(COSINE_EDGES))
    axes[0].plot(COSINE_CENTERS, pure, "k--", linewidth=0.8, label=r"$\cos^2\theta$")
    axes[0].set(xlabel=r"$\cos\theta$", ylabel="Probability density")
    axes[1].set(xlabel="Kinetic energy [GeV]", ylabel="Probability per decade",
                xscale="log", yscale="log", ylim=(1e-3, 1))
    finish(figure, axes, output, "source.pdf")


def draw_response(runs, output):
    figure, axes = plt.subplots(1, 2, figsize=(7.2, 2.9), layout="constrained")
    for key, (label, color, marker) in SOURCES.items():
        run = runs[key]
        for axis, axis_index, centers in [(axes[0], 1, COSINE_CENTERS),
                                          (axes[1], 0, ENERGY_CENTERS)]:
            value, error = binomial(run["coincident"].sum(axis=axis_index),
                                    run["incident"].sum(axis=axis_index))
            axis.errorbar(centers, value, yerr=error, fmt=marker + "-", ms=3, lw=1,
                          color=color, label=label)
    axes[0].set(xlabel=r"$\cos\theta$", ylabel="P(two arms | muon)", yscale="log")
    axes[1].set(xlabel="Kinetic energy [GeV]", xscale="log", yscale="log")
    axes[0].set_ylim(1e-6, 1e-2)
    axes[1].set_ylim(1e-6, 1e-2)
    finish(figure, axes, output, "response.pdf")


def draw_pair_origin(runs, output):
    figure, axes = plt.subplots(1, 2, figsize=(7.2, 2.7), layout="constrained")
    for key, (label, color, marker) in SOURCES.items():
        pairs = runs[key]["pairs"]
        for axis, column, edges, scale in [(axes[0], "cos_zenith", COSINE_EDGES, 1),
                                           (axes[1], "muon_kinetic_MeV", ENERGY_EDGES_GEV, 1000)]:
            value, error = density(pairs[column] / scale, edges)
            value = np.where(value > 0, value, np.nan)
            centers = COSINE_CENTERS if column == "cos_zenith" else ENERGY_CENTERS
            axis.errorbar(centers, value, yerr=error, fmt=marker + "-", ms=3, lw=1,
                          color=color, label=f"{label} (N = {len(pairs):,})")
    axes[0].set(xlabel=r"Muon $\cos\theta$", ylabel="Probability density", yscale="log")
    axes[1].set(xlabel="Muon kinetic energy [GeV]", xscale="log", ylabel="Probability density")
    finish(figure, axes, output, "pair_origin.pdf")


def draw_pairs(runs, output):
    figure, axes = plt.subplots(1, 3, figsize=(7.2, 2.5), layout="constrained")
    for axis, (column, edges, label) in zip(axes, PAIR_PANELS):
        centers = (edges[1:] + edges[:-1]) / 2
        for key, (name, color, marker) in SOURCES.items():
            value, error = density(runs[key]["pairs"][column], edges)
            axis.errorbar(centers, value, yerr=error, fmt=marker + "-", ms=2.5, lw=1,
                          color=color, label=name)
        axis.set(xlabel=label)
    axes[0].set_ylabel("Probability density")
    finish(figure, axes, output, "pairs.pdf", legend_axis=1)


def grouped(number):
    return f"{number:,}".replace(",", "{,}")


def write_table(runs, output):
    lines = [r"\begin{tabular}{lrrrrr}", r"\toprule",
             r"Source & Muons & $P(1)$ & $P(2)$ & Pairs & Symmetric \\", r"\midrule"]
    for key, (label, _, _) in SOURCES.items():
        counts, pairs = runs[key]["multiplicity"], runs[key]["pairs"]
        lines.append(f"{label} & {grouped(counts.sum())} & {counts[1] / counts.sum():.1e} & "
                     f"{counts[2] / counts.sum():.1e} & {grouped(len(pairs))} & "
                     f"{100 * (pairs['energy_asymmetry'] < 0.5).mean():.1f}\\% \\\\")
    lines += [r"\bottomrule", r"\end{tabular}"]
    (output / "multiplicity.tex").write_text("\n".join(lines)+ "\n")

    rows = [r"\begin{tabular}{l" + "rr" * len(WINDOWS) + "}", r"\toprule",
            "Source & " + " & ".join(rf"\multicolumn{{2}}{{c}}{{{name}}}" for name in WINDOWS) + r" \\",
            " & " + " & ".join("All & Sym." for _ in WINDOWS) + r" \\", r"\midrule"]
    for key, (label, _, _) in SOURCES.items():
        cells = []
        for bounds in WINDOWS.values():
            selected = select_window(runs[key]["pairs"], bounds)
            cells += [str(len(selected)), str(int((selected["energy_asymmetry"] < 0.5).sum()))]
        rows.append(f"{label} & " + " & ".join(cells) + r" \\")
    rows += [r"\bottomrule", r"\end{tabular}"]
    (output / "windows.tex").write_text("\n".join(rows) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", type=Path, default=Path("data/comparison"))
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    arguments.output.mkdir(parents=True, exist_ok=True)
    plt.rcParams.update({"font.size": 8.5, "axes.spines.top": False, "axes.spines.right": False,
                         "legend.handlelength": 1.6})
    runs = {key: read_run(arguments.data / f"{key}.root") for key in SOURCES}
    for draw in (draw_source, draw_response, draw_pair_origin, draw_pairs, write_table):
        draw(runs, arguments.output)


if __name__ == "__main__":
    main()
