"""GBEmu ROM integration test harness.

Runs the GBEmu binary against a single Game Boy ROM and verifies the result
in one of two modes:

  serial  Wait for a marker string (e.g. "Passed") on the emulated serial
          output before a timeout. Used by Blargg's test ROMs.

  image   Run the ROM for a fixed duration, dump the final framebuffer to a
          PNG, and compare it pixel-for-pixel against a golden reference
          image. Used by dmg-acid2.

Each invocation tests exactly one ROM; per-test reporting and parallelism
are handled by Bazel via the `gb_rom_test` macro in `tests/gb_rom_test.bzl`.
"""

import argparse
import os
import subprocess
import sys
import threading

from python.runfiles import Runfiles


def _resolve_runfile(runfiles: Runfiles, rlocation_path: str) -> str:
    resolved = runfiles.Rlocation(rlocation_path)
    if not resolved or not os.path.exists(resolved):
        raise FileNotFoundError(
            f"Could not resolve runfile {rlocation_path!r} (got {resolved!r})"
        )
    return resolved


def _run_gbemu(
    command: list[str],
    timeout_sec: float,
    stop_marker: str | None = None,
) -> tuple[str, bool]:
    """Run GBEmu and return (combined_output, stop_marker_seen).

    Streams stdout line-by-line. If `stop_marker` is provided, the process is
    terminated as soon as a line containing the marker is observed, so fast
    tests don't pay the full timeout. A wall-clock Timer terminates the
    process if `timeout_sec` elapses, which unblocks the read loop even when
    the emulator is not producing output (e.g. image mode). A timeout is not
    itself an error — callers decide based on mode.
    """
    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    timer = threading.Timer(timeout_sec, process.terminate)
    timer.daemon = True
    timer.start()

    captured: list[str] = []
    marker_seen = False
    try:
        for line in process.stdout:
            captured.append(line)
            if stop_marker is not None and stop_marker in line:
                marker_seen = True
                break
    finally:
        timer.cancel()
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        if process.stdout and not process.stdout.closed:
            process.stdout.close()

    return "".join(captured), marker_seen


def _run_serial_mode(args: argparse.Namespace, gbemu: str, rom: str) -> int:
    command = [
        gbemu,
        "--headless",
        "--dump_serial",
        f"--rom_file={rom}",
    ]
    print(f"[gb_rom_test] running: {' '.join(command)}", flush=True)

    output, marker_seen = _run_gbemu(
        command, args.rom_timeout, stop_marker=args.pass_marker
    )
    sys.stdout.write(output)
    sys.stdout.flush()

    if not marker_seen:
        print(
            f"[gb_rom_test] FAIL: pass marker {args.pass_marker!r} not found "
            f"in serial output within {args.rom_timeout}s",
            file=sys.stderr,
        )
        return 1
    return 0


def _run_image_mode(args: argparse.Namespace, gbemu: str, rom: str) -> int:
    from PIL import Image, ImageChops

    runfiles = Runfiles.Create()
    reference_png = _resolve_runfile(runfiles, args.reference_image)

    out_png = os.path.join(os.environ["TEST_TMPDIR"], "frame.png")
    if os.path.exists(out_png):
        os.remove(out_png)

    command = [
        gbemu,
        "--headless",
        f"--rom_file={rom}",
        f"--dump_display_path={out_png}",
    ]
    print(f"[gb_rom_test] running: {' '.join(command)}", flush=True)

    output, _ = _run_gbemu(command, args.rom_timeout)
    sys.stdout.write(output)
    sys.stdout.flush()

    if not os.path.exists(out_png):
        print(
            f"[gb_rom_test] FAIL: GBEmu did not produce {out_png}",
            file=sys.stderr,
        )
        return 1

    actual = Image.open(out_png).convert("RGB")
    expected = Image.open(reference_png).convert("RGB")
    if ImageChops.difference(actual, expected).getbbox() is not None:
        print(
            f"[gb_rom_test] FAIL: framebuffer at {out_png} does not match "
            f"reference {reference_png}",
            file=sys.stderr,
        )
        return 1
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--gbemu", required=True, help="rlocationpath of the gbemu binary"
    )
    parser.add_argument(
        "--rom", required=True, help="rlocationpath of the Game Boy ROM under test"
    )
    parser.add_argument("--mode", choices=("serial", "image"), required=True)
    parser.add_argument(
        "--rom-timeout",
        type=float,
        default=120.0,
        help="seconds to let the ROM run before terminating",
    )
    parser.add_argument(
        "--pass-marker", default="Passed", help="serial-mode success marker"
    )
    parser.add_argument(
        "--reference-image",
        default=None,
        help="image-mode rlocationpath of the golden PNG",
    )
    args = parser.parse_args()

    runfiles = Runfiles.Create()
    gbemu = _resolve_runfile(runfiles, args.gbemu)
    rom = _resolve_runfile(runfiles, args.rom)

    if args.mode == "serial":
        return _run_serial_mode(args, gbemu, rom)

    if args.reference_image is None:
        parser.error("--reference-image is required when --mode=image")
    return _run_image_mode(args, gbemu, rom)


if __name__ == "__main__":
    sys.exit(main())
