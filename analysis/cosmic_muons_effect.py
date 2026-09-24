import marimo

__generated_with = "0.24.0"
app = marimo.App(width="medium")


@app.cell
def _():
    import marimo as mo
    import matplotlib.pyplot as plt
    import numpy as np
    import pandas as pd
    import tomllib
    import uproot
    import xml.etree.ElementTree as ET
    from pathlib import Path

    project_directory = Path(__file__).resolve().parents[1]
    with (project_directory / "config/cosmic_muons.toml").open("rb") as stream:
        configured_file = tomllib.load(stream)["runtime"]["output_file"]
    return ET, Path, configured_file, mo, np, pd, plt, project_directory, tomllib, uproot


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    # Cosmic-muon coincidences

    Can cosmic muons create structure in our detector's reconstructed pair angles?
    This notebook applies the selections in the supplied **cosmic-muon study**
    (`../cosmic-muons.pdf`, Sections III–V) to our sixteen-arm detector.
    Figures 2–7 provide the plot definitions; their five- and six-arm geometries
    are not our geometry, so their peak positions and efficiencies are not predictions here.

    Each simulated event starts with one incident muon. A two-arm coincidence
    is reconstructed as two directions from a presumed target vertex, even when
    the deposits came from a through-going muon or its secondaries. This geometric
    interpretation, rather than a generated electron–positron pair, defines the angle.
    """)
    return


@app.cell(hide_code=True)
def _(configured_file, mo):
    input_file = mo.ui.text(value=configured_file, label="ROOT file", full_width=True).form()
    angular_bin_width = mo.ui.dropdown(
        options={"2°": 2, "5°": 5, "10°": 10}, value="5°", label="Angular bins"
    )
    mo.vstack([input_file, angular_bin_width])
    return angular_bin_width, input_file


@app.cell
def _(ET, Path, configured_file, input_file, mo, pd, project_directory, tomllib, uproot):
    data_path = Path(input_file.value or configured_file).expanduser()
    if not data_path.is_absolute():
        data_path = project_directory / data_path
    mo.stop(not data_path.is_file(), mo.md(f"ROOT file not found: `{data_path}`"))
    config_path = Path(str(data_path) + ".toml")
    geometry_path = Path(str(data_path) + ".gdml")
    mo.stop(
        not config_path.is_file() or not geometry_path.is_file(),
        mo.md("Keep the run's `.root.toml` and `.root.gdml` sidecars beside the ROOT file."),
    )
    with config_path.open("rb") as _stream:
        run_config = tomllib.load(_stream)
    _geometry = ET.parse(geometry_path)
    arm_count = sum(
        placement.find("volumeref").get("ref") == "Scintillator"
        for placement in _geometry.findall(".//physvol")
    )
    mo.stop(arm_count < 2, mo.md("The saved geometry has fewer than two scintillator arms."))
    with uproot.open(data_path, handler=uproot.source.file.MemmapSource) as _root_file:
        mo.stop("cosmic_events" not in _root_file, mo.md("This file has no `cosmic_events` tree."))
        events = pd.DataFrame(_root_file["cosmic_events"].arrays(
            ["event_id", "n_hit", "arm_1", "arm_2", "energy_sum_MeV",
             "energy_asymmetry", "opening_angle_deg", "opening_angle_z25mm_deg"],
            library="np",
        ))
    generated_count = len(events)
    mo.stop(generated_count == 0, mo.md("The cosmic event tree is empty. Select a completed run."))
    mo.stop(events["event_id"].duplicated().any(), mo.md("Event IDs are duplicated; select one run."))
    return arm_count, data_path, events, generated_count, run_config


@app.cell(hide_code=True)
def _(arm_count, data_path, generated_count, mo, run_config):
    _thresholds = run_config["readout"]["thresholds"]
    _source = run_config["source"]["cosmic_muons"]
    mo.md(rf"""
    ## Run and trigger definition

    **{data_path.name}** contains **{generated_count:,} incident muons**, including
    misses, against {run_config['runtime']['events']:,} requested events.
    The saved geometry contains **{arm_count} arms**. Only the eight event branches
    needed below are loaded; the much larger step tree is not read.

    The saved source uses a {_source['plane']['side_cm']:g} cm square at
    $y={_source['plane']['height_cm']:g}$ cm, zenith angles up to
    {_source['angular']['zenith_max_deg']:g}°, and kinetic energies from
    {_source['energy']['min_gev']:g} to {_source['energy']['max_gev']:g} GeV.
    Configuration and geometry come from this run's sidecars, not today's detector settings.

    An arm is triggered only if **all three** deposits exceed the saved thresholds:

    | Layer | Strict threshold |
    |---|---:|
    | MWPC gas | {_thresholds['mwpc_mev'] * 1000:g} keV |
    | Thin $\Delta E$ scintillator | {_thresholds['delta_e_mev'] * 1000:g} keV |
    | Long $E$ scintillator | {_thresholds['scintillator_mev'] * 1000:g} keV |

    We use the recorded `n_hit`; changing thresholds requires recomputing hits from
    `cosmic_arms`, not changing a plotting cut. The paper's Table II uses 50 keV for
    $\Delta E$, whereas its prose says 100 keV for plastic scintillators; the saved
    values above specify this run unambiguously. These are ideal deposit thresholds,
    with no electronics resolution or optical coincidence requirement.
    """)
    return


@app.cell
def _(events, np):
    two_arm_events = events.loc[events["n_hit"].eq(2)].copy()
    _observables = ["energy_sum_MeV", "energy_asymmetry", "opening_angle_deg",
                    "opening_angle_z25mm_deg"]
    _finite = np.isfinite(two_arm_events[_observables]).all(axis=1)
    invalid_pair_count = int((~_finite).sum())
    pairs = two_arm_events.loc[_finite].copy()
    return invalid_pair_count, pairs, two_arm_events


@app.cell(hide_code=True)
def _(generated_count, invalid_pair_count, mo, two_arm_events):
    mo.md(rf"""
    ## Multiplicity and coincidence efficiency

    **{len(two_arm_events):,} / {generated_count:,}** incident muons triggered exactly
    two arms: **{len(two_arm_events) / generated_count:.4%}**.
    There are **{invalid_pair_count:,}** two-arm rows with non-finite observables;
    these are excluded from the following pair plots, but remain in the incident
    and trigger counts. Empty selections produce empty histograms rather than fits.

    A miss is a real trial. The efficiency denominator is the number of rows in
    `cosmic_events`, never the number of detected particles or the requested run size.
    """)
    return


@app.cell
def _(arm_count, events, mo, np, plt):
    _figure, _axis = plt.subplots(figsize=(8, 3.5), layout="constrained")
    _axis.hist(events["n_hit"], bins=np.arange(arm_count + 2) - 0.5, color="steelblue")
    _axis.set(xlabel="Triggered arms", ylabel="Incident muons", xticks=range(arm_count + 1))
    mo.ui.matplotlib(_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Deposited energy and asymmetry — Figure 2

    For two triggered arms, let $E_i=E_{i,\mathrm{long}}+E_{i,\Delta E}$.
    Gas energy establishes the trigger but is not included in this sum:

    $$E_{\rm sum}=E_1+E_2,\qquad A=\frac{|E_1-E_2|}{E_1+E_2}.$$

    The stored asymmetry is **nonnegative**, equivalent to ordering $E_1\ge E_2$.
    We plot $0\le A\le1$, without inventing a signed distribution to match the
    paper's display. Symmetric events have $A<0.5$; asymmetric events have
    $A\ge0.5$, assigning equality explicitly. Energy windows are half-open
    $[E_{\min},E_{\max})$ so adjacent signal/control samples do not share a boundary.
    The beryllium and helium studies are separate selections and may overlap.

    All unscaled plots show counts with common bins, not independently normalized
    shapes. Energy spectra use 1 MeV bins spanning the sample; angular bins are
    controlled above. No smoothing or background subtraction is applied.
    """)
    return


