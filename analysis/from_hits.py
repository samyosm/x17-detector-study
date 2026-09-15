import marimo

__generated_with = "0.24.0"
app = marimo.App(width="medium")


@app.cell
def _():
    import marimo as mo
    import matplotlib.pyplot as plt
    import numpy as np
    import pandas as pd
    import ROOT
    from scipy.stats import linregress

    ROOT.EnableImplicitMT()
    DATA_PATH = "./data/14september2026_100k_mixed.root"
    BAR_COUNT = 16
    SCINTILLATOR_RADIUS_CM = 21.0
    FIDUCIAL_HALF_LENGTH_CM = 55.0
    return (
        BAR_COUNT,
        DATA_PATH,
        FIDUCIAL_HALF_LENGTH_CM,
        ROOT,
        SCINTILLATOR_RADIUS_CM,
        linregress,
        mo,
        np,
        pd,
        plt,
    )


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    # Detector position reconstruction

    This notebook reconstructs the longitudinal hit position using light attenuation and photon timing. Simulated track momentum provides the reference position used to calibrate and evaluate both methods.
    """)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Input

    The `events` tree supplies the generated electron and positron momenta, generated opening angle, photon counts, and first photon times. Only these branches are read. ROOT processes independent entries concurrently and returns one NumPy array per branch.
    """)
    return


@app.cell
def _(BAR_COUNT, DATA_PATH, ROOT):
    def load_readout(path, bar_count):
        event_branches = [
            "event_id",
            "source_mode",
            "opening_angle_deg",
            "positron_px_MeV",
            "positron_py_MeV",
            "positron_pz_MeV",
            "electron_px_MeV",
            "electron_py_MeV",
            "electron_pz_MeV",
        ]
        pmt_branches = [
            f"pmt_{bar:02d}_{end}_{field}"
            for bar in range(1, bar_count + 1)
            for end in ("D", "U")
            for field in ("photons", "first_time_ns")
        ]
        return ROOT.RDataFrame("events", path).AsNumpy(event_branches + pmt_branches)

    readout = load_readout(DATA_PATH, BAR_COUNT)
    return (readout,)


@app.cell(hide_code=True)
def _(BAR_COUNT, mo):
    min_photon_count = mo.ui.number(
        start=0,
        step=1,
        value=100,
        debounce=True,
        label="Minimum photons at each end",
    )
    selected_bar = mo.ui.dropdown(
        options={f"Bar {bar:02d}": bar for bar in range(1, BAR_COUNT + 1)},
        value="Bar 08",
        label="Bar",
    )
    calibration_percent = mo.ui.slider(
        start=10,
        stop=90,
        step=5,
        value=70,
        show_value=True,
        label="Calibration data [%]",
    )
    mo.hstack([min_photon_count, calibration_percent, selected_bar], justify="start")
    return calibration_percent, min_photon_count, selected_bar


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Reference position

    Each generated electron and positron is projected in a straight line from the target to the scintillator mid-radius $R=21$ cm. For momentum $(p_x,p_y,p_z)$,

    $$z_{\rm truth}=R\frac{p_z}{\sqrt{p_x^2+p_y^2}}, \qquad
    \phi=\operatorname{atan2}(p_y,p_x).$$

    With $\Delta\phi=2\pi/16$, the nearest bar is

    $$k=1+\operatorname{round}\left(\frac{\phi\bmod2\pi}{\Delta\phi}\right)\bmod16.$$

    The mid-radius is used because the PMT reconstruction represents one position along a bar rather than the entry or exit surface. The resulting $(k,z_{\rm truth})$ is the reference used for calibration and evaluation.
    """)
    return


@app.cell
def _(BAR_COUNT, SCINTILLATOR_RADIUS_CM, np, pd, readout):
    def project_tracks(data, radius, bar_count):
        tracks = []
        angle_step = 2.0 * np.pi / bar_count

        for particle in ("positron", "electron"):
            px = data[f"{particle}_px_MeV"]
            py = data[f"{particle}_py_MeV"]
            pz = data[f"{particle}_pz_MeV"]
            transverse_momentum = np.hypot(px, py)
            safe_momentum = np.where(transverse_momentum == 0, np.nan, transverse_momentum)
            azimuth = np.mod(np.arctan2(py, px), 2.0 * np.pi)

            tracks.append(pd.DataFrame({
                "event_id": data["event_id"],
                "bar": np.mod(np.round(azimuth / angle_step).astype(int), bar_count) + 1,
                "z_truth": radius * pz / safe_momentum,
            }))

        return pd.concat(tracks, ignore_index=True).dropna(subset=["z_truth"])

    truth_df = project_tracks(readout, SCINTILLATOR_RADIUS_CM, BAR_COUNT)
    return (truth_df,)


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Selected measurements

    For threshold $N_{\min}$, an event-bar is retained when

    $$D>N_{\min}\quad\text{and}\quad U>N_{\min}.$$

    PMT rows are joined to truth using `(event_id, bar)`. If both particles map to the same event-bar, that key occurs twice and is removed because one pair of PMT signals cannot provide two positions. Finally,

    $$|z_{\rm truth}|\leq55\ \mathrm{cm}$$

    is required. This fiducial region excludes the final 15 cm at each end of the 140 cm bars, where end effects can violate the simple attenuation and timing models.
    """)
    return


