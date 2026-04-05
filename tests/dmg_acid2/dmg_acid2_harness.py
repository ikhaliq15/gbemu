import glob
import logging
import os
import queue
import subprocess
import sys
import threading
import unittest

from PIL import Image, ImageChops

from python.runfiles import Runfiles
from typing import Iterable, Tuple

# TODO Enhancements:
#   - This is a heavily hacked together harness based off the existing blargg cpu_instrs harness.
#    - It needs to be heavily cleaned up and the commonalities with the blargg cpu_instrs harness should be extracted out to a shared utility module.


# Test parameters
TEST_ROM_TIMEOUT_SECONDS = 10
TEST_ROM_PASSED_MARKER = "Passed"

# Test data and command information
GBEMU_BINARY_LOCATION = os.path.join("gbemu", "gbemu_exe")
GBEMU_DUMP_SERIAL_OPTION = "--dump_serial"
GEBEMU_HEADLESS_MODE_OPTION = "--headless"
GBEMU_ROM_OPTION = "--rom_file"
GEBEMU_DUMP_DISPLAY_ON_EXIT_OPTION = "--dump_display_path"
OUTPUT_REFERENCE_PNG_OPTION = "output.png"


def are_images_identical(img1_path, img2_path):
    # Open the images (ignore alpha channel)
    image_one = Image.open(img1_path).convert("RGB")
    image_two = Image.open(img2_path).convert("RGB")

    diff = ImageChops.difference(image_one, image_two)
    return not bool(diff.getbbox())


def process_output_contains_target_before_timeout(
    command: Iterable[str],
    target_string: str,
    timeout_sec: float,
    timeout_is_failure: bool = True,
) -> Tuple[bool, str]:
    # Start process
    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        text=True,
    )

    # Setup queue to collect process outputs
    program_output = queue.Queue()

    def collect_outputs(output: queue.Queue):
        for line in iter(process.stdout.readline, ""):
            if not line:
                continue
            output.put(line)
            if target_string in line:
                break
        process.stdout.close()
        process.terminate()
        process.wait()

    # Start output collection thread
    t = threading.Thread(target=collect_outputs, args=(program_output,))
    t.start()

    timeout_occurred = False

    try:
        process.wait(timeout=timeout_sec)
    except subprocess.TimeoutExpired:
        process.terminate()
        process.wait()

        timeout_occurred = True
    finally:
        if t.is_alive():
            t.join(timeout=timeout_sec)
            if t.is_alive():
                logging.warning(
                    "Output collection thread is still alive after join timeout. This may indicate an issue with the thread not exiting properly."
                )
                timeout_occurred = True

    if timeout_occurred and timeout_is_failure:
        raise subprocess.TimeoutExpired(command, timeout_sec)

    # Return if the target string appears in any line of the output and the output itself
    output = "".join(program_output.queue)
    return any(target_string in line for line in program_output.queue), output


class DMGAcid2Test(unittest.TestCase):
    def test_run(self):
        r = Runfiles.Create()

        dmg_acid2_test_rom_glob = r.Rlocation("dmg_acid2_rom/file/dmg-acid2.gb")
        test_roms = sorted(glob.glob(dmg_acid2_test_rom_glob))
        assert test_roms, f"No test roms found with glob: {dmg_acid2_test_rom_glob}"
        assert (
            len(test_roms) == 1
        ), f"Expected exactly one test rom, but found {len(test_roms)} with glob: {dmg_acid2_test_rom_glob}"
        test_rom = test_roms[0]

        golden_reference_png_glob = r.Rlocation("dmg_acid2/img/reference-dmg.png")
        golden_reference_pngs = sorted(glob.glob(golden_reference_png_glob))
        assert (
            golden_reference_pngs
        ), f"No golden reference PNGs found with glob: {golden_reference_png_glob}"
        assert (
            len(golden_reference_pngs) == 1
        ), f"Expected exactly one golden reference PNG, but found {len(golden_reference_pngs)} with glob: {golden_reference_png_glob}"
        golden_reference_png = golden_reference_pngs[0]

        test_reference_png = r.Rlocation(OUTPUT_REFERENCE_PNG_OPTION)

        # For each test rom, start a subtest that runs GBEmu on the rom and detects
        # if the "Passed" marker appears in the serial output before the configured
        # timeout occurs.
        for test_rom in test_roms:
            base_rom_name = os.path.basename(test_rom)
            with self.subTest(f"dmg_acid2_{base_rom_name}", rom=test_rom):
                logger = logging.getLogger(f"DMGAcid2Test.test_run_{base_rom_name}")

                test_reference_png_exists = os.path.exists(test_reference_png)
                self.assertFalse(
                    test_reference_png_exists,
                    f"Expected test reference PNG to not exist at {test_reference_png}, but it exists.",
                )

                command = [
                    GBEMU_BINARY_LOCATION,
                    GBEMU_DUMP_SERIAL_OPTION,
                    GEBEMU_HEADLESS_MODE_OPTION,
                    f"{GEBEMU_DUMP_DISPLAY_ON_EXIT_OPTION}={test_reference_png}",
                    GBEMU_ROM_OPTION,
                    test_rom,
                ]

                _, __ = process_output_contains_target_before_timeout(
                    command,
                    TEST_ROM_PASSED_MARKER,
                    TEST_ROM_TIMEOUT_SECONDS,
                    timeout_is_failure=False,
                )

                test_reference_png_exists = os.path.exists(test_reference_png)
                self.assertTrue(
                    test_reference_png_exists,
                    f"Expected test reference PNG to be dumped at {test_reference_png}, but it does not exist.",
                )

                self.assertTrue(
                    are_images_identical(test_reference_png, golden_reference_png),
                    "The test reference PNG does not match the golden reference PNG.",
                )

                if test_reference_png_exists:
                    os.remove(test_reference_png)


if __name__ == "__main__":
    logging.basicConfig(stream=sys.stderr, level=logging.INFO)
    unittest.main()