@app.cell
def _(angular_bin_width, np, pairs):
    angle_edges = np.arange(0, 181, angular_bin_width.value)
    energy_max = max(40, int(np.ceil(pairs["energy_sum_MeV"].max()))) if len(pairs) else 40
    energy_edges = np.arange(energy_max + 1)
    windows = {
        "Be signal": (16, 20, "tab:blue"),
        "Be background": (12, 16, "tab:purple"),
        "He signal": (18, 22, "tab:orange"),
        "He background": (14, 18, "tab:green"),
    }
    return angle_edges, energy_edges, windows


@app.cell
def _(np):
    def select_window(data, bounds, symmetric=None):
        lower, upper = bounds[:2]
        selected = data["energy_sum_MeV"].ge(lower) & data["energy_sum_MeV"].lt(upper)
        if symmetric is not None:
            selected &= data["energy_asymmetry"].lt(0.5) if symmetric else data["energy_asymmetry"].ge(0.5)
        return data.loc[selected]

    def draw_counts(axis, values, edges, label, color, filled=False, scale=1.0, linestyle="-"):
        counts, _ = np.histogram(values, bins=edges)
        axis.stairs(counts * scale, edges, label=label, color=color,
                    fill=filled, alpha=0.3 if filled else 1, linestyle=linestyle)

    def mark_energy_windows(axis, names, windows, horizontal=False):
        for name in names:
            lower, upper, color = windows[name]
            draw_band = axis.axhspan if horizontal else axis.axvspan
            draw_band(lower, upper, alpha=0.13, color=color,
                      label=f"{name}: {lower}–{upper} MeV")

    return draw_counts, mark_energy_windows, select_window