@app.cell
def _(BAR_COUNT, FIDUCIAL_HALF_LENGTH_CM, min_photon_count, np, pd, readout, truth_df):
    def select_measurements(data, truth, bar_count, photon_threshold, half_length):
        selected = []

        for bar in range(1, bar_count + 1):
            d_count = data[f"pmt_{bar:02d}_D_photons"]
            u_count = data[f"pmt_{bar:02d}_U_photons"]
            keep = (d_count > photon_threshold) & (u_count > photon_threshold)
            selected.append(pd.DataFrame({
                "event_id": data["event_id"][keep],
                "bar": bar,
                "D": d_count[keep],
                "U": u_count[keep],
                "t_D": data[f"pmt_{bar:02d}_D_first_time_ns"][keep],
                "t_U": data[f"pmt_{bar:02d}_U_first_time_ns"][keep],
            }))

        pmt_data = pd.concat(selected, ignore_index=True)
        single_tracks = truth.drop_duplicates(["event_id", "bar"], keep=False)
        matched = pmt_data.merge(single_tracks, on=["event_id", "bar"], how="inner")
        return matched[matched["z_truth"].abs() <= half_length].copy()

    measurements_df = select_measurements(
        readout,
        truth_df,
        BAR_COUNT,
        min_photon_count.value,
        FIDUCIAL_HALF_LENGTH_CM,
    )
    return (measurements_df,)


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Calibration and validation samples

    Unique event IDs are shuffled with a fixed random seed. If the selected calibration fraction is $f$, the first $\lfloor fN\rfloor$ of the $N$ shuffled event IDs form the calibration sample and the remainder form the validation sample. Splitting by event ID keeps the electron and positron from one event together and prevents the same event from entering both samples.

    Calibration events determine the attenuation and timing constants. Validation events alone determine reconstructed positions, opening angles, plots, and RMSE values. The fixed seed makes the split reproducible when the notebook is rerun.
    """)
    return


@app.cell
def _(calibration_percent, measurements_df, np):
    def split_events(measurements, percent):
        event_ids = measurements["event_id"].unique().copy()
        generator = np.random.default_rng(0)
        generator.shuffle(event_ids)
        split_index = int(len(event_ids) * percent / 100)
        calibration_ids = event_ids[:split_index]
        is_calibration = measurements["event_id"].isin(calibration_ids)
        return measurements[is_calibration].copy(), measurements[~is_calibration].copy()

    calibration_df, validation_df = split_events(
        measurements_df,
        calibration_percent.value,
    )
    return calibration_df, validation_df


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Attenuation calibration

    For every calibration event-bar, the measured light asymmetry is

    $$y_i=\ln\left(\frac{D_i}{U_i}\right).$$

    Each bar is fitted independently with

    $$\ln\left(\frac{D}{U}\right)=m z+b.$$

    by minimizing $\sum_i[y_i-(mz_i+b)]^2$. The fitted quantities are converted using

    $$\beta=-m,\qquad \alpha_0=e^{-b},\qquad \lambda=\frac{2}{\beta}.$$

    SciPy supplies $\sigma_m$ and $\sigma_b$. First-order uncertainty propagation gives

    $$\sigma_\beta=\sigma_m,\qquad
    \sigma_{\alpha_0}=\alpha_0\sigma_b,\qquad
    \sigma_\lambda=\frac{2\sigma_\beta}{\beta^2}.$$

    Bars are fitted separately because their PMT gains and optical transport can differ.

    The calibration plot shows every selected $(z_i,y_i)$ pair for the chosen bar and the fitted line $-\beta z-\ln\alpha_0$.
    """)
    return


