def load_readout(path, bar_count):
    import ROOT
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


def load_pair_truth(paths):
    import uproot
    return uproot.concatenate(
        [f"{path}:events" for path in paths],
        filter_name=["opening_angle_deg", "pair_mass_MeV"],
        library="np",
    )
