import numpy as np
import pandas as pd
from scipy.stats import linregress


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
            "bar": np.mod(np.round(np.mod(1.5 * np.pi - azimuth, 2 * np.pi) / angle_step).astype(int), bar_count) + 1,
            "z_truth": radius * pz / safe_momentum,
        }))

    return pd.concat(tracks, ignore_index=True).dropna(subset=["z_truth"])


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


def split_events(measurements, percent):
    event_ids = measurements["event_id"].unique().copy()
    generator = np.random.default_rng(0)
    generator.shuffle(event_ids)
    split_index = int(len(event_ids) * percent / 100)
    calibration_ids = event_ids[:split_index]
    is_calibration = measurements["event_id"].isin(calibration_ids)
    return measurements[is_calibration].copy(), measurements[~is_calibration].copy()


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
    phi = 1.5 * np.pi - (complete["bar"].to_numpy() - 1) * 2.0 * np.pi / bar_count
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