@app.cell
def _(calibration_df, linregress, np, pd):
    def fit_attenuation(measurements):
        data = measurements.copy()
        data["log_ratio"] = np.log(data["D"].astype(float) / data["U"].astype(float))
        fits = []

        for bar, group in data.groupby("bar"):
            fit = linregress(group["z_truth"], group["log_ratio"])
            beta = -fit.slope
            fits.append({
                "bar": bar,
                "events": len(group),
                "alpha_0": np.exp(-fit.intercept),
                "alpha_0_error": np.exp(-fit.intercept) * fit.intercept_stderr,
                "beta_cm_inv": beta,
                "beta_error": fit.stderr,
                "lambda_cm": 2.0 / beta,
                "lambda_error": 2.0 * fit.stderr / beta**2,
            })

        return data, pd.DataFrame(fits)

    attenuation_data_df, attenuation_fits_df = fit_attenuation(calibration_df)
    return attenuation_data_df, attenuation_fits_df


@app.cell
def _(attenuation_fits_df, mo):
    mo.ui.table(attenuation_fits_df, selection=None, page_size=16)
    return


@app.cell
def _(attenuation_data_df, attenuation_fits_df, mo, np, plt, selected_bar):
    def plot_attenuation_fit(data, fits, bar):
        selected = data[data["bar"] == bar].sort_values("z_truth")
        fit = fits[fits["bar"] == bar].iloc[0]
        fitted_log_ratio = (
            -fit["beta_cm_inv"] * selected["z_truth"] - np.log(fit["alpha_0"])
        )
        figure, axis = plt.subplots()
        axis.scatter(selected["z_truth"], selected["log_ratio"], s=5)
        axis.plot(selected["z_truth"], fitted_log_ratio)
        axis.set_xlabel("Truth z [cm]")
        axis.set_ylabel("ln(D/U)")
        axis.set_title(f"Attenuation fit for bar {bar:02d}")
        return axis

    attenuation_fit_axis = plot_attenuation_fit(
        attenuation_data_df,
        attenuation_fits_df,
        selected_bar.value,
    )
    mo.ui.matplotlib(attenuation_fit_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Position from attenuation

    For each validation measurement, the D/U counts are combined with the $\alpha_0$ and $\beta$ fitted from the calibration sample for its bar. Inverting the attenuation relation gives

    $$z_{\rm attenuation}=\frac{1}{\beta}
    \ln\left(\frac{U}{\alpha_0D}\right).$$

    For measurement $i$, the residual and root mean square error are

    $$r_i=z_{\rm attenuation,i}-z_{\rm truth,i},\qquad
    \operatorname{RMSE}=\sqrt{\frac{1}{N}\sum_{i=1}^{N}r_i^2}.$$

    RMSE is the only performance metric used here. It has units of centimetres and penalizes the large position errors that most strongly distort a reconstructed opening angle.

    The reconstruction plot shows every $(z_{\rm truth},z_{\rm attenuation})$ pair for the chosen bar. The line $z_{\rm attenuation}=z_{\rm truth}$ marks perfect reconstruction.
    """)
    return


@app.cell
def _(attenuation_fits_df, np, pd, validation_df):
    def summarize_positions(data, residual_column):
        evaluated = data.copy()
        evaluated["squared_error"] = evaluated[residual_column]**2
        summary = evaluated.groupby("bar", as_index=False).agg(
            events=(residual_column, "size"),
            mean_squared_error=("squared_error", "mean"),
        )
        summary["rmse_cm"] = np.sqrt(summary.pop("mean_squared_error"))
        return evaluated, summary

    def reconstruct_attenuation(data, fits):
        evaluated = data.copy()
        evaluated["log_ratio"] = np.log(
            evaluated["D"].astype(float) / evaluated["U"].astype(float)
        )
        calibration = fits[["bar", "alpha_0", "beta_cm_inv"]]
        reconstructed = evaluated.merge(calibration, on="bar", how="inner")
        reconstructed["z_attenuation"] = np.log(
            reconstructed["U"] / (reconstructed["alpha_0"] * reconstructed["D"])
        ) / reconstructed["beta_cm_inv"]
        reconstructed["attenuation_residual"] = (
            reconstructed["z_attenuation"] - reconstructed["z_truth"]
        )
        return reconstructed

    attenuation_reconstructed_df = reconstruct_attenuation(
        validation_df,
        attenuation_fits_df,
    )
    attenuation_reconstructed_df, attenuation_summary_df = summarize_positions(
        attenuation_reconstructed_df,
        "attenuation_residual",
    )
    return attenuation_reconstructed_df, attenuation_summary_df, summarize_positions


@app.cell(hide_code=True)
def _(attenuation_summary_df, mo, selected_bar):
    attenuation_result = attenuation_summary_df[
        attenuation_summary_df["bar"] == selected_bar.value
    ].iloc[0]
    mo.md(
        f"For bar {selected_bar.value:02d}, the attenuation RMSE is "
        f"{attenuation_result['rmse_cm']:.2f} cm."
    )
    return


@app.cell
def _(attenuation_reconstructed_df, mo, plt, selected_bar):
    def plot_attenuation_reconstruction(data, bar):
        selected = data[data["bar"] == bar]
        figure, axis = plt.subplots()
        axis.scatter(selected["z_truth"], selected["z_attenuation"], s=5)
        axis.plot([-55, 55], [-55, 55])
        axis.set_xlabel("Truth z [cm]")
        axis.set_ylabel("Attenuation z [cm]")
        axis.set_title(f"Attenuation reconstruction for bar {bar:02d}")
        return axis

    attenuation_axis = plot_attenuation_reconstruction(
        attenuation_reconstructed_df,
        selected_bar.value,
    )
    mo.ui.matplotlib(attenuation_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Opening angle from attenuation positions

    Two distinct reconstructed event-bars are paired by event. The centre of bar $k$ is at $\phi_k=2\pi(k-1)/16$. Combining its transverse centre with the reconstructed z gives

    $$\vec r=(R\cos\phi_k,R\sin\phi_k,z_{\rm attenuation}).$$

    Normalize the two vectors and calculate

    $$\theta=\cos^{-1}(\hat r_1\cdot\hat r_2).$$

    Only events with exactly two retained event-bars are used. Their angular error and RMSE are

    $$\delta\theta_i=\theta_{\rm reconstructed,i}-\theta_{\rm truth,i},\qquad
    \operatorname{RMSE}_\theta=\sqrt{\frac{1}{N}\sum_{i=1}^{N}(\delta\theta_i)^2}.$$

    The plotted distributions use common 2° bins from 0° to 180° so their counts can be compared directly.
    """)
    return


