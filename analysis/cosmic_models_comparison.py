import marimo

__generated_with = "0.24.0"
app = marimo.App(width="medium")


@app.cell
def _():
    import marimo as mo
    import matplotlib.pyplot as plt
    import numpy as np
    import pandas as pd
    from pathlib import Path
    from model_runs import COSINE_EDGES as cosine_edges
    from model_runs import ENERGY_EDGES_GEV as energy_edges_gev
    from model_runs import WINDOWS, read_run, select_window

    project_directory = Path(__file__).resolve().parents[1]
    return (Path, WINDOWS, cosine_edges, energy_edges_gev, mo, np, pd, plt,
            project_directory, read_run, select_window)


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    # Two cosmic-muon sources in the same detector

    The simulation can generate cosmic muons with two implementations:

    | | George's `B4PrimaryGeneratorAction` | Guan model (ours) |
    |---|---|---|
    | Zenith angle | accept–reject on cos²θ, up to 70° | projected Guan flux, up to 70° |
    | Energy | Tripathi–Shukla form, 0.1–20 GeV, sampled linearly | Guan flux, 0.1–100 GeV, sampled from tabulated inverse CDF |
    | Charge | μ⁺ only | μ⁺ and μ⁻ equally |
    | Source plane | 20 × 30 cm² at y = 45 cm | 200 × 200 cm² at y = 60 cm |
    | Absolute rate | not normalized | integrated flux through the plane |

    Both fire muons downward from a horizontal plane through the same sixteen-arm
    detector with identical physics, thresholds and readout. The two sources differ
    in more than one respect, so a third run separates the effects: the **Guan model
    on George's plane**. Comparing it with George's run isolates the
    spectrum and angular distribution; comparing it with our production plane
    isolates the plane.
    """)
    return


@app.cell(hide_code=True)
def _(mo, project_directory):
    _folder = project_directory / "data/comparison"
    george_file = mo.ui.text(value=str(_folder / "b4.root"), label="George (B4)", full_width=True)
    same_plane_file = mo.ui.text(value=str(_folder / "guan_b4_plane.root"),
                                 label="Guan, George's plane", full_width=True)
    our_plane_file = mo.ui.text(value=str(_folder / "guan_production_plane.root"),
                                label="Guan, our plane", full_width=True)
    angular_bin_width = mo.ui.dropdown(options={"5°": 5, "10°": 10, "15°": 15}, value="10°",
                                       label="Angular bins")
    mo.vstack([george_file, same_plane_file, our_plane_file, angular_bin_width])
    return angular_bin_width, george_file, our_plane_file, same_plane_file


@app.cell
def _(Path, george_file, mo, our_plane_file, read_run, same_plane_file):
    _selected = {"George (B4)": george_file.value,
                 "Guan, George's plane": same_plane_file.value,
                 "Guan, our plane": our_plane_file.value}
    _missing = [path for path in _selected.values() if not Path(path).is_file()]
    mo.stop(bool(_missing), mo.md("ROOT file not found: " + ", ".join(f"`{p}`" for p in _missing)))
    runs = {name: read_run(path) for name, path in _selected.items()}
    colors = {"George (B4)": "tab:red", "Guan, George's plane": "tab:blue",
              "Guan, our plane": "tab:green"}
    return colors, runs


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## What each source throws at the detector

    The histograms use every incident muon, including those that miss the detector,
    and are normalized to unit area. The Guan run on George's plane is shown
    only once because the plane does not change the spectrum.
    """)
    return


@app.cell
def _(colors, cosine_edges, energy_edges_gev, mo, np, plt, runs):
    _figure, _axes = plt.subplots(1, 2, figsize=(11, 4), layout="constrained")
    for _name in ["George (B4)", "Guan, our plane"]:
        _incident = runs[_name]["incident"]
        _by_cosine = _incident.sum(axis=1)
        _by_energy = _incident.sum(axis=0)
        _axes[0].stairs(_by_cosine / _by_cosine.sum() / np.diff(cosine_edges), cosine_edges,
                        label=_name, color=colors[_name])
        _axes[1].stairs(_by_energy / _by_energy.sum() / np.diff(np.log10(energy_edges_gev)),
                        energy_edges_gev, label=_name, color=colors[_name])
    _cosine_centers = (cosine_edges[1:] + cosine_edges[:-1]) / 2
    _pure = _cosine_centers**2 / np.sum(_cosine_centers**2 * np.diff(cosine_edges))
    _axes[0].plot(_cosine_centers, _pure, "k--", label="cos²θ")
    _axes[0].set(xlabel="cos θ (zenith)", ylabel="Probability density")
    _axes[1].set(xlabel="Kinetic energy [GeV]", ylabel="Probability per decade",
                 xscale="log", yscale="log")
    _axes[0].legend()
    mo.as_html(_figure)
    return


