# X17 detector study

## Setup and run

```sh
mamba env create -f environment.yml
mamba activate x17-detector-study
python scripts/generate_geometry.py
cmake -S . -B build -DCMAKE_PREFIX_PATH="$CONDA_PREFIX"
cmake --build build -j
./build/simulate_detection
```

Edit `config/configuration.toml` to set the source, run mode, and output file before running. Run commands from the repository root.

## Detector

![Whole detector](docs/figures/whole.png)

![Beam-axis view](docs/figures/beam_axis.png)

![Target](docs/figures/target.png)

## Results

## License

MIT License

Copyright (c) 2026 Samy Rahmani

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