@app.cell
def _(BAR_COUNT, SCINTILLATOR_RADIUS_CM, attenuation_reconstructed_df, np, pd, readout):
    def calculate_opening_angles(
        positions,
        data,
        radius,
        bar_count,
        z_column,
        angle_column,
        error_column,
    ):
        complete = positions[
            positions.groupby("event_id")["event_id"].transform("size") == 2
        ].sort_values(["event_id", "bar"])
        phi = (complete["bar"].to_numpy() - 1) * 2.0 * np.pi / bar_count
        directions = np.column_stack([
            radius * np.cos(phi),
            radius * np.sin(phi),
            complete[z_column].to_numpy(),
        ])
        directions /= np.linalg.norm(directions, axis=1)[:, None]
        pairs = directions.reshape(-1, 2, 3)
        cosine = np.sum(pairs[:, 0] * pairs[:, 1], axis=1).clip(-1.0, 1.0)

        event_truth = pd.DataFrame({
            "event_id": data["event_id"],
            "opening_angle_truth_deg": data["opening_angle_deg"],
        })
        angles = pd.DataFrame({
            "event_id": complete["event_id"].to_numpy()[::2],
            angle_column: np.degrees(np.arccos(cosine)),
        }).merge(event_truth, on="event_id", how="left")
        angles[error_column] = angles[angle_column] - angles["opening_angle_truth_deg"]
        return angles

    opening_angles_df = calculate_opening_angles(
        attenuation_reconstructed_df,
        readout,
        SCINTILLATOR_RADIUS_CM,
        BAR_COUNT,
        "z_attenuation",
        "opening_angle_reconstructed_deg",
        "opening_angle_error_deg",
    )
    return calculate_opening_angles, opening_angles_df


