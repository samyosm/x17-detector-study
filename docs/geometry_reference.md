# Geometry reference and optical extension

The geometry follows the **active** `B4c/src/B4cDetectorConstruction.cc` and
`B4c/src/B4cScintParameterisation.cc` in the supplied `WC_Scint_16dE_x` directory.
Files ending in `_nominal`, `_Vietnam`, `_foil`, or `~` are alternative versions,
not the geometry built by their CMake project. The September 14 technical and
configuration notes in the parent directory describe the superseded geometry.

## Matched geometry

| Component | Active reference implementation |
|---|---|
| World | 250 cm air cube, NIST `G4_AIR` |
| Long scintillators | 16 polystyrene trapezoids, 140 cm long, 9.5 cm radial thickness, inner apothem 16 cm |
| Wrapping | PVC polyhedral mother; 0.02 cm wrap allowance; trapezoid half-widths reduced by 0.02 cm |
| Chamber | Annular gas mother, radii 6–6.8 cm, length 36 cm; 1 mm inner and outer Rohacell daughters |
| Chamber gas | Density 0.0017 g/cm³; 80% Ar and 20% NIST CO₂ **by mass** |
| Rohacell | Density 0.1 g/cm³; original elemental fractions, including their sum of 1.0001 before Geant4 normalization |
| Beam pipe | NIST graphite, radii 2.995–3.085 cm, length 25 cm |
| Vacuum | Radius 2.99 cm, length 25 cm; the 0.005 cm radial air gap is intentional |
| Target | Natural NIST LiF disk, radius 0.15875 cm, thickness 1.9 µm; centre z = −0.05 cm; frame rotation +45° about x |
| Target support | Hollow copper tube and brass nut, with dimensions and positions derived exactly as in the reference |
| Flanges | Aluminium rings at z = ±13.77 cm; radii 2.29–4.76 cm; length 2.54 cm |
| ΔE detectors | 16 polystyrene boxes, 0.2 × 2.8 × 8.094 cm, radius 14.372 cm, inside air cavities in PLA housings |

Preserved quirks:

- The reference constructs a copper foil logical volume but comments out its
  placement. There is **no backing foil** in the transported geometry.
- The pair source stays at the origin even though the target is at −0.5 mm.
- Long-bar copy 0 is at negative y; copies advance clockwise as viewed from +z.
  ROOT labels remain 01–16, corresponding to official copies 0–15.
- Bar rotations retain `3.1416/2`, including its tiny departure from π/2.
  The generator converts the original rotation composition to GDML Euler angles.
- The PVC mother retains the supplied angular span `2*pi + pi/16`; Geant4 treats
  this as a closed polygon. Its local z range is −0.02 to 140.02 cm and its
  world translation is −70 cm.
- Unplaced flat wire chambers, unused Teflon, and the unused `RadPLA` variable do
  not add material to the geometry.

## Optical extension

The reference has **no PMT volumes, optical surfaces, or optical physics**.
Its end energies and times are parametrized from deposited energy and a first-hit
position. This project retains optical transport as requested, without adding
PMT cylinders to the matched geometry.

Virtual absorbing sensors cover each long scintillator's complete end face.
The stepping action converts the boundary point to the bar's local coordinates,
checks its local y end plane, applies the configured constant quantum efficiency,
and records arrival time and photon energy. It terminates that photon regardless
of detection. There is no glass transmission, window, PMT transit-time spread,
or electronics model. The obsolete glass-index setting has been removed.
PVC has no refractive-index table: photons reaching it are absorbed. End arrivals
are scored even when the boundary process terminates them for that reason.
No reflective wrapping properties are invented. Polystyrene is shared by long
bars and ΔE detectors, so both scintillate; only long-bar ends are scored.

The extension preserves **D = negative z, U = positive z** and the existing ROOT
branch names (`pmt_...`). The reference's parametrized U/D formulas use the
opposite end convention, and are not copied into this optical study. Optical
counts and times must not be interpreted as reproducing their energy-based
readout. Their ΔE and chamber sensitive-detector response and their event
selection are also not ported; the corresponding materials are transported.
The existing pair-source physics remains this project's source model.

