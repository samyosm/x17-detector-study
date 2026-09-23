import marimo

__generated_with = "0.24.0"
app = marimo.App(width="medium")


@app.cell
def _():
    from detector_plots import plot_primary_truth
    return (plot_primary_truth,)


@app.cell
def _(plot_primary_truth):
    DATA_PATH_IPC = "./data/ipc_readout.root"
    plot_primary_truth("IPC", DATA_PATH_IPC)
    return


@app.cell
def _(plot_primary_truth):
    DATA_PATH_X17 = "./data/x17_readout.root"
    plot_primary_truth("X17", DATA_PATH_X17)
    return


@app.cell
def _(plot_primary_truth):
    DATA_PATH_MIXED = "./data/14september2026_100k_mixed.root"
    plot_primary_truth("Mixed", DATA_PATH_MIXED)
    return


if __name__ == "__main__":
    app.run()