@app.cell(hide_code=True)
def _(mo, opening_angles_df):
    opening_angle_rmse = (opening_angles_df["opening_angle_error_deg"]**2).mean()**0.5
    mo.md(
        f"For {len(opening_angles_df):,} reconstructed pairs, the opening-angle RMSE is "
        f"{opening_angle_rmse:.2f}°."
    )
    return


@app.cell
def _(mo, np, opening_angles_df, plt):
    def plot_opening_angles(data):
        figure, axis = plt.subplots()
        bins = np.linspace(0, 180, 91)
        axis.hist(data["opening_angle_truth_deg"], bins=bins, histtype="step", label="Truth")
        axis.hist(data["opening_angle_reconstructed_deg"], bins=bins, histtype="step", label="Reconstructed")
        axis.set_xlabel("Opening angle [degrees]")
        axis.set_ylabel("Events")
        axis.legend()
        return axis

    opening_angle_axis = plot_opening_angles(opening_angles_df)
    mo.ui.matplotlib(opening_angle_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Timing calibration

    Calibration rows with a non-finite first arrival time are removed. For every remaining calibration event-bar,

    $$\Delta t_i=t_{D,i}-t_{U,i}.$$

    Because D is at negative z and U at positive z, each bar is fitted to

    $$t_D-t_U=t_0+s z, \qquad v_{\rm eff}=\frac{2}{s}.$$

    by minimizing $\sum_i[\Delta t_i-(t_0+s z_i)]^2$. The intercept $t_0$ absorbs a fixed D/U timing offset, and

    $$v_{\rm eff}=\frac{2}{s}.$$

    SciPy supplies $\sigma_s$ and $\sigma_{t_0}$. Propagating the slope uncertainty gives

    $$\sigma_{v_{\rm eff}}=\frac{2\sigma_s}{s^2}.$$

    The factor of two follows because moving the hit by $z$ lengthens the path to D and shortens the path to U by the same amount. Since the estimator uses the earliest detected photon, $v_{\rm eff}$ is treated as an empirical calibration coefficient. The calibration plot shows every $(z_i,\Delta t_i)$ pair for the chosen bar and the fitted line $t_0+s z$.
    """)
    return


@app.cell
def _(calibration_df, linregress, np, pd):
    def prepare_timing(measurements):
        timed = measurements[
            np.isfinite(measurements["t_D"]) & np.isfinite(measurements["t_U"])
        ].copy()
        timed["delta_t_ns"] = timed["t_D"] - timed["t_U"]
        return timed

    def fit_timing(measurements):
        timed = prepare_timing(measurements)
        fits = []

        for bar, group in timed.groupby("bar"):
            fit = linregress(group["z_truth"], group["delta_t_ns"])
            fits.append({
                "bar": bar,
                "events": len(group),
                "v_eff_cm_ns": 2.0 / fit.slope,
                "v_eff_error": 2.0 * fit.stderr / fit.slope**2,
                "slope_ns_cm": fit.slope,
                "slope_error": fit.stderr,
                "time_offset_ns": fit.intercept,
                "time_offset_error": fit.intercept_stderr,
            })

        return timed, pd.DataFrame(fits)

    timing_data_df, timing_fits_df = fit_timing(calibration_df)
    return prepare_timing, timing_data_df, timing_fits_df


@app.cell
def _(mo, timing_fits_df):
    mo.ui.table(timing_fits_df, selection=None, page_size=16)
    return


@app.cell
def _(mo, plt, selected_bar, timing_data_df, timing_fits_df):
    def plot_timing_fit(data, fits, bar):
        selected = data[data["bar"] == bar].sort_values("z_truth")
        fit = fits[fits["bar"] == bar].iloc[0]
        fitted_time_difference = (
            fit["time_offset_ns"] + fit["slope_ns_cm"] * selected["z_truth"]
        )
        figure, axis = plt.subplots()
        axis.scatter(selected["z_truth"], selected["delta_t_ns"], s=5)
        axis.plot(selected["z_truth"], fitted_time_difference)
        axis.set_xlabel("Truth z [cm]")
        axis.set_ylabel("t_D - t_U [ns]")
        axis.set_title(f"Timing fit for bar {bar:02d}")
        return axis

    timing_fit_axis = plot_timing_fit(timing_data_df, timing_fits_df, selected_bar.value)
    mo.ui.matplotlib(timing_fit_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Position from timing

    For each validation measurement, the first-photon time difference is combined with the slope and offset fitted from the calibration sample for its bar. Inverting the timing relation gives

    $$z_{\rm timing,i}=\frac{\Delta t_i-t_0}{s}.$$

    The residual and RMSE are

    $$r_i=z_{\rm timing,i}-z_{\rm truth,i},\qquad
    \operatorname{RMSE}=\sqrt{\frac{1}{N}\sum_{i=1}^{N}r_i^2}.$$

    The reconstruction plot shows every $(z_{\rm truth},z_{\rm timing})$ pair for the chosen bar. The line $z_{\rm timing}=z_{\rm truth}$ marks perfect reconstruction.
    """)
    return