@app.cell
def _(draw_counts, energy_edges, mark_energy_windows, mo, np, pairs, plt, windows):
    _figure, _axes = plt.subplots(2, 1, figsize=(8, 6), layout="constrained")
    draw_counts(_axes[0], pairs["energy_sum_MeV"], energy_edges, "Two arms", "black")
    mark_energy_windows(_axes[0], ["Be signal", "He signal"], windows)
    _axes[0].set(xlabel="Summed scintillator energy [MeV]", ylabel="Events / 1 MeV")
    _axes[0].legend()
    draw_counts(_axes[1], pairs["energy_asymmetry"], np.linspace(0, 1, 51), "Two arms", "black")
    _axes[1].axvline(0.5, color="tab:red", linestyle="--", label="Asymmetry cut")
    _axes[1].set(xlabel="Absolute energy asymmetry", ylabel="Events / 0.02", xlim=(0, 1))
    _axes[1].legend()
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Energy–angle correlation — Figures 3 and 6

    In each triggered MWPC sector the simulation records the energy-weighted
    step-midpoint centroid, $\mathbf r_i=\sum_j\Delta E_j\mathbf r_j/\sum_j\Delta E_j$.
    From the assumed target position $\mathbf v$,

    $$\theta=\arccos\left(
    \frac{(\mathbf r_1-\mathbf v)\cdot(\mathbf r_2-\mathbf v)}
    {|\mathbf r_1-\mathbf v|\,|\mathbf r_2-\mathbf v|}\right).$$

    Both maps below use $\mathbf v=0$ and the same two-arm sample, bins, and color
    scale. They differ only in the highlighted study windows. A correlation can
    arise because track path length affects deposited energy while the same track
    determines which sectors and centroid positions are hit.
    """)
    return


@app.cell
def _(angle_edges, energy_edges, mark_energy_windows, mo, np, pairs, plt, windows):
    _counts, _, _ = np.histogram2d(pairs["opening_angle_deg"], pairs["energy_sum_MeV"],
                                  bins=(angle_edges, energy_edges))
    _figure, _axes = plt.subplots(1, 2, figsize=(11, 4), sharey=True, layout="constrained")
    for _axis, _names in zip(_axes, [("Be signal", "He signal"), ("Be signal", "Be background")]):
        _mesh = _axis.pcolormesh(angle_edges, energy_edges, _counts.T, cmap="viridis",
                                vmin=0, vmax=max(1, _counts.max()), shading="auto")
        mark_energy_windows(_axis, _names, windows, horizontal=True)
        _axis.set(xlabel="Opening angle [°]", xlim=(0, 180))
        _axis.legend(fontsize="small", loc="lower left", bbox_to_anchor=(0, 1))
    _axes[0].set_ylabel("Summed scintillator energy [MeV]")
    _figure.colorbar(_mesh, ax=_axes, label="Events / bin")
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(arm_count, mo):
    mo.md(rf"""
    ## Arm-pair geometry

    Our {arm_count} equally spaced sectors have pitch $360°/{arm_count}$.
    For one-based arm indices $i,j$, their smaller azimuthal separation is

    $$\Delta\phi=\frac{{360°}}{{{arm_count}}}
    \min(|i-j|,{arm_count}-|i-j|).$$

    This is a sector-centre separation, not the reconstructed three-dimensional
    opening angle. The table and map test the geometric explanation discussed
    around Figures 3 and 6 without imposing the paper's three six-arm categories.
    """)
    return


@app.cell
def _(angle_edges, arm_count, mo, np, pairs, pd, plt):
    _arm_distance = (pairs["arm_1"] - pairs["arm_2"]).abs()
    _separation = np.minimum(_arm_distance, arm_count - _arm_distance)
    _sectors = np.arange(1, arm_count // 2 + 1)
    _counts = _separation.value_counts().reindex(_sectors, fill_value=0)
    topology_table = pd.DataFrame({
        "Separation [°]": _sectors * 360 / arm_count,
        "Two-arm events": _counts.to_numpy(),
        "Fraction": _counts.to_numpy() / len(pairs) if len(pairs) else np.zeros(len(_sectors)),
    })
    _figure, _axis = plt.subplots(figsize=(8, 4), layout="constrained")
    _histogram = _axis.hist2d(pairs["opening_angle_deg"], _separation,
                            bins=(angle_edges, np.arange(arm_count // 2 + 2) - 0.5),
                            cmap="viridis")
    _axis.set(xlabel="Reconstructed opening angle [°]", ylabel="Arm-centre separation [°]",
              yticks=_sectors, yticklabels=[f"{value * 360 / arm_count:g}" for value in _sectors])
    _figure.colorbar(_histogram[3], ax=_axis, label="Events / bin")
    mo.vstack([mo.ui.table(topology_table, selection=None, pagination=False), mo.as_html(_figure)])
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Beryllium windows — Figure 4

    Compare symmetric and asymmetric two-arm events in the 16–20 MeV signal
    window, then repeat in the 12–16 MeV control window. Both panels have identical
    angular bins and a shared count scale. A concentration confined to a selected
    energy band should be checked against the full energy–angle map before it is
    interpreted as a particle signal.
    """)
    return