@app.cell
def _(mo, pd, runs):
    _rows = []
    for _name in ["George (B4)", "Guan, our plane"]:
        _by_energy = runs[_name]["incident"].sum(axis=0)
        _rows.append({"Source": _name, "Incident muons": int(_by_energy.sum()),
                      "Below 1 GeV": _by_energy[:10].sum() / _by_energy.sum(),
                      "Above 10 GeV": _by_energy[20:].sum() / _by_energy.sum()})
    mo.ui.table(pd.DataFrame(_rows), selection=None, pagination=False,
                format_mapping={"Below 1 GeV": "{:.1%}", "Above 10 GeV": "{:.1%}"})
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    George's sampler is described as cos²θ, but the sampled zenith distribution is
    not exactly that: the dashed curve shows the difference. The accept–reject
    step compares cos²θ with a uniform value between cos²(70°) and 1, so the density
    it produces is proportional to cos²θ − cos²(70°). A rejected energy also redraws the
    zenith angle, which changes the shape further. Our Guan sampler draws the zenith
    angle from the projected Guan flux and then the energy conditional on it.
    """)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## How often a muon becomes a two-arm event

    Multiplicity is the number of hit arms, defined as in the main muon analysis:
    all three active layers must exceed their thresholds. Probabilities are per
    incident muon, so they include the geometric effect of each plane's size.
    """)
    return


@app.cell
def _(mo, pd, runs):
    _rows = []
    for _name, _run in runs.items():
        _counts = _run["multiplicity"]
        _total = _counts.sum()
        _rows.append({"Source": _name, "Incident muons": int(_total),
                      "0 arms": int(_counts[0]), "1 arm": int(_counts[1]),
                      "2 arms": int(_counts[2]), "3+ arms": int(_counts[3:].sum()),
                      "P(1 arm)": _counts[1] / _total, "P(2 arms)": _counts[2] / _total,
                      "Pairs / single": _counts[2] / max(_counts[1], 1)})
    multiplicity_table = pd.DataFrame(_rows)
    mo.ui.table(multiplicity_table, selection=None, pagination=False,
                format_mapping={"P(1 arm)": "{:.3e}", "P(2 arms)": "{:.3e}", "Pairs / single": "{:.4f}"})
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Which muons produce pairs

    For each bin in energy or zenith angle, the curve is the fraction of incident
    muons that give exactly two hit arms, with binomial uncertainties. Bins with
    no incident muons are omitted. Differences between the two Guan runs are
    purely geometric; the difference between George's run and the Guan run on the
    same plane shows how the muon distribution maps to detector response.
    """)
    return


@app.cell
def _(colors, cosine_edges, energy_edges_gev, mo, np, plt, runs):
    def _fraction(coincident, incident):
        with np.errstate(divide="ignore", invalid="ignore"):
            fraction = coincident / incident
            error = np.sqrt(fraction * (1 - fraction) / incident)
        return fraction, error

    _figure, _axes = plt.subplots(1, 2, figsize=(11, 4), layout="constrained")
    for _name, _run in runs.items():
        for _axis, _axis_index, _edges in [(_axes[0], 1, cosine_edges), (_axes[1], 0, energy_edges_gev)]:
            _fraction_values, _error = _fraction(_run["coincident"].sum(axis=_axis_index),
                                                 _run["incident"].sum(axis=_axis_index))
            _centers = np.sqrt(_edges[1:] * _edges[:-1]) if _axis is _axes[1] else (_edges[1:] + _edges[:-1]) / 2
            _axis.errorbar(_centers, _fraction_values, yerr=_error, fmt="o-", ms=3,
                           color=colors[_name], label=_name)
    _axes[0].set(xlabel="cos θ (zenith)", ylabel="P(2 arms | incident muon)", yscale="log")
    _axes[1].set(xlabel="Kinetic energy [GeV]", xscale="log", yscale="log")
    _axes[0].legend()
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Observables of the two-arm events

    The pair observables follow the main analysis: summed scintillator energy,
    energy asymmetry, and the opening angle reconstructed from the assumed target
    at the origin. Each histogram is normalized to unit area so that shapes can be
    compared regardless of how many pairs each source produced; the error bars are
    Poisson uncertainties on the bin contents.
    """)
    return