@app.cell
def _(prepare_timing, summarize_positions, timing_fits_df, validation_df):
    def reconstruct_timing(data, fits):
        timed = prepare_timing(data)
        reconstructed = timed.merge(
            fits[["bar", "slope_ns_cm", "time_offset_ns"]],
            on="bar",
            how="inner",
        )
        reconstructed["z_timing"] = (
            reconstructed["delta_t_ns"] - reconstructed["time_offset_ns"]
        ) / reconstructed["slope_ns_cm"]
        reconstructed["timing_residual"] = reconstructed["z_timing"] - reconstructed["z_truth"]
        return reconstructed

    timing_reconstructed_df = reconstruct_timing(validation_df, timing_fits_df)
    timing_reconstructed_df, timing_summary_df = summarize_positions(
        timing_reconstructed_df,
        "timing_residual",
    )
    return timing_fits_df, timing_reconstructed_df, timing_summary_df


@app.cell
def _(mo, timing_summary_df):
    mo.ui.table(timing_summary_df, selection=None, page_size=16)
    return


@app.cell(hide_code=True)
def _(mo, selected_bar, timing_summary_df):
    timing_result = timing_summary_df[timing_summary_df["bar"] == selected_bar.value].iloc[0]
    mo.md(
        f"For bar {selected_bar.value:02d}, the timing RMSE is "
        f"{timing_result['rmse_cm']:.2f} cm."
    )
    return