@app.cell
def _(angle_edges, angular_bin_width, draw_counts, mo, pairs, plt, select_window, windows):
    _figure, _axes = plt.subplots(2, 1, figsize=(8, 6), sharex=True, sharey=True, layout="constrained")
    for _axis, _name in zip(_axes, ["Be signal", "Be background"]):
        _lower, _upper, _color = windows[_name]
        for _symmetric, _label, _line_color in [(True, "A < 0.5", _color), (False, "A ≥ 0.5", "tab:red")]:
            _sample = select_window(pairs, windows[_name], symmetric=_symmetric)
            draw_counts(_axis, _sample["opening_angle_deg"], angle_edges,
                        f"{_label}: {len(_sample):,}", _line_color, filled=_symmetric,
                        linestyle="-" if _symmetric else "--")
        _axis.set(title=f"{_name}: {_lower}–{_upper} MeV", ylabel=f"Events / {angular_bin_width.value}°", xlim=(0, 180))
        _axis.legend()
    _axes[-1].set_xlabel("Opening angle [°]")
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Helium windows and assumed vertex — Figure 5

    Select symmetric events in 18–22 MeV and 14–18 MeV. For each sample compare
    the recorded angles from $\mathbf v=(0,0,0)$ and $\mathbf v=(0,0,2.5\ \mathrm{cm})$.
    The same events enter both curves; only the assumed vertex changes.
    This is a reconstruction hypothesis, not a new simulation with the physical
    target moved. The deposited energies and triggered arms remain unchanged.
    """)
    return


@app.cell
def _(angle_edges, angular_bin_width, draw_counts, mo, pairs, plt, select_window, windows):
    _figure, _axes = plt.subplots(2, 1, figsize=(8, 6), sharex=True, sharey=True, layout="constrained")
    for _axis, _name in zip(_axes, ["He signal", "He background"]):
        _sample = select_window(pairs, windows[_name], symmetric=True)
        _lower, _upper, _color = windows[_name]
        draw_counts(_axis, _sample["opening_angle_deg"], angle_edges, "Vertex z = 0", _color, filled=True)
        draw_counts(_axis, _sample["opening_angle_z25mm_deg"], angle_edges, "Vertex z = 25 mm", "black", linestyle="--")
        _axis.set(title=f"{_name}: {_lower}–{_upper} MeV, A < 0.5, N = {len(_sample):,}",
                  ylabel=f"Events / {angular_bin_width.value}°", xlim=(0, 180))
        _axis.legend()
    _axes[-1].set_xlabel("Opening angle [°]")
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Signal and control overlaid — Figure 7

    Overlay symmetric signal events, asymmetric signal events, and symmetric
    control events using the origin-based angle. The windows have equal width
    (4 MeV), and no additional area normalization is applied. Unlike the original
    Figure 7, these curves all describe our sixteen-arm apparatus.
    """)
    return


