#!/usr/bin/env python3
"""
Push a Zephyr/MCUboot-signed image to the board over the length-prefixed UDP
protocol in embedded-pio/updatemanager_eth/.

  START : 02 01 + <uint32 LE image length>   -> board erases slot, ACKs
  DATA  : N packets of 512 bytes             -> board writes exactly <length> bytes
  (no end marker; board finalizes on byte count, then reboots)

Stop-and-wait: each packet waits for a 1-byte ACK (0x00 = ok, non-zero = error).

Default image: build/customZephyrOTA/zephyr/zephyr.signed.bin
"""
import argparse
import socket
import struct
import sys
import time

PAYLOAD = 512
START_MARKER = bytes([0x02, 0x01])
DEFAULT_IMAGE = "build/customZephyrOTA/zephyr/zephyr.signed.bin"


def chunk_image(data: bytes, pad: int):
    for off in range(0, len(data), PAYLOAD):
        piece = data[off:off + PAYLOAD]
        if len(piece) < PAYLOAD:
            piece = piece + bytes([pad]) * (PAYLOAD - len(piece))
        yield off, piece


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image", nargs="?", default=DEFAULT_IMAGE, help=f"signed image (default {DEFAULT_IMAGE})")
    ap.add_argument("--ip", default="192.168.1.1", help="board address (MCU_IP_ADDR)")
    ap.add_argument("--port", type=int, default=12345, help="board port (LOCAL_PORT)")
    ap.add_argument("--bind", default=None, help="local IP to send from, e.g. 192.168.1.2")
    ap.add_argument("--pad", type=lambda s: int(s, 0), default=0xFF, help="fill byte for the final chunk (default 0xFF)")
    ap.add_argument("--warmup", type=float, default=1.5, help="seconds to wait before START (default 1.5)")
    ap.add_argument("--ack-timeout", type=float, default=15.0, help="seconds to wait for each ACK (default 15)")
    ap.add_argument("--dry-run", action="store_true", help="analyze, send nothing")
    args = ap.parse_args()

    try:
        with open(args.image, "rb") as f:
            image = f.read()
    except OSError as e:
        sys.exit(f"cannot read image: {e}")

    if not image:
        sys.exit("image is empty")
    if not (0 <= args.pad <= 0xFF):
        sys.exit("--pad must be a single byte 0..255")

    packets = list(chunk_image(image, args.pad))

    print(f"image      : {args.image}")
    print(f"size       : {len(image)} bytes")
    print(f"packets    : {len(packets)} x {PAYLOAD} B")
    print(f"dest       : {args.ip}:{args.port}" + (f"  from {args.bind}" if args.bind else ""))

    if args.dry_run:
        print("\ndry-run: nothing sent")
        return 0

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    bind_ip = args.bind if args.bind else ""
    sock.bind((bind_ip, args.port))
    sock.settimeout(args.ack_timeout)
    dest = (args.ip, args.port)

    def wait_ack(what: str):
        try:
            data, _ = sock.recvfrom(8)
        except socket.timeout:
            sys.exit(f"\nno ACK after {what} (>{args.ack_timeout:.0f}s) - aborting.")
        if data and data[0] != 0x00:
            sys.exit(f"\nboard reported error 0x{data[0]:02x} after {what} - aborting.")

    if args.warmup > 0:
        print(f"\nwarmup {args.warmup:.1f}s ...")
        time.sleep(args.warmup)

    t0 = time.time()
    sock.sendto(START_MARKER + struct.pack("<I", len(image)), dest)
    print(f"-> START   len={len(image)} (waiting for erase-done ACK, up to {args.ack_timeout:.0f}s)")
    wait_ack("START (erase)")
    print("<- ACK     erase done, streaming")

    sent = 0
    for i, (off, pkt) in enumerate(packets):
        sock.sendto(pkt, dest)
        wait_ack(f"data {i}")
        sent += len(pkt)
        if i % 32 == 0 or i == len(packets) - 1:
            print(f"-> data {i:4d}/{len(packets)}  off 0x{off:06x}")

    dt = time.time() - t0
    print(f"\nsent {len(image)} bytes in {dt:.1f}s. Board finalizes and reboots.")
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
