import numpy as np
import pandas as pd
import uproot

COSINE_EDGES = np.linspace(np.cos(np.radians(70)), 1, 31)
ENERGY_EDGES_GEV = np.logspace(-1, 2, 31)
FIELDS = ["n_hit", "muon_kinetic_MeV", "cos_zenith", "energy_sum_MeV",
          "energy_asymmetry", "opening_angle_deg"]
WINDOWS = {"Be signal": (16, 20), "Be control": (12, 16),
           "He signal": (18, 22), "He control": (14, 18)}


def read_run(path):
    incident = np.zeros((len(COSINE_EDGES) - 1, len(ENERGY_EDGES_GEV) - 1))
    coincident = np.zeros_like(incident)
    multiplicity = np.zeros(17, dtype=np.int64)
    frames = []
    with uproot.open(path, handler=uproot.source.file.MemmapSource) as source:
        for batch in source["cosmic_events"].iterate(FIELDS, step_size="64 MB", library="np"):
            energy = batch["muon_kinetic_MeV"] / 1000
            cosine = batch["cos_zenith"]
            bins = (COSINE_EDGES, ENERGY_EDGES_GEV)
            incident += np.histogram2d(cosine, energy, bins=bins)[0]
            two_arm = batch["n_hit"] == 2
            coincident += np.histogram2d(cosine[two_arm], energy[two_arm], bins=bins)[0]
            counts = np.bincount(batch["n_hit"], minlength=len(multiplicity))
            multiplicity += counts[:len(multiplicity)]
            frames.append(pd.DataFrame({name: batch[name][two_arm] for name in FIELDS}))
    pairs = pd.concat(frames, ignore_index=True)
    pairs = pairs.loc[np.isfinite(pairs[FIELDS[3:]]).all(axis=1)]
    return {"incident": incident, "coincident": coincident,
            "multiplicity": multiplicity, "pairs": pairs}


def select_window(pairs, bounds):
    energy = pairs["energy_sum_MeV"]
    return pairs.loc[energy.ge(bounds[0]) & energy.lt(bounds[1])]
