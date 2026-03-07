#!/usr/bin/env python3
"""
tdleaf_selfplay.py — Drive a TDLEAF-enabled EXchess binary in self-play
to accumulate online TDLeaf(λ) training.

Usage (from run/ directory):
    python3 tdleaf_selfplay.py EXchess_v2026_03_07b_tdleaf -n 200 --tc 1
    python3 tdleaf_selfplay.py EXchess_v2026_03_07b_tdleaf -n 50 --depth 6
    python3 tdleaf_selfplay.py EXchess_v2026_03_07b_tdleaf -n 10 --depth 6 --verbose

The binary must be built with:
    perl comp.pl <name> NNUE=1 TDLEAF=1

Run from run/ so that nn-ad9b42354671.nnue and nn-ad9b42354671.tdleaf.bin
are found in the working directory.

Protocol (xboard, single process):
    Per game:  new  →  st <tc>  [→ sd <depth>]  →  go
    Per move:  on seeing "move <uci>" from engine → send "go"
    Game end:  on seeing "1-0"/"0-1"/"1/2-1/2" → wait for TDLeaf stderr
               confirmation, then start next game.
"""

import argparse
import os
import re
import subprocess
import sys
import threading
import time
from queue import Queue, Empty

# Patterns matched against engine stdout.
# We search() rather than match() because on the very first move of the session
# the non-xboard prompt ("White-To-Move[1]: ") may be prepended to the move line
# if the engine printed the prompt before our "xb" arg / "xboard" command was
# processed.  search() handles both the clean form ("move Nf3") and the prefixed
# form ("White-To-Move[1]: move Nf3").
MOVE_RE   = re.compile(r'\bmove\s+(\S+)$')
RESULT_RE = re.compile(r'\b(1-0|0-1|1/2-1/2)\b')
# Patterns matched against engine stderr
TDLEAF_OK = re.compile(r'^TDLeaf: updated weights for (\d+)-ply game')
TDLEAF_SK = re.compile(r'^TDLeaf: skipping short game \((\d+) plies\)')


def reader_thread(stream, q, label):
    """Read lines from *stream* and put (label, line) tuples onto *q*.
    Sends (label, None) as a sentinel on EOF."""
    try:
        for line in stream:
            q.put((label, line.rstrip('\n')))
    except Exception:
        pass
    finally:
        q.put((label, None))


def send(proc, cmd, verbose=False):
    """Write a single command line to the engine's stdin."""
    if verbose:
        print(f"  >>> {cmd}", flush=True)
    proc.stdin.write(cmd + '\n')
    proc.stdin.flush()


def drain(q, timeout=0.15):
    """Discard everything in *q* for up to *timeout* seconds."""
    deadline = time.monotonic() + timeout
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        try:
            q.get(timeout=min(remaining, 0.05))
        except Empty:
            break


def start_game(proc, tc, depth, verbose=False):
    """Send the per-game command sequence."""
    send(proc, "new", verbose)
    # max_search_time is an int in EXchess (centiseconds = seconds * 100).
    # Send as a whole number of seconds; clamp to at least 1 so st 0 doesn't
    # collapse the time limit to zero.  When --depth is set it dominates anyway.
    tc_int = max(1, round(tc))
    send(proc, f"st {tc_int}", verbose)
    if depth:
        send(proc, f"sd {depth}", verbose)
    send(proc, "go", verbose)


def check_tdleaf_line(line):
    """Return (plies, skipped) if *line* is a TDLeaf confirmation, else None."""
    m = TDLEAF_OK.match(line)
    if m:
        return int(m.group(1)), False
    m = TDLEAF_SK.match(line)
    if m:
        return int(m.group(1)), True
    return None


def wait_for_tdleaf(q, timeout, verbose=False, stderr_buffer=None):
    """Wait up to *timeout* seconds for a TDLeaf confirmation on stderr.

    *stderr_buffer* is a list of stderr lines already consumed from *q* during
    the preceding game loop (TDLeaf writes to unbuffered stderr, so its message
    can arrive before the stdout result line is flushed from the pipe buffer).
    We check that list first before blocking on the queue.

    Returns (plies, skipped) where:
      plies   — number of plies updated (int), or None on timeout/no-TDLEAF
      skipped — True if the game was too short and skipped
    """
    # Check lines already buffered during game play.
    for line in (stderr_buffer or []):
        result = check_tdleaf_line(line)
        if result:
            return result

    print(f"  [waiting for TDLeaf — up to {timeout:.0f}s]", flush=True)
    deadline = time.monotonic() + timeout
    found    = False
    plies    = None
    skipped  = False

    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        try:
            label, line = q.get(timeout=min(remaining, 0.5))
        except Empty:
            if found:
                break
            continue

        if line is None:
            break

        if verbose:
            tag = "OUT" if label == 'out' else "ERR"
            print(f"  [{tag}] {line}", flush=True)

        if label == 'err':
            result = check_tdleaf_line(line)
            if result:
                plies, skipped = result
                found = True
                deadline = min(deadline, time.monotonic() + 0.25)

    return plies, skipped