@app.cell
def _(angle_edges, angular_bin_width, draw_counts, mo, pairs, plt, select_window, windows):
    _figure, _axis = plt.subplots(figsize=(8, 4), layout="constrained")
    for _name, _symmetric, _color, _filled, _style in [
        ("Be signal", True, "tab:blue", True, "-"),
        ("Be signal", False, "tab:orange", False, "-"),
        ("Be background", True, "tab:purple", False, "--"),
    ]:
        _sample = select_window(pairs, windows[_name], symmetric=_symmetric)
        _label = f"{_name}, {'A < 0.5' if _symmetric else 'A ≥ 0.5'}"
        draw_counts(_axis, _sample["opening_angle_deg"], angle_edges, _label, _color,
                    filled=_filled, linestyle=_style)
    _axis.set(xlabel="Opening angle [°]", ylabel=f"Events / {angular_bin_width.value}°", xlim=(0, 180))
    _axis.legend()
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Selection yields and an exposure estimate

    For $N$ incident muons and $n$ selected events, the estimated efficiency is
    $\hat\epsilon=n/N$. The table gives a 95% Wilson binomial interval, which
    remains meaningful when a small sample yields zero coincidences:

    $$c=\frac{\hat\epsilon+z^2/(2N)}{1+z^2/N},\qquad
    h=\frac{z\sqrt{\hat\epsilon(1-\hat\epsilon)/N+z^2/(4N^2)}}{1+z^2/N},
    \quad z=1.96.$$

    The interval is $[c-h,c+h]$, clipped to $[0,1]$. It describes Monte Carlo
    counting uncertainty, not uncertainty in the cosmic spectrum or detector model.

    The paper assumes $R_\mu=700$ incident muons/s through its source plane and
    $T=300$ hours. **If we adopt that same external rate**, the expected selected
    count is $R_\mu T\hat\epsilon$ and each simulated event has weight
    $w=R_\mu T/N$. This does not turn the generated sample into 300 hours of
    statistical precision. It is not a measured rate for our setup. Rescaling
    to a different plane or incident spectrum requires a new rate calculation.
    """)
    return


@app.cell
def _(generated_count, np, pairs, pd, select_window, two_arm_events, windows):
    assumed_incident_count = 700 * 300 * 3600
    exposure_weight = assumed_incident_count / generated_count
    _selections = [("Exactly two arms", len(two_arm_events)), ("Finite two-arm observables", len(pairs))]
    for _name, _bounds in windows.items():
        for _symmetric in [True, False]:
            _sample = select_window(pairs, _bounds, symmetric=_symmetric)
            _selections.append((f"{_name}, {'A < 0.5' if _symmetric else 'A ≥ 0.5'}", len(_sample)))
    _rows = []
    for _label, _count in _selections:
        _efficiency = _count / generated_count
        _denominator = 1 + 1.96**2 / generated_count
        _centre = (_efficiency + 1.96**2 / (2 * generated_count)) / _denominator
        _half_width = 1.96 * np.sqrt(
            _efficiency * (1 - _efficiency) / generated_count + 1.96**2 / (4 * generated_count**2)
        ) / _denominator
        _rows.append({
            "Selection": _label, "Events": _count, "Efficiency": _efficiency,
            "95% lower": max(0, _centre - _half_width), "95% upper": min(1, _centre + _half_width),
            "Expected at 700 Hz, 300 h": _count * exposure_weight,
        })
    yield_table = pd.DataFrame(_rows)
    return exposure_weight, yield_table


@app.cell(hide_code=True)
def _(mo, yield_table):
    mo.ui.table(yield_table, selection=None, pagination=False)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Cosmic component of Figures 9 and 10

    The following spectra apply the exposure weight above to the two-arm sample.
    Energy includes all two-arm asymmetries; angular panels require $A<0.5$.
    Error bars show the per-bin Monte Carlo uncertainty $w\sqrt{n}$ (the usual
    Poisson approximation), not the counting error of a real 300-hour exposure.

    Figure 8 needs a generated and reconstructed IPC sample plus the experimental
    acceptance data. The complete Figures 9–10 additionally need the paper's
    Zhang–Miller IPC model and reaction-rate normalization. Our current simplified
    IPC source does not supply those inputs. We therefore show **cosmics only**;
    no IPC curve, cosmic-to-IPC ratio, or claimed X17 significance is inferred.
    """)
    return


