#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

./cpio.sh
./efi.sh
./test.sh