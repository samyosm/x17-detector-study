import marimo

__generated_with = "0.24.0"
app = marimo.App(width="medium")


@app.cell
def _():
    import uproot
    import numpy as np
    import matplotlib.pyplot as plt

    return np, plt, uproot


@app.cell
def _(np, plt, uproot):
    def plot(mode="", *paths):
        if not paths:
            raise ValueError("At least one ROOT file path must be provided.")

        tree_targets = [f"{path}:events" for path in paths]

        branches = ["source_mode", "opening_angle_deg", "pair_mass_MeV"]
        data = uproot.concatenate(tree_targets, filter_name=branches, library="np")

        source_mode = data["source_mode"]
        opening_angle_deg = data["opening_angle_deg"]
        pair_mass_MeV = data["pair_mass_MeV"]

        valid_mask = ~np.isnan(opening_angle_deg)
    
        angles = opening_angle_deg[valid_mask]
        masses = pair_mass_MeV[valid_mask]
        modes = source_mode[valid_mask]

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

    return (plot,)


@app.cell
def _(plot):
    DATA_PATH_IPC = "./data/ipc_readout.root"
    plot("IPC", DATA_PATH_IPC)
    return


@app.cell
def _(plot):
    DATA_PATH_X17 = "./data/x17_readout.root"
    plot("X17", DATA_PATH_X17)
    return


@app.cell
def _(plot):
    DATA_PATH_MIXED = "./data/mixed_readout.root"
    plot("Mixed", DATA_PATH_MIXED)
    return


if __name__ == "__main__":
    app.run()