@app.cell
def _(angle_edges, angular_bin_width, energy_edges, exposure_weight, mo, np, pairs, plt, select_window, windows):
    _figure, _axes = plt.subplots(3, 1, figsize=(8, 9), layout="constrained")
    _samples = [(pairs["energy_sum_MeV"], energy_edges, "Two-arm cosmic energy", "Summed scintillator energy [MeV]", "1 MeV")]
    for _name in ["Be signal", "Be background"]:
        _sample = select_window(pairs, windows[_name], symmetric=True)
        _samples.append((_sample["opening_angle_deg"], angle_edges, f"{_name}, A < 0.5", "Opening angle [°]", f"{angular_bin_width.value}°"))
    for _axis, (_values, _edges, _title, _xlabel, _bin_label) in zip(_axes, _samples):
        _counts, _ = np.histogram(_values, bins=_edges)
        _axis.stairs(_counts * exposure_weight, _edges, color="tab:red", linestyle="--")
        _centres = (_edges[:-1] + _edges[1:]) / 2
        _axis.errorbar(_centres, _counts * exposure_weight, yerr=np.sqrt(_counts) * exposure_weight,
                      fmt="none", color="tab:red", alpha=0.5)
        _axis.set(title=f"{_title} — assumed 700 Hz, 300 h", xlabel=_xlabel,
                  ylabel=f"Expected events / {_bin_label}")
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Reading the result

    First inspect coincidence efficiency and the number of events in each window.
    Then compare energy–angle correlations, arm separation, asymmetry selections,
    and the shifted-vertex curves. A peak that changes with these choices supports
    a geometric/selection explanation within this simulation; it does not alone
    establish the origin of an experimental excess. Empty bins in a small sample
    do not establish zero background.

    For more precision, generate more independent muons with a new seed and output
    name. Do not count reruns with the same seed as independent exposure. Preserve
    the ROOT file and its configuration, geometry, and run manifest together.
    """)
    return


if __name__ == "__main__":
    app.run()