@app.cell
def _(mo, plt, selected_bar, timing_reconstructed_df):
    def plot_timing_reconstruction(data, bar):
        selected = data[data["bar"] == bar]
        figure, axis = plt.subplots()
        axis.scatter(selected["z_truth"], selected["z_timing"], s=5)
        axis.plot([-55, 55], [-55, 55])
        axis.set_xlabel("Truth z [cm]")
        axis.set_ylabel("Timing z [cm]")
        axis.set_title(f"Timing reconstruction for bar {bar:02d}")
        return axis

    timing_axis = plot_timing_reconstruction(timing_reconstructed_df, selected_bar.value)
    mo.ui.matplotlib(timing_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Opening angle from timing positions

    Two distinct timing-reconstructed event-bars are paired by event. For bar $k$, $\phi_k=2\pi(k-1)/16$, so its direction is constructed from

    $$\vec r=(R\cos\phi_k,R\sin\phi_k,z_{\rm timing}),\qquad
    \hat r=\frac{\vec r}{|\vec r|}.$$

    The angle, angular error, and RMSE are

    $$\theta=\cos^{-1}(\hat r_1\cdot\hat r_2),$$

    $$\delta\theta_i=\theta_{\rm timing,i}-\theta_{\rm truth,i},\qquad
    \operatorname{RMSE}_\theta=\sqrt{\frac{1}{N}\sum_{i=1}^{N}(\delta\theta_i)^2}.$$

    As in the attenuation result, only events with exactly two retained event-bars are used. Truth and timing-reconstructed distributions use the same 2° bins from 0° to 180°.
    """)
    return


@app.cell
def _(BAR_COUNT, SCINTILLATOR_RADIUS_CM, calculate_opening_angles, readout, timing_reconstructed_df):
    timing_opening_angles_df = calculate_opening_angles(
        timing_reconstructed_df,
        readout,
        SCINTILLATOR_RADIUS_CM,
        BAR_COUNT,
        "z_timing",
        "timing_opening_angle_deg",
        "timing_opening_angle_error_deg",
    )
    return (timing_opening_angles_df,)


@app.cell(hide_code=True)
def _(mo, timing_opening_angles_df):
    timing_angle_rmse = (
        timing_opening_angles_df["timing_opening_angle_error_deg"]**2
    ).mean()**0.5
    mo.md(
        f"For {len(timing_opening_angles_df):,} reconstructed pairs, the timing "
        f"opening-angle RMSE is {timing_angle_rmse:.2f}°."
    )
    return


@app.cell
def _(mo, np, plt, timing_opening_angles_df):
    def plot_timing_opening_angles(data):
        bins = np.linspace(0, 180, 91)
        figure, axis = plt.subplots()
        axis.hist(data["opening_angle_truth_deg"], bins=bins, histtype="step", label="Truth")
        axis.hist(data["timing_opening_angle_deg"], bins=bins, histtype="step", label="Reconstructed")
        axis.set_xlabel("Opening angle [degrees]")
        axis.set_ylabel("Events")
        axis.legend()
        return axis

    timing_opening_axis = plot_timing_opening_angles(timing_opening_angles_df)
    mo.ui.matplotlib(timing_opening_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Method comparison

    Attenuation and timing rows are paired using `(event_id, bar)`, so both methods are evaluated on exactly the same measurements. For method $M$,

    $$r_{M,i}=z_{M,i}-z_{\rm truth,i},\qquad
    \operatorname{RMSE}_M=\sqrt{\frac{1}{N}\sum_{i=1}^{N}r_{M,i}^2}.$$

    The method with the smaller RMSE is preferred because it produces the smaller quadratic position error on this sample. RMSE is computed once across all paired measurements and once within each bar. The residual plot uses common 2 cm bins spanning both complete residual ranges, so the two distributions are directly comparable without discarding outliers.

    The calibration and evaluation use the same events, making this an in-sample comparison. A paper-quality performance estimate should fit the calibration constants on one event sample and report RMSE on an independent sample.
    """)
    return


