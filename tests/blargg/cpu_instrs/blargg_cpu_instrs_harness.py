import glob
import logging
import os
import queue
import subprocess
import sys
import threading
import unittest

from python.runfiles import Runfiles
from typing import Iterable, Tuple

# TODO Enhancements:
#   - Allow for no timeout option (should be doable by passing Nones all the way to subprocess/threading waits/joins)
#   - Can we have the timeout period be provided/enforced by the Bazel level?


# Test parameters
TEST_ROM_TIMEOUT_SECONDS = 120
TEST_ROM_PASSED_MARKER = "Passed"

# Test data and command information
GBEMU_BINARY_LOCATION = os.path.join("gbemu", "gbemu-bin")
GBEMU_BLARGG_LOGGING_OPTION = "--blarg_console"
GEBEMU_HEADLESS_MODE_OPTION = "--headless"


def process_output_contains_target_before_timeout(
    command: Iterable[str],
    target_string: str,
    timeout_sec: float,
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
        process.kill()
        process.wait()

    # Start output collection thread
    t = threading.Thread(target=collect_outputs, args=(program_output,))
    t.start()

    # Wait on the process until the timeout period. If we pass the wait without
    # falling into the TimeoutExpired exception block, we know the program found
    # the target string and already killed the process. Otherwise, we have
    # to kill the process in the exception block. Finally, we join the thread
    # in case it was still alive capturing the rest of the output.

    timeout_occurred = False

    try:
        process.wait(timeout=timeout_sec)
    except subprocess.TimeoutExpired:
        process.kill()
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

    if timeout_occurred:
        raise subprocess.TimeoutExpired(command, timeout_sec)

    # Return if the target string appears in any line of the output and the output itself
    output = "".join(program_output.queue)
    return any(target_string in line for line in program_output.queue), output


class BlarggCPUInstrsTest(unittest.TestCase):
    def test_run(self):
        r = Runfiles.Create()
        blargg_test_rom_glob = os.path.join(
            r.Rlocation("blargg_test_roms/cpu_instrs/individual/"),
            "*.gb"
        )

        # Find all the test roms
        test_roms = sorted(glob.glob(blargg_test_rom_glob))
        assert test_roms, f"No test roms found with glob: {blargg_test_rom_glob}"

        # For each test rom, start a subtest that runs GBEmu on the rom and detects
        # if the "Passed" marker appears in the serial output before the configured
        # timeout occurs.
        for test_rom in test_roms:
            base_rom_name = os.path.basename(test_rom)
            with self.subTest(f"cpu_instrs_{base_rom_name}", rom=test_rom):
                logger = logging.getLogger(
                    f"BlarggCPUInstrsTest.test_run_{base_rom_name}"
                )

                command = [
                    GBEMU_BINARY_LOCATION,
                    test_rom,
                    GBEMU_BLARGG_LOGGING_OPTION,
                    GEBEMU_HEADLESS_MODE_OPTION,
                ]

                passed_found, program_output = (
                    process_output_contains_target_before_timeout(
                        command,
                        TEST_ROM_PASSED_MARKER,
                        TEST_ROM_TIMEOUT_SECONDS,
                    )
                )
                logger.info(f"\nProgram Output:\n{program_output}")
                self.assertTrue(
                    passed_found,
                    msg=f"Blarg test did not output '{TEST_ROM_PASSED_MARKER}' message to serial output.",
                )


if __name__ == "__main__":
    logging.basicConfig(stream=sys.stderr, level=logging.INFO)
    unittest.main()