@app.cell
def _(angular_bin_width, colors, mo, np, plt, runs):
    def _density(values, edges):
        counts = np.histogram(values, bins=edges)[0].astype(float)
        norm = max(counts.sum(), 1) * np.diff(edges)
        return counts / norm, np.sqrt(counts) / norm

    _panels = [("energy_sum_MeV", np.arange(0, 81, 2), "Summed scintillator energy [MeV]"),
               ("energy_asymmetry", np.linspace(0, 1, 21), "Absolute energy asymmetry"),
               ("opening_angle_deg", np.arange(0, 181, angular_bin_width.value), "Opening angle [°]")]
    _figure, _axes = plt.subplots(1, 3, figsize=(14, 4), layout="constrained")
    for _axis, (_column, _edges, _label) in zip(_axes, _panels):
        for _name, _run in runs.items():
            _values, _error = _density(_run["pairs"][_column], _edges)
            _centers = (_edges[1:] + _edges[:-1]) / 2
            _axis.errorbar(_centers, _values, yerr=_error, fmt="o-", ms=3, color=colors[_name],
                           label=f"{_name} (N = {len(_run['pairs']):,})")
        _axis.set(xlabel=_label, ylabel="Probability density")
    _axes[0].legend(fontsize="small")
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(cosine_edges, mo, runs):
    _george, _same, _ours = (runs[name] for name in
                             ["George (B4)", "Guan, George's plane", "Guan, our plane"])
    def _p2(run):
        return run["multiplicity"][2] / run["multiplicity"].sum()
    def _mean_cosine(run):
        _by_cosine = run["incident"].sum(axis=1)
        _centers = (cosine_edges[1:] + cosine_edges[:-1]) / 2
        return (_by_cosine * _centers).sum() / _by_cosine.sum()
    def _median(run, column):
        return run["pairs"][column].median()
    def _symmetric(run):
        return (run["pairs"]["energy_asymmetry"] < 0.5).mean()
    mo.md(rf"""
    ## Reading the comparison

    - **Plane size sets the yield per muon.** The probability of a two-arm event
      is {_p2(_ours):.2e} for our plane and {_p2(_same):.2e} for George's plane with the same
      Guan spectrum, a factor {_p2(_same) / _p2(_ours):.0f}. The small plane sits closer to the
      detector and mostly selects near-vertical muons: the median zenith cosine of
      pair-producing muons is {_median(_same, 'cos_zenith'):.3f} there and
      {_median(_ours, 'cos_zenith'):.3f} on our plane.
    - **George's spectrum gives fewer pairs per muon than Guan on the same plane**
      ({_p2(_george):.2e} against {_p2(_same):.2e}, ratio {_p2(_george) / _p2(_same):.2f}),
      which is consistent with its more inclined muons (mean zenith cosine
      {_mean_cosine(_george):.3f} against {_mean_cosine(_same):.3f}), since the small plane
      favours near-vertical tracks. Its energy cut at 20 GeV also removes the high-energy tail.
    - **The pair observables barely change.** Median summed energy is
      {_median(_george, 'energy_sum_MeV'):.1f}, {_median(_same, 'energy_sum_MeV'):.1f} and
      {_median(_ours, 'energy_sum_MeV'):.1f} MeV, median opening angle
      {_median(_george, 'opening_angle_deg'):.1f}°, {_median(_same, 'opening_angle_deg'):.1f}° and
      {_median(_ours, 'opening_angle_deg'):.1f}°, and symmetric fractions
      {_symmetric(_george):.1%}, {_symmetric(_same):.1%} and {_symmetric(_ours):.1%}.
      Pair shapes are set mainly by the detector, not by the source model.

    The window counts are small, especially on our plane with only
    {len(_ours['pairs'])} pairs, so differences between windows are not significant
    at this statistics. These are per-muon comparisons: George's source has no
    absolute normalization, so no event rate follows from it.
    """)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Signal and control windows

    Counts of two-arm events whose summed energy falls in each window (lower edge
    inclusive, upper edge exclusive), split by asymmetry at 0.5. These are raw
    counts for each run's own number of incident muons, so the sources are not
    comparable here without normalization; the *fractions* of pairs are. The two
    signal windows overlap, so their counts must not be added.
    """)
    return


@app.cell
def _(WINDOWS, mo, pd, runs, select_window):
    _rows = []
    for _name, _run in runs.items():
        _pairs = _run["pairs"]
        for _window, _bounds in WINDOWS.items():
            _selected = select_window(_pairs, _bounds)
            _symmetric = int(_selected["energy_asymmetry"].lt(0.5).sum())
            _rows.append({"Source": _name, "Window": _window, "Pairs": len(_selected),
                          "Symmetric": _symmetric,
                          "Share of all pairs": len(_selected) / max(len(_pairs), 1),
                          "Symmetric share of all pairs": _symmetric / max(len(_pairs), 1)})
    window_table = pd.DataFrame(_rows)
    mo.ui.table(window_table, selection=None, pagination=False,
                format_mapping={"Share of all pairs": "{:.2%}", "Symmetric share of all pairs": "{:.2%}"})
    return


if __name__ == "__main__":
    app.run()