@app.cell
def _(attenuation_reconstructed_df, np, pd, timing_reconstructed_df):
    def compare_methods(attenuation, timing):
        paired = attenuation[
            ["event_id", "bar", "z_truth", "z_attenuation", "attenuation_residual"]
        ].merge(
            timing[["event_id", "bar", "z_timing", "timing_residual"]],
            on=["event_id", "bar"],
            how="inner",
        )
        summaries = []
        for method, residual in (
            ("Attenuation", "attenuation_residual"),
            ("Timing", "timing_residual"),
        ):
            summaries.append({
                "method": method,
                "events": len(paired),
                "rmse_cm": np.sqrt((paired[residual]**2).mean()),
            })

        paired["attenuation_squared_error"] = paired["attenuation_residual"]**2
        paired["timing_squared_error"] = paired["timing_residual"]**2
        per_bar = paired.groupby("bar", as_index=False).agg(
            events=("event_id", "size"),
            attenuation_mse=("attenuation_squared_error", "mean"),
            timing_mse=("timing_squared_error", "mean"),
        )
        per_bar["attenuation_rmse_cm"] = np.sqrt(per_bar.pop("attenuation_mse"))
        per_bar["timing_rmse_cm"] = np.sqrt(per_bar.pop("timing_mse"))
        return paired, pd.DataFrame(summaries), per_bar

    comparison_df, method_summary_df, comparison_by_bar_df = compare_methods(
        attenuation_reconstructed_df,
        timing_reconstructed_df,
    )
    return comparison_by_bar_df, comparison_df, method_summary_df


@app.cell(hide_code=True)
def _(method_summary_df, mo):
    best_method = method_summary_df.loc[method_summary_df["rmse_cm"].idxmin()]
    mo.md(
        f"{best_method['method']} performs better on this sample, with an RMSE of "
        f"{best_method['rmse_cm']:.2f} cm."
    )
    return


@app.cell
def _(comparison_by_bar_df, method_summary_df, mo):
    method_tables = mo.vstack([
        mo.ui.table(method_summary_df, selection=None, pagination=False),
        mo.ui.table(comparison_by_bar_df, selection=None, page_size=16),
    ])
    method_tables
    return


@app.cell
def _(comparison_df, mo, np, plt, selected_bar):
    def plot_residual_comparison(data, bar):
        selected = data[data["bar"] == bar]
        limit = 2.0 * np.ceil(max(
            selected["attenuation_residual"].abs().max(),
            selected["timing_residual"].abs().max(),
        ) / 2.0)
        bins = np.arange(-limit, limit + 2.0, 2.0)
        figure, axis = plt.subplots()
        axis.hist(selected["attenuation_residual"], bins=bins, histtype="step", label="Attenuation")
        axis.hist(selected["timing_residual"], bins=bins, histtype="step", label="Timing")
        axis.set_xlabel("Reconstructed z - truth z [cm]")
        axis.set_ylabel("Events")
        axis.set_title(f"Residuals for bar {bar:02d}")
        axis.legend()
        return axis

    comparison_axis = plot_residual_comparison(comparison_df, selected_bar.value)
    mo.ui.matplotlib(comparison_axis)
    return


if __name__ == "__main__":
    app.run()
