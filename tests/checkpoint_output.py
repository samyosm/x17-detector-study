import argparse
import hashlib
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time

import numpy as np
import uproot


def write_config(directory, name, events, checkpoint):
    project = Path(__file__).resolve().parents[1]
    text = (project / "config/cosmic_muons.toml").read_text()
    replacements = {
        "threads": "2",
        "events": str(events),
        "checkpoint_events": str(checkpoint),
        "output_file": f'"{directory / name}.root"',
    }
    for key, value in replacements.items():
        text = re.sub(rf"^{key} = .*", f"{key} = {value}", text, flags=re.MULTILINE)
    text = text.replace("../geometry/world.gdml", str(project / "geometry/world.gdml"))
    path = directory / f"{name}.toml"
    path.write_text(text)
    return path


def read_event_ids(path):
    with uproot.open(path, handler=uproot.source.file.MemmapSource) as source:
        cosmic_ids = source["cosmic_events"]["event_id"].array(library="np")
        event_ids = source["events"]["event_id"].array(library="np")
        np.testing.assert_array_equal(np.sort(cosmic_ids), np.sort(event_ids))
        for tree in ["cosmic_arms", "cosmic_steps", "photon_hits"]:
            ids = source[tree]["event_id"].array(library="np")
            assert np.isin(ids, cosmic_ids).all(), tree
    return np.sort(cosmic_ids)


def run_successfully(binary, config, log):
    with log.open("w") as output:
        subprocess.run([binary, "--config", str(config)], stdout=output,
                       stderr=subprocess.STDOUT, check=True, timeout=90)


def check_completed_runs(binary, directory):
    config = write_config(directory, "complete", 250, 100)
    run_successfully(binary, config, directory / "complete.log")
    folder = directory / "complete.root.parts"
    output = directory / "complete.root"
    assert not folder.exists()
    np.testing.assert_array_equal(read_event_ids(output), np.arange(250))
    for extension in [".toml", ".gdml", ".run.txt"]:
        assert Path(str(output) + extension).is_file()
    assert "Completed events: 250" in Path(str(output) + ".run.txt").read_text()
    before = hashlib.sha256(output.read_bytes()).digest()
    repeated = subprocess.run([binary, "--config", str(config)], capture_output=True, timeout=30)
    assert repeated.returncode != 0
    assert b"Output already exists" in repeated.stderr
    assert hashlib.sha256(output.read_bytes()).digest() == before
    config = write_config(directory, "single", 25, 100)
    run_successfully(binary, config, directory / "single.log")
    np.testing.assert_array_equal(read_event_ids(directory / "single.root"), np.arange(25))


def check_interrupted_run(binary, directory):
    config = write_config(directory, "interrupted", 100000000, 100)
    folder = directory / "interrupted.root.parts"
    first = folder / "part-000001.root"
    second = folder / "part-000002.partial.root"
    with (directory / "interrupted.log").open("w") as log:
        process = subprocess.Popen([binary, "--config", str(config)], stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 90
            while not (first.exists() and second.exists()):
                assert process.poll() is None, "Simulation exited before the interruption test"
                assert time.monotonic() < deadline, "Checkpoint was not produced"
                time.sleep(0.005)
            process.kill()
            process.wait(timeout=15)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=15)
    np.testing.assert_array_equal(read_event_ids(first), np.arange(100))
    assert process.returncode != 0


def check_failed_merge(binary, directory):
    config = write_config(directory, "failed-merge", 2500, 1000)
    folder = directory / "failed-merge.root.parts"
    first = folder / "part-000001.root"
    with (directory / "failed-merge.log").open("w") as log:
        process = subprocess.Popen([binary, "--config", str(config)], stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 90
            while not first.exists():
                assert process.poll() is None
                assert time.monotonic() < deadline
                time.sleep(0.005)
            process.send_signal(signal.SIGSTOP)
            first.write_bytes(b"Simulated damaged checkpoint")
            process.send_signal(signal.SIGCONT)
            assert process.wait(timeout=90) != 0
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=15)
    assert not (directory / "failed-merge.root").exists()
    np.testing.assert_array_equal(read_event_ids(folder / "part-000002.root"), np.arange(1000, 2000))
    np.testing.assert_array_equal(read_event_ids(folder / "part-000003.root"), np.arange(2000, 2500))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    binary = str(parser.parse_args().binary.resolve())
    with tempfile.TemporaryDirectory(prefix="x17-checkpoint-test-") as temporary:
        directory = Path(temporary)
        check_completed_runs(binary, directory)
        check_interrupted_run(binary, directory)
        check_failed_merge(binary, directory)
    print("Checkpoint completion, event IDs, overwrite protection, single-file merging, forced interruption, and failed-merge recovery passed.")


if __name__ == "__main__":
    main()
