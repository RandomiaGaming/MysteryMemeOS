#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

# Updates Musl by deleting the existing install and downloading/extracting a new one.
# Doesn't care if Musl was already downloaded it will always redownload.
rm -rf ./musl
curl https://musl.cc/x86_64-linux-musl-native.tgz -o ./musl.tgz
tar -xvf ./musl.tgz
rm ./musl.tgz
mv ./x86_64-linux-musl-native ./musl