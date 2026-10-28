#!/usr/bin/env python3
"""Refuse a native shim build against binaries whose private ABI was not inspected."""
import hashlib
from pathlib import Path
import sys

EXPECTED = {
    "bin/CPAAProxyEx": "5d6b652b7eeff8673b3428906fa59f57f1c63e5153dbb49eba3ef2e4fc6c47f0",
    "lib/libAirPlay.so": "c319336788701928673380d0869647583c9fa48345e4cc25a0e86f28cf35b0e5",
    "lib/libCoreUtils.so": "5d7cc40ee4638e17e0282a603be9022947507413bb79e0b39477d6fc2c803b61",
    # route guidance bridge: iAP2LinkRunLoopCreate argument order, iAP2LinkQueueSendData
    "lib/libiAP2Link.so": "04b6abbee63ccdaf8b2a799caf4e2941e8ebc04516f70dbff3f1f3462a57f44e",
}


def main():
    root = Path(sys.argv[1])
    for name, expected in EXPECTED.items():
        actual = hashlib.sha256((root / name).read_bytes()).hexdigest()
        if actual != expected:
            raise SystemExit(f"Native proxy ABI mismatch: {name}: {actual}; expected stock v129 {expected}")
    print("   native proxy ABI: inspected v129 CPAAProxyEx, AirPlay, CoreUtils and iAP2Link match")


if __name__ == "__main__":
    main()
