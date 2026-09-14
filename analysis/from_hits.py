import marimo

__generated_with = "0.24.0"
app = marimo.App(width="medium")


@app.cell
def _():
    import marimo as mo
    import ROOT
    import numpy as np
    from scipy.stats import linregress
    import pandas as pd
    import altair as alt

    ROOT.EnableImplicitMT()
    DATA_PATH = "./data/mixed_readout.root"
    return DATA_PATH, ROOT, alt, linregress, mo, np, pd


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Detector light-response calibration

    Simulated PMT counts are read from the event tree. The charts below compare the D/U light ratio with the generated track position.
    """)
    return


@app.cell
def _(DATA_PATH, ROOT):
    _branches = [
        "event_id",
        "positron_px_MeV", "positron_py_MeV", "positron_pz_MeV",
        "electron_px_MeV", "electron_py_MeV", "electron_pz_MeV",
    ] + [f"pmt_{bar:02d}_{end}_photons" for bar in range(1, 17) for end in ("D", "U")]
    readout = ROOT.RDataFrame("events", DATA_PATH).AsNumpy(_branches)
    return (readout,)


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    The truth position is the straight-track intersection with the scintillator mid-radius:

    $$z_{\rm truth}=21\,\mathrm{cm}\,\frac{p_z}{\sqrt{p_x^2+p_y^2}}.$$

    The nearest of 16 bar centers is $1+\operatorname{round}(\phi/(2\pi/16))\bmod 16$, where $\phi=\operatorname{atan2}(p_y,p_x)$ and bar 1 is at $\phi=0$.
    """)
    return


@app.cell
def _(np, pd, readout):
    # Barrel cylinder radius: R_inner (16 cm) + t_radial/2 (5 cm) = 21 cm
    R_SCINT_CM = 21.0
    BAR_PHI_STEP = 2.0 * np.pi / 16.0

    def get_track_truth(px, py, pz, particle_label):
        pt = np.hypot(px, py)
        pt_safe = np.where(pt == 0, np.nan, pt)

        z_truth = R_SCINT_CM * (pz / pt_safe)

        phi = np.mod(np.arctan2(py, px), 2.0 * np.pi)

        bar = np.mod(np.round(phi / BAR_PHI_STEP).astype(int), 16) + 1

        return pd.DataFrame({
            "bar": bar,
            "z_truth": z_truth,
            "particle": particle_label
        })

    _truth_branches = [
        "event_id",
        "positron_px_MeV", "positron_py_MeV", "positron_pz_MeV",
        "electron_px_MeV", "electron_py_MeV", "electron_pz_MeV"
    ]
    truth_df = pd.DataFrame({_branch: readout[_branch] for _branch in _truth_branches})

    pos_df = get_track_truth(
        truth_df["positron_px_MeV"].values,
        truth_df["positron_py_MeV"].values,
        truth_df["positron_pz_MeV"].values,
        "e+"
    )
    pos_df["event_id"] = truth_df["event_id"].values

    ele_df = get_track_truth(
        truth_df["electron_px_MeV"].values,
        truth_df["electron_py_MeV"].values,
        truth_df["electron_pz_MeV"].values,
        "e-"
    )
    ele_df["event_id"] = truth_df["event_id"].values

    truth_df = (
        pd.concat([pos_df, ele_df], ignore_index=True)
        .dropna(subset=["z_truth"])
        [["event_id", "bar", "z_truth"]]
    )
    return (truth_df,)


@app.cell(hide_code=True)
def _(mo, readout):
    min_photon_count = mo.ui.number(start=0, step=1, value=100, debounce=True,
                                    label="Minimum photons at each end")
    selected_bar = mo.ui.dropdown(options={f"Bar {bar:02d}": bar for bar in range(1, 17)},
                                  value="Bar 08", label="Inspect bar")
    mo.vstack([
        mo.md(f"# PMT readout\n\n**{len(readout['event_id']):,} events** loaded. Change the minimum detected-photon count to update the calibration."),
        mo.hstack([min_photon_count, selected_bar], justify="start", gap=2),
    ])
    return min_photon_count, selected_bar


