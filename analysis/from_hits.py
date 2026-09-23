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
    import tomllib
    from pathlib import Path

    with (Path(__file__).resolve().parents[1] / "config/configuration.toml").open("rb") as _stream:
        _config = tomllib.load(_stream)
    DATA_PATH = str(Path(__file__).resolve().parents[1] / _config["runtime"]["output_file"])
    _geometry = tomllib.loads((Path(__file__).resolve().parents[1] / "config/geometry.toml").read_text())["geometry"]
    BAR_COUNT = _geometry["scintillator"]["count"]
    SCINTILLATOR_RADIUS_CM = (_geometry["scintillator"]["inner_radius_cm"]
                              + _geometry["scintillator"]["radial_thickness_cm"] / 2)
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

    The optical extension uses virtual sensors at the bar ends (D at negative z, U at positive z). These are photon counts, not the reference team's energy-based end signals. PVC side boundaries absorb photons; there is no measured reflective wrapping model.

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
def _(BAR_COUNT, DATA_PATH):
    from root_data import load_readout

    readout = load_readout(DATA_PATH, BAR_COUNT)
    return (readout,)


@app.cell(hide_code=True)
def _(BAR_COUNT, mo):
    min_photon_count = mo.ui.number(
        start=0,
        step=1,
        value=0,
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

    Each generated electron and positron is projected in a straight line from the source origin to the scintillator mid-radius $R=20.75$ cm. For momentum $(p_x,p_y,p_z)$,

    $$z_{\rm truth}=R\frac{p_z}{\sqrt{p_x^2+p_y^2}}, \qquad
    \phi=\operatorname{atan2}(p_y,p_x).$$

    With $\Delta\phi=2\pi/16$, the nearest bar is

    $$k=1+\operatorname{round}\left(\frac{(3\pi/2-\phi)\bmod2\pi}{\Delta\phi}\right)\bmod16.$$

    The mid-radius is used because the PMT reconstruction represents one position along a bar rather than the entry or exit surface. The resulting $(k,z_{\rm truth})$ is the reference used for calibration and evaluation.
    """)
    return


@app.cell
def _(BAR_COUNT, SCINTILLATOR_RADIUS_CM, readout):
    from reconstruction import project_tracks

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
def _(BAR_COUNT, FIDUCIAL_HALF_LENGTH_CM, min_photon_count, readout, truth_df):
    from reconstruction import select_measurements

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
def _(calibration_percent, measurements_df):
    from reconstruction import split_events

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
def _(calibration_df):
    from reconstruction import fit_attenuation

    attenuation_data_df, attenuation_fits_df = fit_attenuation(calibration_df)
    return attenuation_data_df, attenuation_fits_df


@app.cell
def _(attenuation_fits_df, mo):
    mo.ui.table(attenuation_fits_df, selection=None, page_size=16)
    return


@app.cell
def _(attenuation_data_df, attenuation_fits_df, mo, selected_bar):
    from detector_plots import plot_attenuation_fit

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
def _(attenuation_fits_df, validation_df):
    from reconstruction import summarize_positions

    from reconstruction import reconstruct_attenuation

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
def _(attenuation_reconstructed_df, mo, selected_bar):
    from detector_plots import plot_attenuation_reconstruction

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

    Two distinct reconstructed event-bars are paired by event. The centre of bar $k$ is at $\phi_k=3\pi/2-2\pi(k-1)/16$. Combining its transverse centre with the reconstructed z gives

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
def _(BAR_COUNT, SCINTILLATOR_RADIUS_CM, attenuation_reconstructed_df, readout):
    from reconstruction import calculate_opening_angles

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
def _(mo, opening_angles_df):
    from detector_plots import plot_opening_angles

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
def _(calibration_df):
    from reconstruction import prepare_timing

    from reconstruction import fit_timing

    timing_data_df, timing_fits_df = fit_timing(calibration_df)
    return prepare_timing, timing_data_df, timing_fits_df


@app.cell
def _(mo, timing_fits_df):
    mo.ui.table(timing_fits_df, selection=None, page_size=16)
    return


@app.cell
def _(mo, selected_bar, timing_data_df, timing_fits_df):
    from detector_plots import plot_timing_fit

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
def _(summarize_positions, timing_fits_df, validation_df):
    from reconstruction import reconstruct_timing

    timing_reconstructed_df = reconstruct_timing(validation_df, timing_fits_df)
    timing_reconstructed_df, timing_summary_df = summarize_positions(
        timing_reconstructed_df,
        "timing_residual",
    )
    return timing_reconstructed_df, timing_summary_df


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
def _(mo, selected_bar, timing_reconstructed_df):
    from detector_plots import plot_timing_reconstruction

    timing_axis = plot_timing_reconstruction(timing_reconstructed_df, selected_bar.value)
    mo.ui.matplotlib(timing_axis)
    return


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""
    ## Opening angle from timing positions

    Two distinct timing-reconstructed event-bars are paired by event. For bar $k$, $\phi_k=3\pi/2-2\pi(k-1)/16$, so its direction is constructed from

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
def _(mo, timing_opening_angles_df):
    from detector_plots import plot_opening_angles as plot_timing_opening_angles

    timing_opening_axis = plot_timing_opening_angles(timing_opening_angles_df, "timing_opening_angle_deg")
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

    Both methods use the same held-out validation events; calibration events are separate.
    """)
    return


@app.cell
def _(attenuation_reconstructed_df, timing_reconstructed_df):
    from reconstruction import compare_methods

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
def _(comparison_df, mo, selected_bar):
    from detector_plots import plot_residual_comparison

    comparison_axis = plot_residual_comparison(comparison_df, selected_bar.value)
    mo.ui.matplotlib(comparison_axis)
    return


if __name__ == "__main__":
    app.run()
