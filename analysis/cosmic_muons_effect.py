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

    H. Benmansour et al., [*On the importance of cosmic-ray background in the
    Atomki anomaly*](https://arxiv.org/abs/2609.18383v1), arXiv:2609.18383v1 (2026),
    show that cosmic muons can produce angular peaks in simulated five- and six-arm
    pair spectrometers. Detector geometry and deposited-energy selections shape
    these peaks, potentially imitating features attributed to X17.

    We apply their analysis to our sixteen-arm detector to estimate how often
    muons pass the pair selection and whether they create similar structure.
    A muon crossing two arms can be reconstructed as two particles leaving the
    target. We follow that reconstruction from deposited energy to opening angle,
    then estimate the background over an experimental exposure. The different
    geometry means the paper's peak positions and rates need not carry over.
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
    _manifest_path = Path(str(data_path) + ".run.txt")
    _manifest = _manifest_path.read_text() if _manifest_path.is_file() else ""
    _rate_lines = [line for line in _manifest.splitlines() if line.startswith("Source rate Hz:")]
    mo.stop(
        not _rate_lines and "power" not in run_config["source"]["cosmic_muons"]["angular"],
        mo.md("Keep the Guan run's `.root.run.txt` sidecar beside the ROOT file for exposure normalization."),
    )
    incident_rate_hz = float(_rate_lines[0].split(":", 1)[1]) if _rate_lines else 700.0
    rate_description = (
        "Guan model integrated over the saved generation domain and horizontal plane"
        if _rate_lines else "legacy assumption from the Atomki-background study"
    )
    return arm_count, data_path, events, generated_count, incident_rate_hz, rate_description, run_config


@app.cell(hide_code=True)
def _(arm_count, data_path, generated_count, mo, run_config):
    _thresholds = run_config["readout"]["thresholds"]
    _source = run_config["source"]["cosmic_muons"]
    mo.md(rf"""
    ## From incident muons to detector hits

    The selected run contains **{generated_count:,} incident muons** out of
    {run_config['runtime']['events']:,} requested events, with **{arm_count} detector arms**.
    Its source is a {_source['plane']['side_cm']:g} cm square at
    y = {_source['plane']['height_cm']:g} cm, with zenith angles up to
    {_source['angular']['zenith_max_deg']:g}° and kinetic energies from
    {_source['energy']['min_gev']:g} to {_source['energy']['max_gev']:g} GeV.
    These settings and the geometry are taken from the saved run records.

    Each arm combines a long scintillator, a thin ΔE scintillator, and a matching
    angular sector of the multiwire proportional chamber (MWPC). All three
    deposits must exceed their thresholds for the arm to count as hit.

    | Layer | Deposit must exceed |
    |---|---:|
    | MWPC gas | {_thresholds['mwpc_mev'] * 1000:g} keV |
    | Thin ΔE scintillator | {_thresholds['delta_e_mev'] * 1000:g} keV |
    | Long scintillator | {_thresholds['scintillator_mev'] * 1000:g} keV |

    These are ideal energy-deposit requirements; electronics resolution and
    coincidence timing are not modelled. The hit counts already incorporate
    these thresholds. Testing other thresholds requires recalculating the hits
    from the individual arm deposits.
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
    ## Selecting two-arm coincidences

    The number of hit arms is the event's *multiplicity*. We retain events with
    exactly two hit arms, giving **{len(two_arm_events):,} coincidences** and an
    efficiency of **{len(two_arm_events) / generated_count:.4%}**.
    Efficiency is the selected count divided by all generated muons, including
    those that miss the detector.

    Of these coincidences, **{invalid_pair_count:,}** have undefined energy or angle
    values and are excluded from subsequent distributions. They remain included
    in the trigger count. The multiplicity distribution below shows how often
    an incident muon produces each number of hit arms.
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
    ## Energy sharing between the arms

    For each selected arm, add the long-scintillator and ΔE deposits to obtain
    its energy, E₁ or E₂. The gas deposit establishes the hit but is excluded
    from this energy sum. Two quantities describe the pair:

    - **Summed energy:** E_sum = E₁ + E₂.
    - **Energy asymmetry:** A = |E₁ − E₂| / (E₁ + E₂).

    Equal deposits give A = 0; a deposit concentrated in one arm gives A near 1.
    We call events with A < 0.5 symmetric and those with A ≥ 0.5 asymmetric.
    This split tests whether comparable deposits in two arms preferentially
    produce an angular concentration.

    Energy spectra use 1 MeV bins. Angular plots use the bin width selected above.
    Curves retain their event counts so differences in selection yield remain
    visible. Energy windows include the lower boundary and exclude the upper
    boundary, keeping adjacent signal and control samples separate.
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
    ## Reconstructing the opening angle

    The MWPC provides a position for each hit arm. In the simulation this is the
    average of the deposit-step midpoints, weighted by their deposited energies.
    Draw a vector from the assumed target position to each of the two positions;
    the angle between those vectors is the reconstructed opening angle.

    A through-going muon need not originate at the target. Nevertheless, applying
    this pair reconstruction gives it an apparent opening angle. The same track's
    path through the scintillators determines its energy deposit, so energy and
    angle can become correlated.

    The maps below expose that correlation before any energy selection. Both use
    the detector centre as the assumed vertex and share the same data, bins, and
    colour scale. The highlighted bands identify the signal and control windows
    used in the comparisons that follow.
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
    ## Connecting the angle to detector geometry

    The {arm_count} arms are equally spaced around the beam axis, with a pitch of
    {360 / arm_count:g}°. For arms i and j, the smaller separation is the pitch
    multiplied by min(|i − j|, {arm_count} − |i − j|).

    This transverse separation helps identify which arm pairs produce a feature
    in the angular spectrum. The reconstructed angle also depends on where along
    the detector the deposits occur, so a single arm separation can produce a
    range of three-dimensional opening angles. The table counts each separation;
    the map relates it to the reconstructed angle.
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
    ## Testing the beryllium energy selection

    Following the beryllium-8 analysis, select a **16–20 MeV signal window** and
    compare it with a **12–16 MeV control window**. Here “signal window” names an
    energy selection; every event in this sample was generated by a cosmic muon.

    Within each window, compare symmetric and asymmetric energy sharing. The
    shared axes make both the angular shapes and the event yields comparable.
    A feature enhanced by a particular energy and asymmetry selection can arise
    from the correlation seen in the full energy–angle map.
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
    ## Testing the helium selection and target position

    The helium-4 comparison uses **18–22 MeV** for the signal window and
    **14–18 MeV** for the control window, retaining symmetric events. These
    selections overlap the beryllium windows and are evaluated separately.

    For each sample, reconstruct the angle twice: once from the detector centre
    and once from a point displaced by +25 mm along the beam axis, following the
    target offset considered in the study. Both curves contain the same events.
    Their difference measures the effect of the assumed vertex on reconstruction;
    the physical target and energy deposits have not been changed.
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
    ## Comparing signal and control shapes

    Bringing the beryllium selections onto one axis makes their relative sizes
    easier to compare. The curves show symmetric signal-window events,
    asymmetric signal-window events, and symmetric control-window events.
    Both energy windows are 4 MeV wide. Their counts are used directly, without
    rescaling the curves to equal areas.
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
def _(incident_rate_hz, mo, rate_description):
    mo.md(rf"""
    ## From selected counts to an exposure estimate

    If n of N incident muons pass a selection, its estimated efficiency is n / N.
    The table reports this fraction and a **95% Wilson binomial confidence
    interval**. Unlike a simple symmetric error bar, this interval gives a
    nonzero upper bound even when no events survive. It describes counting
    uncertainty in the simulation; it does not include uncertainty in the source
    or detector model.

    To estimate an experimental yield, multiply the efficiency by the expected
    number of incident muons. The rate is **{incident_rate_hz:.2f} muons/s**:
    {rate_description}. For **300 hours**, each simulated event carries a weight
    of this rate times 300 hours, divided by N.

    This is a source-model estimate, not a measurement of our setup.
    Scaling the sample changes its predicted yield but does not improve its
    statistical precision.
    """)
    return


@app.cell
def _(generated_count, incident_rate_hz, np, pairs, pd, select_window, two_arm_events, windows):
    assumed_incident_count = incident_rate_hz * 300 * 3600
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
            "Expected in 300 h": _count * exposure_weight,
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
    ## Predicted cosmic spectra

    Applying the exposure weight gives the cosmic energy and angular spectra
    below. The energy spectrum includes all two-arm coincidences; the angular
    spectra additionally require symmetric energy sharing in the beryllium windows.
    Error bars are the event weight times the square root of the simulated bin
    count, a Poisson approximation to Monte Carlo counting uncertainty. For empty
    selections, use the confidence interval in the table rather than a zero error bar.

    Estimating the cosmic fraction of an experimental pair sample also requires
    the nuclear background from internal pair conversion (IPC). That comparison
    needs a validated IPC model and its absolute reaction rate. This analysis
    estimates the cosmic contribution alone; the current simplified IPC source
    does not reproduce the study's full Zhang–Miller calculation.
    """)
    return


@app.cell
def _(angle_edges, angular_bin_width, energy_edges, exposure_weight, incident_rate_hz, mo, np, pairs, plt, select_window, windows):
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
        _axis.set(title=f"{_title} — {incident_rate_hz:.2f} Hz, 300 h", xlabel=_xlabel,
                  ylabel=f"Expected events / {_bin_label}")
    mo.as_html(_figure)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Interpreting the background

    The coincidence efficiency sets the overall rate. The energy–angle maps and
    arm separations explain its shape, while the energy, asymmetry, and vertex
    comparisons reveal how the final selection changes it. Together these checks
    distinguish a broad cosmic contribution from one concentrated in a candidate
    signal region.

    A small or empty selected sample limits what can be concluded. More independent
    simulated events improve counting precision; beam-off data test whether the
    predicted rates and shapes describe the real detector. The practical criterion
    is whether the validated cosmic contribution lies below the experiment's
    acceptable background in its final signal selection.
    """)
    return


if __name__ == "__main__":
    app.run()
