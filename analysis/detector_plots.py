import numpy as np
import matplotlib.pyplot as plt
from root_data import load_pair_truth


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


def plot_attenuation_reconstruction(data, bar):
    selected = data[data["bar"] == bar]
    figure, axis = plt.subplots()
    axis.scatter(selected["z_truth"], selected["z_attenuation"], s=5)
    axis.plot([-55, 55], [-55, 55])
    axis.set_xlabel("Truth z [cm]")
    axis.set_ylabel("Attenuation z [cm]")
    axis.set_title(f"Attenuation reconstruction for bar {bar:02d}")
    return axis


def plot_opening_angles(data, reconstructed_column="opening_angle_reconstructed_deg"):
    figure, axis = plt.subplots()
    bins = np.linspace(0, 180, 91)
    axis.hist(data["opening_angle_truth_deg"], bins=bins, histtype="step", label="Truth")
    axis.hist(data[reconstructed_column], bins=bins, histtype="step", label="Reconstructed")
    axis.set_xlabel("Opening angle [degrees]")
    axis.set_ylabel("Events")
    axis.legend()
    return axis


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


def plot_timing_reconstruction(data, bar):
    selected = data[data["bar"] == bar]
    figure, axis = plt.subplots()
    axis.scatter(selected["z_truth"], selected["z_timing"], s=5)
    axis.plot([-55, 55], [-55, 55])
    axis.set_xlabel("Truth z [cm]")
    axis.set_ylabel("Timing z [cm]")
    axis.set_title(f"Timing reconstruction for bar {bar:02d}")
    return axis


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


def plot_primary_truth(mode="", *paths):
    if not paths:
        raise ValueError("At least one ROOT file path must be provided.")

    data = load_pair_truth(paths)

    opening_angle_deg = data["opening_angle_deg"]
    pair_mass_MeV = data["pair_mass_MeV"]

    valid_mask = ~np.isnan(opening_angle_deg)

    angles = opening_angle_deg[valid_mask]
    masses = pair_mass_MeV[valid_mask]

    event_count = len(angles)

    fig, axes = plt.subplots(1, 2, figsize=(12, 5))

    fig.suptitle(f"Mode: {mode}, Event count: {event_count}")

    axes[0].hist(angles, bins=90, color="royalblue", edgecolor="black", alpha=0.75)
    axes[0].set_xlabel(r"Opening Angle $\theta_{ee}$ (degrees)")
    axes[0].set_ylabel("Events°")
    axes[0].set_title("Primary Pair Opening Angle")
    axes[0].grid(axis="y", linestyle="--", alpha=0.6)

    axes[1].hist(masses, bins=100, range=(0, 20), color="crimson", edgecolor="black", alpha=0.75)
    axes[1].set_xlabel(r"Invariant Mass $m_{ee}$ (MeV)")
    axes[1].set_ylabel("Events")
    axes[1].set_title("Pair Invariant Mass")
    axes[1].grid(axis="y", linestyle="--", alpha=0.6)

    plt.tight_layout()
    plt.show()