def main():
    run_dir = os.path.dirname(os.path.abspath(__file__))

    parser = argparse.ArgumentParser(
        description="Self-play TDLeaf training loop for TDLEAF-enabled EXchess.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "binary",
        help="EXchess binary name (in run/) or absolute path",
    )
    parser.add_argument(
        "-n", "--games", type=int, default=100,
        help="Number of self-play games (default: 100)",
    )
    parser.add_argument(
        "--tc", type=float, default=1.0,
        help="Fixed time per move in seconds (default: 1.0)",
    )
    parser.add_argument(
        "--depth", type=int, default=None,
        help="Max search depth; overrides time control when set (default: off)",
    )
    parser.add_argument(
        "--tdleaf-timeout", type=float, default=15.0,
        help="Seconds to wait for TDLeaf stderr confirmation per game (default: 15)",
    )
    parser.add_argument(
        "--move-timeout", type=float, default=60.0,
        help="Seconds to wait for a single engine move before aborting (default: 60)",
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Print all raw engine input/output lines",
    )
    args = parser.parse_args()

    # Resolve binary path
    binary = (
        args.binary if os.path.isabs(args.binary)
        else os.path.join(run_dir, args.binary)
    )
    if not os.path.isfile(binary):
        print(f"Error: binary not found: {binary}", file=sys.stderr)
        sys.exit(1)

    # Run in the binary's directory so .nnue / .tdleaf.bin are found
    work_dir     = os.path.dirname(binary)
    weights_file = os.path.join(work_dir, "nn-ad9b42354671.tdleaf.bin")

    tc_str = f"{args.tc}s/move"
    if args.depth:
        tc_str += f"  depth≤{args.depth}"

    print(f"Binary:       {binary}")
    print(f"Games:        {args.games}")
    print(f"Time ctrl:    {tc_str}")
    print(f"Move timeout: {args.move_timeout:.0f}s   TDLeaf timeout: {args.tdleaf_timeout:.0f}s")
    print(f"Work dir:     {work_dir}")
    if args.verbose:
        print("Verbose:      on")
    print()

    # ----------------------------------------------------------------
    # Launch engine
    # ----------------------------------------------------------------
    # Pass "xb" so xboard=1 is set before the main loop's first iteration.
    # Without this, EXchess prints "White-To-Move[1]: " (no newline) at startup
    # before reading the "xboard" command from stdin, and Python's readline()
    # fuses it with the first move line into "White-To-Move[1]: move Nf3".
    proc = subprocess.Popen(
        [binary, "xb"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=work_dir,
        text=True,
        bufsize=1,
    )

    q = Queue()
    threading.Thread(
        target=reader_thread, args=(proc.stdout, q, 'out'), daemon=True
    ).start()
    threading.Thread(
        target=reader_thread, args=(proc.stderr, q, 'err'), daemon=True
    ).start()

    # Drain startup banner (~0.5 s); print it in verbose mode
    if args.verbose:
        print("  [startup output]")
    deadline = time.monotonic() + 0.5
    while time.monotonic() < deadline:
        try:
            label, line = q.get(timeout=0.05)
            if args.verbose and line is not None:
                tag = "OUT" if label == 'out' else "ERR"
                print(f"  [{tag}] {line}", flush=True)
        except Empty:
            pass

    # Enter xboard mode; suppress thinking lines for speed
    send(proc, "xboard", args.verbose)
    send(proc, "nopost", args.verbose)
    # Drain any feature lines the engine sends back in xboard mode
    drain(q, timeout=0.2)

    # ----------------------------------------------------------------
    # Self-play loop
    # ----------------------------------------------------------------
    wins = draws = losses = 0
    td_plies_list = []
    start_wall    = time.monotonic()

    for game_num in range(1, args.games + 1):

        elapsed_so_far = time.monotonic() - start_wall
        print(
            f"--- Game {game_num}/{args.games}"
            f"  (W={wins} D={draws} L={losses}"
            f"  elapsed={elapsed_so_far:.0f}s) ---",
            flush=True,
        )

        start_game(proc, args.tc, args.depth, args.verbose)

        # ---- play one game ----
        game_result   = None
        move_count    = 0
        game_start    = time.monotonic()
        last_progress = game_start
        # stderr lines seen during gameplay; TDLeaf (unbuffered stderr) can
        # arrive before the stdout result line is flushed from the pipe buffer,
        # so we save them here and check them first in wait_for_tdleaf().
        stderr_during_game = []

        while game_result is None:
            try:
                label, line = q.get(timeout=args.move_timeout)
            except Empty:
                print(
                    f"  TIMEOUT ({args.move_timeout:.0f}s) waiting for engine move"
                    f" after {move_count} moves — aborting game.",
                    flush=True,
                )
                game_result = "timeout"
                break

            if line is None:
                print(
                    f"  Engine stream closed unexpectedly after {move_count} moves.",
                    flush=True,
                )
                game_result = "error"
                break

            if args.verbose:
                tag = "OUT" if label == 'out' else "ERR"
                print(f"  [{tag}] {line}", flush=True)

            if label == 'err':
                stderr_during_game.append(line)
                continue

            if label == 'out':
                if MOVE_RE.search(line):
                    move_count += 1

                    # Periodic progress update every 10 moves
                    now = time.monotonic()
                    if move_count % 10 == 0 or (now - last_progress) >= 5.0:
                        elapsed_game = now - game_start
                        print(
                            f"  move {move_count:3d}  elapsed={elapsed_game:.1f}s",
                            flush=True,
                        )
                        last_progress = now

                    # Engine finished a move and is waiting for input.
                    # Send "go" to continue self-play.  If this was the
                    # last move of the game, the extra "go" is harmless
                    # (engine processes it with game.over=1 still set).
                    send(proc, "go", args.verbose)
                    continue

                m = RESULT_RE.search(line)
                if m:
                    game_result = m.group(1)
                    # Don't break immediately — engine still needs to
                    # process our last "go" and run TDLeaf.

            # Ignore unrecognised lines unless verbose (already printed above).

        if game_result in ("timeout", "error"):
            drain(q, timeout=1.0)
            continue

        # ---- wait for TDLeaf update ----
        plies, skipped = wait_for_tdleaf(
            q, args.tdleaf_timeout, args.verbose, stderr_during_game
        )

        # Drain any residual output before next game
        drain(q, timeout=0.1)

        # ---- tally ----
        if game_result == "1-0":
            wins   += 1
        elif game_result == "0-1":
            losses += 1
        else:
            draws  += 1

        total = wins + draws + losses

        if plies is not None:
            td_plies_list.append(plies)

        if plies is None:
            td_status = "TDLeaf: no confirmation (check --tdleaf-timeout or binary flags)"
        elif skipped:
            td_status = f"TDLeaf: skipped (only {plies} plies)"
        else:
            td_status = f"TDLeaf: updated {plies} plies"

        game_elapsed = time.monotonic() - game_start
        score_pct    = 100.0 * (wins + 0.5 * draws) / max(1, total)
        print(
            f"  Result: {game_result}  moves={move_count}  time={game_elapsed:.1f}s  "
            f"W={wins} D={draws} L={losses}  score={score_pct:.1f}%  {td_status}",
            flush=True,
        )

    # ----------------------------------------------------------------
    # Shut down engine
    # ----------------------------------------------------------------
    try:
        send(proc, "quit", args.verbose)
        proc.wait(timeout=5)
    except Exception:
        proc.kill()

    elapsed = time.monotonic() - start_wall
    total   = wins + draws + losses
    avg_len = sum(td_plies_list) / len(td_plies_list) if td_plies_list else 0.0

    print()
    print("=" * 60)
    print(f"Finished {args.games} games in {elapsed:.1f}s  "
          f"({elapsed / max(1, args.games):.1f}s/game)")
    print(f"Result:   W={wins}  D={draws}  L={losses}  "
          f"score={100.0*(wins+0.5*draws)/max(1,total):.1f}%")
    if td_plies_list:
        print(f"Avg ply count (TDLeaf): {avg_len:.1f}")
    print(f"Weights file: {weights_file}")


if __name__ == "__main__":
    main()
