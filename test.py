#!/usr/bin/env python3
"""
Push a Zephyr/MCUboot-signed image to the board over the makeshift UDP protocol
implemented in embedded-pio/updatemanager_eth/.

Firmware FSM (updatemanager_eth.cpp), buffer = CONFIG_IMG_BLOCK_BUF_SIZE = 512:

  WAIT_FOR_ETH_UPDATE : one packet starting  02 01  -> UPDATING_WITH_ETH
  UPDATING_WITH_ETH   : every packet is written to slot1 via flash_img;
                        a packet starting  01 02  -> ETH_UPDATE_DONE
  ETH_UPDATE_DONE     : flush, boot_request_upgrade(TEST), reboot

Wire protocol this script sends:

  1. START marker      02 01                       (2 bytes, sent once)
  2. N data packets    exactly 512 bytes each      (last one tail-padded)
  3. END marker        01 02                        (2 bytes)

The image goes to slot1 exactly as-is; the last chunk is padded ONLY in its
trailing bytes (past the real image), which MCUboot ignores. No chunk is padded
in the middle - that would shift the image and break the signature.

NO FRAMING PROTECTION: a data packet whose first two bytes are the END marker
(01 02) would be misread as end-of-transfer and truncate the image. This script
scans for that before sending and refuses unless --force is given.

Default image: build/customZephyrOTA/zephyr/zephyr.signed.bin  (the signed
image linked for slot0; written to slot1, swapped in by MCUboot). Do NOT send
zephyr.bin (unsigned) or the .hex.
"""
import argparse
import os
import socket
import sys
import time

PAYLOAD = 512                      # must equal CONFIG_IMG_BLOCK_BUF_SIZE
START_MARKER = bytes([0x02, 0x01])
END_MARKER = bytes([0x01, 0x02])
DEFAULT_IMAGE = "build/customZephyrOTA/zephyr/zephyr.signed.bin"


def chunk_image(data: bytes, pad: int):
    """Yield 512-byte packets; only the final one is tail-padded."""
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
    ap.add_argument("--bind", default=None,
                    help="local IP to send from, e.g. 192.168.1.2 (forces the right NIC)")
    ap.add_argument("--pad", type=lambda s: int(s, 0), default=0xFF,
                    help="fill byte for the final chunk's trailing bytes (default 0xFF)")
    ap.add_argument("--warmup", type=float, default=1.5,
                    help="seconds to wait before the START marker, so the board finishes "
                         "Ethernet init and has armed its receiver (default 1.5)")
    ap.add_argument("--ack-timeout", type=float, default=15.0,
                    help="seconds to wait for each ACK. Must exceed the board's slot erase (the "
                         "post-START ACK arrives only after that erase completes) (default 15)")
    ap.add_argument("--force", action="store_true",
                    help="send even if a data chunk collides with the END marker (01 02)")
    ap.add_argument("--dry-run", action="store_true", help="analyze and check collisions, send nothing")
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
    full = sum(1 for _, p in packets if p == image[_:_ + PAYLOAD])  # cosmetic
    last_real = len(image) - (len(packets) - 1) * PAYLOAD
    pad_bytes = PAYLOAD - last_real if last_real < PAYLOAD else 0

    print(f"image      : {args.image}")
    print(f"size       : {len(image)} bytes")
    print(f"packets    : {len(packets)} x {PAYLOAD} B  (last chunk {last_real} real + {pad_bytes} pad = 0x{args.pad:02X})")
    print(f"dest       : {args.ip}:{args.port}" + (f"  from {args.bind}" if args.bind else ""))

    # Preflight: no data chunk may start with the END marker (no framing protection).
    collisions = [off for off, p in packets if p[:2] == END_MARKER]
    if collisions:
        print(f"\n!! {len(collisions)} data chunk(s) start with the END marker {END_MARKER.hex(' ')} "
              f"at image offset(s): {', '.join(hex(o) for o in collisions[:8])}"
              + (" ..." if len(collisions) > 8 else ""))
        print("   The firmware would treat the first as end-of-transfer and truncate the image.")
        print("   Rebuild (the bytes change) or pass --force to send anyway (image will fail validation).")
        if not args.force:
            return 1

    # Also flag a data chunk that looks like the START marker - harmless in UPDATING,
    # but worth knowing if you ever move the check.
    start_lookalikes = sum(1 for off, p in packets if p[:2] == START_MARKER)
    if start_lookalikes:
        print(f"   (note: {start_lookalikes} data chunk(s) start with {START_MARKER.hex(' ')}; "
              f"harmless once past WAIT)")

    if args.dry_run:
        print("\ndry-run: nothing sent")
        return 0

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # Bind to the same port we send to, so the board's ACK (sent to its connected
    # peer <bind>:<port>) lands back on this socket.
    bind_ip = args.bind if args.bind else ""
    sock.bind((bind_ip, args.port))
    sock.settimeout(args.ack_timeout)
    dest = (args.ip, args.port)

    def wait_ack(what: str):
        try:
            data, _ = sock.recvfrom(8)
        except socket.timeout:
            sys.exit(f"\nno ACK after {what} (>{args.ack_timeout:.0f}s) - aborting. "
                     f"Board may not have finished erasing, or the link/ports are wrong.")
        if data and data[0] != 0x00:
            sys.exit(f"\nboard reported error 0x{data[0]:02x} after {what} - aborting "
                     f"(likely a flash write failure; image is incomplete).")

    if args.warmup > 0:
        print(f"\nwarmup {args.warmup:.1f}s (let the board arm its receiver) ...")
        time.sleep(args.warmup)

    t0 = time.time()
    sock.sendto(START_MARKER, dest)
    print(f"-> START   {START_MARKER.hex(' ')}   (waiting for erase-done ACK, up to {args.ack_timeout:.0f}s)")
    wait_ack("START (erase)")           # blocks through the slot erase - no fixed gap needed
    print("<- ACK     erase done, streaming")

    sent = 0
    for i, (off, pkt) in enumerate(packets):
        sock.sendto(pkt, dest)
        wait_ack(f"data {i}")           # stop-and-wait: one packet per ACK, never overruns
        sent += len(pkt)
        if i % 32 == 0 or i == len(packets) - 1:
            print(f"-> data {i:4d}/{len(packets)}  off 0x{off:06x}  ({sent}/{len(packets)*PAYLOAD} B)")

    sock.sendto(END_MARKER, dest)
    dt = time.time() - t0
    print(f"-> END     {END_MARKER.hex(' ')}")
    print(f"\nsent {len(packets)} data packets ({sent} B, {len(image)} real) in {dt:.1f}s. "
          f"Board should flush, request TEST upgrade, and reboot.")
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
