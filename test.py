#!/usr/bin/env python3
"""
Drive the zephyrupdate state machine over UDP.

The firmware FSM (zephyrupdateethernet.cpp):

  WAIT_FOR_ETH_UPDATE : accepts one packet whose first two bytes are the
                        --init header (default 02 01), then -> UPDATING_WITH_ETH
  UPDATING_WITH_ETH   : counts payload bytes, logs "Saw 512 bytes" at 512

  1. send the init header packet  [--init]      -> arms UPDATING_WITH_ETH
  2. send N data packets of --chunk bytes each  -> board counts bytes, logs at 512

Each data packet is filled with its own sequence number so the board's
hexdump is legible: packet 0 = 00 00 00..., packet 1 = 01 01 01...

NOTE ON THE FIRMWARE DOUBLE-READ: in UPDATING_WITH_ETH the task calls
getNextPacket() twice per loop (top of loop + inside the case), so it
CONSUMES two packets but COUNTS only one. Until that bug is fixed, the board
needs 2x the data packets to reach 512 counted bytes. --match-firmware
doubles the packet count so "Saw 512 bytes" actually fires against the code
as written; drop it once the task reads once per iteration.
"""
import argparse
import socket
import sys
import time


def parse_hex_bytes(s: str) -> bytes:
    """Accept '02 01', '0201', or '0x02,0x01' -> b'\\x02\\x01'."""
    cleaned = s.replace("0x", "").replace(",", " ").strip()
    if " " in cleaned:
        return bytes(int(tok, 16) for tok in cleaned.split())
    if len(cleaned) % 2 != 0:
        raise ValueError("hex string needs an even number of digits")
    return bytes.fromhex(cleaned)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ip", default="192.168.1.1", help="board address (MCU_IP_ADDR)")
    ap.add_argument("--port", type=int, default=12345, help="board port (LOCAL_PORT)")
    ap.add_argument("--bind", default=None,
                    help="local IP to send from, e.g. 192.168.1.2 (forces the right NIC on multi-homed hosts)")
    ap.add_argument("--init", default="02 01",
                    help="init header bytes the FSM waits for (hex). Default '02 01'.")
    ap.add_argument("--chunk", type=int, default=64, help="bytes per data packet")
    ap.add_argument("--packets", type=int, default=None,
                    help="number of data packets (default: enough to reach --total)")
    ap.add_argument("--total", type=int, default=512, help="bytes the board needs to see (ETHERNET_BUFFER_SIZE)")
    ap.add_argument("--gap", type=float, default=0.02,
                    help="seconds between packets; keeps the board's 10-slot FIFO from overflowing")
    ap.add_argument("--match-firmware", action="store_true",
                    help="double the data packets to compensate for the UPDATING double-read bug")
    ap.add_argument("--no-init", action="store_true",
                    help="skip the init header packet (board already in UPDATING_WITH_ETH)")
    args = ap.parse_args()

    if args.chunk < 1 or args.chunk > 512:
        sys.exit("--chunk must be 1..512 (board reads into a 512-byte buffer)")

    try:
        init = parse_hex_bytes(args.init)
    except ValueError as e:
        sys.exit(f"--init: {e}")
    if len(init) < 2:
        sys.exit("--init needs at least 2 bytes; the FSM checks buffer[0] and buffer[1]")

    n_packets = args.packets if args.packets is not None else -(-args.total // args.chunk)  # ceil
    if args.match_firmware:
        n_packets *= 2

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if args.bind:
        sock.bind((args.bind, 0))
    dest = (args.ip, args.port)

    if not args.no_init:
        sock.sendto(init, dest)
        print(f"-> init    {init.hex(' ')}   (arms UPDATING_WITH_ETH)")
        time.sleep(args.gap)

    sent = 0
    for i in range(n_packets):
        payload = bytes([i & 0xFF]) * args.chunk
        sock.sendto(payload, dest)
        sent += len(payload)
        print(f"-> data #{i:<3} {len(payload):3d} B  (running total {sent:4d})  first bytes: {payload[:4].hex(' ')} ...")
        time.sleep(args.gap)

    print(f"\nsent init [{init.hex(' ')}] + {n_packets} data packets, {sent} data bytes, to {dest[0]}:{dest[1]}")
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())