@app.cell
def _(min_photon_count, np, pd, readout, truth_df):
    _selected = []
    for _bar in range(1, 17):
        _d = readout[f"pmt_{_bar:02d}_D_photons"]
        _u = readout[f"pmt_{_bar:02d}_U_photons"]
        _keep = (_d > min_photon_count.value) & (_u > min_photon_count.value)
        if _keep.any():
            _selected.append(pd.DataFrame({
                "event_id": readout["event_id"][_keep], "bar": _bar,
                "D": _d[_keep], "U": _u[_keep],
            }))
    _valid_bars = pd.concat(_selected, ignore_index=True) if _selected else pd.DataFrame(
        columns=["event_id", "bar", "D", "U"])
    _valid_bars["log_ratio"] = np.log(_valid_bars["D"].astype(float) / _valid_bars["U"].astype(float))
    _single_track_truth = truth_df.drop_duplicates(subset=["event_id", "bar"], keep=False)
    calib_df = _valid_bars.merge(_single_track_truth, on=["event_id", "bar"], how="inner")
    return (calib_df,)


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    # Finding Z using Attenuation

    We know that $z = \frac{1}{\beta}\ln{\frac{U}{\alpha{(0)}D}}$ where $\beta = -\frac{1}{\lambda}$ and $\alpha{(0)} = \frac{G_U}{G_D}$. We need to find the per-bar $\alpha{(0)}$ and $\beta$ values.
    """)
    return


@app.cell
def _(calib_df, linregress, np, pd):
    fiducial = calib_df[calib_df["z_truth"].abs() <= 55.0]
    _fits = []
    for _bar_id, _group in fiducial.groupby("bar"):
        if len(_group) < 3 or _group["z_truth"].nunique() < 2:
            continue
        _res = linregress(_group["z_truth"], _group["log_ratio"])
        _beta = -_res.slope
        _fits.append({
            "bar": int(_bar_id), "events": len(_group),
            "beta_cm_inv": _beta, "beta_error": _res.stderr,
            "lambda_cm": 2.0 / _beta if _beta != 0 else np.nan,
            "lambda_error": 2.0 * _res.stderr / _beta**2 if _beta != 0 else np.nan,
            "alpha_0": np.exp(-_res.intercept),
            "alpha_0_error": np.exp(-_res.intercept) * _res.intercept_stderr,
            "slope": _res.slope, "intercept": _res.intercept,
        })
    fits_df = pd.DataFrame(_fits, columns=["bar", "events", "beta_cm_inv", "beta_error",
                                          "lambda_cm", "lambda_error", "alpha_0", "alpha_0_error",
                                          "slope", "intercept"])
    return fiducial, fits_df


@app.cell(hide_code=True)
def _(alt, fits_df, mo):
    _counts = alt.Chart(fits_df).mark_bar().encode(
        x=alt.X("bar:O", title="Scintillator bar"),
        y=alt.Y("events:Q", title="Fiducial event-bars"),
        tooltip=["bar", "events"],
    ).properties(title="Usable events by bar", height=240)
    _view = mo.vstack([
        mo.md("## Per-bar calibration"),
        mo.ui.altair_chart(_counts),
        mo.ui.table(fits_df[["bar", "events", "beta_cm_inv", "beta_error",
                             "lambda_cm", "lambda_error", "alpha_0", "alpha_0_error"]],
                    selection=None, page_size=16),
    ]) if not fits_df.empty else mo.md("No bars have enough matched events to fit at this threshold.")
    _view
    return


@app.cell(hide_code=True)
def _(alt, fiducial, fits_df, mo, selected_bar):
    _bar_data = fiducial[fiducial["bar"] == selected_bar.value]
    _fit = fits_df[fits_df["bar"] == selected_bar.value]
    if _bar_data.empty:
        _view = mo.md(f"## Bar {selected_bar.value:02d}\n\nNo matched events at this threshold.")
    else:
        # Bin before plotting so presentation remains responsive for large runs.
        _points = _bar_data.assign(z_bin=_bar_data["z_truth"].round(0)).groupby("z_bin", as_index=False).agg(
            log_ratio=("log_ratio", "mean"), events=("log_ratio", "size"))
        _chart = alt.Chart(_points).mark_circle(size=55).encode(
            x=alt.X("z_bin:Q", title="Projected z [cm]"),
            y=alt.Y("log_ratio:Q", title="Mean ln(D / U)"),
            size=alt.Size("events:Q", legend=None),
            tooltip=["z_bin", "log_ratio", "events"],
        ).properties(height=300)
        if not _fit.empty:
            _line = alt.Chart(_points).transform_calculate(
                fitted=f"{_fit.iloc[0]['slope']} * datum.z_bin + {_fit.iloc[0]['intercept']}"
            ).mark_line(color="orange").encode(x="z_bin:Q", y="fitted:Q")
            _chart = _chart + _line
        _view = mo.vstack([mo.md(f"## Bar {selected_bar.value:02d}: light asymmetry vs. position"),
                           mo.ui.altair_chart(_chart)])
    _view
    return


if __name__ == "__main__":
    app.run()