The analysis reads geometry radius, bar count, and output path from TOML, and
uses the corrected clockwise azimuths. Straight-line projection to the centre
radius remains an approximation, not exact tracking through trapezoids. Existing
ROOT files describe the earlier geometry; generate fresh data
before interpreting performance. The output filename now distinguishes the new
geometry. The default photon threshold is zero because absorbing PVC reduces
collection substantially.

## Reproduce the comparison

With the project environment activated:

```sh
python scripts/generate_geometry.py
cmake -S tests/geometry -B build/geometry-check -DCMAKE_PREFIX_PATH="$CONDA_PREFIX"
cmake --build build/geometry-check -j
build/geometry-check/geometry_probe reference unused /tmp/x17-reference.txt
build/geometry-check/geometry_probe gdml geometry/world.gdml /tmp/x17-generated.txt
python tests/geometry/compare.py /tmp/x17-reference.txt /tmp/x17-generated.txt
```

The probe compiles the supplied reference detector and its dependencies directly;
it does not alter them. Set CMake `REFERENCE_DIR` to use another reference location.
The comparison covers all 76 placed instances, hierarchy depth, copy numbers,
world transforms, shape parameters, bounding limits, density and elemental mass
fractions. Both constructions run Geant4 overlap checks. The reference's
parameterized bars are expanded for comparison with the individual GDML placements.

For small runs without changing production settings, copy the configuration,
use absolute geometry/macro paths, change event count and output filename, and run
`./build/simulate_detection --config /path/to/test.toml`.

## Validation on this checkout

- C++ build and notebook static checks passed.
- All 76 reference placements matched, including complete shape parameters.
- Geant4 overlap checks reported no overlaps in either construction.
- A 200-event mixed smoke run produced 1,902 detected photons; every event/channel count and first time matched its photon records.
- Two 20-event electron samples at bar 01, z = ±50 cm confirmed D/U end orientation.

Reference source SHA-256 fingerprints:

```text
ce831d74c31d2868378bdcaa4cfb154175023557ad9d3a8d72fa3a1ac038df42  B4cDetectorConstruction.cc
5c556fffddc4c5a437135c588091b22502249f84b994804fa6277ba96c06d056  B4cScintParameterisation.cc
```

## Visualization in the reference color scheme

[Open the self-contained comparison gallery](figures/reference_comparison/index.html).
It contains seven paired views rendered directly from the compiled reference and
our GDML using Geant4's ToolsSG offscreen driver. The full view uses the reference
camera (theta 60°, phi 60°), gray background, opaque colors and default visibility.
Other views explicitly identify the covers hidden to reveal nested parts. The
single ΔE housing view uses black/white wireframes to expose the cyan scintillator.
The default geometry and all transport materials remain unchanged.

| Part | Reference color | Placed instances |
|---|---|---:|
| Long scintillators | Cyan | 16 |
| PVC enclosure around long scintillators | Green | 1 |
| ΔE scintillators | Cyan | 16 |
| ΔE PLA housings | Black | 16 |
| Air cavities inside the housings | White | 16 |
| Chamber gas | (0.9, 0.6, 0.4) | 1 |
| Inner and outer Rohacell walls | Gray | 2 |
| Graphite beam pipe | Green | 1 |
| Aluminium flanges | (0.67, 0.65, 0.61) | 2 |
| LiF target | (0.9, 0.77, 0.13) | 1 |
| Hollow copper cooling tube | (0.64, 0.49, 0.06) | 1 |
| Brass nut | Gray | 1 |
| World and beam vacuum | Invisible | 2 |

The visualization attributes are applied in `src/detector/Visualization.hh` and
checked against the original colors/visibility by the numerical geometry probe.
To regenerate the gallery after building `tests/geometry`:

```sh
python scripts/render_geometry_comparison.py
```

The output contains the native PNG images, render logs and a self-contained HTML
gallery. For an interactive Geant4 window, use runtime visualization mode and
`macros/figures/whole.mac`. Use `macros/figures/delta_e.mac` to expose the ΔE ring.
