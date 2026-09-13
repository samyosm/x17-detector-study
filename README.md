# X17 detector study

Minimal multithreaded Geant4 study with GDML geometry based on [*Status of the X17 search in Montreal*](X17_Montreal.pdf), section 2.

```sh
mamba env create -f environment.yml  # first time only
mamba activate x17-detector-study
python scripts/generate_geometry.py
cmake -S . -B build -DCMAKE_PREFIX_PATH="$CONDA_PREFIX"
cmake --build build -j
./build/simulate_detection
```

Edit [config/configuration.toml](config/configuration.toml) before running. Every runtime and source setting is required; a missing or invalid value stops the program. The executable accepts no arguments. Set `runtime.mode` to `batch` or `visualization`. Batch mode runs `runtime.events` events; `runtime.progress_interval` prints the cumulative number of completed events across workers at each interval (currently every 10,000 events). Visualization mode executes `runtime.visualization_macro` and opens the interactive Geant4 viewer. The input GDML and visualization macro paths are relative to the TOML file. The ROOT output path is relative to the directory where the executable is launched. The output file is replaced on each run.

The `source` table selects `gun`, `ipc`, `x17`, or `mixed`. Gun particle, energy, position, and direction are explicit there. Pair modes inject e⁺, e⁻, and a recoiling ⁸Be nucleus at the center of the ⁷LiF coating. Set the transition energy and X17 mass in the same table. The X17 mode samples a fixed-mass two-body intermediate; the IPC mode uses a simplified virtual-photon mass spectrum and isotropic pair decay. In `mixed` mode, `source.x17_fraction` is the probability that each event uses X17; the remaining events use IPC. Set it strictly between 0 and 1. This fraction is a chosen mixture for simulation, **not a predicted physical branching ratio**. These are acceptance templates, **not validated X17 or IPC predictions**. They omit nuclear multipole amplitudes, M1/E1 interference, polarization, beam recoil, resonance line shape, and absolute branching rates.

Each worker selects one `PrimarySource` at startup. `GunSource`, `IpcSource`, and `X17Source` own mode-specific behavior; `MixedSource` chooses between the IPC and X17 implementations for each event. `PairSource` shares the recoil and lepton-decay kinematics. `PrimaryGeneratorAction` only delegates event generation.

The C++ code is grouped by responsibility under `src/`: `source/` contains primary generation, `actions/` contains Geant4 run/event/step hooks and ROOT readout, `detector/` loads GDML, and `configuration/` reads TOML. Each header sits beside its implementation. `app/main.cc` wires these pieces together; `config/`, `geometry/`, and `scripts/` hold the editable settings, generated GDML, and geometry generator.

The `geometry` table supplies dimensions used by `scripts/generate_geometry.py`. Run that script again after changing geometry settings. The detector has a 36 cm MWPC with a 6 cm inner radius, surrounded by 16 scintillator bars of length 140 cm at a 16 cm inner radius. Each bar has one PMT at each end, for 32 channels. **D is at −z and U at +z.** A target-region beamline runs along x through the center of the barrel (z): protons enter from −x and strike a ⁷LiF coating on a 45° aluminum foil. The ⁷Li(p,γ)⁸Be reaction produces the excited beryllium state of interest; the target is lithium, not beryllium. The standard physics list does **not** implement that resonant production or an X17 particle.

The scintillator radial thickness is 100 mm and the MWPC wall is 1.0 mm Rohacell, as supplied for this study. The paper omits the bar tangential width, PMT body dimensions, and most target-region dimensions. These values are labeled as placeholders in the TOML file. The 0.8 mm carbon-fiber beamline wall and 45° target angle come from the paper. Carbon-fiber composition, Rohacell composition/density, and cooling-rod material are approximations. Confirm placeholder dimensions before using material effects for physics results. The beamline outside the target region, coolant circuit, MWPC wire/strip detail, and optical interfaces are not modeled.

The ROOT file contains an `events` TTree with `event_id`, `source_mode` (`0` gun, `1` IPC, `2` X17), pair truth, and two deposited-energy branches per scintillator: `pmt_01_D_edep_MeV` through `pmt_16_U_edep_MeV`. In a mixed run, `source_mode` records the component actually used for each event (`1` or `2`). Pair-truth branches are NaN in gun mode. Both ends currently receive the same bar energy deposit as a placeholder, not an optical-photon or calibrated PMT signal. Worker threads merge into one ROOT file, so rows may be out of order; use `event_id` to identify events.
