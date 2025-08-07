#!/bin/bash
set -euo pipefail
cd "$(dirname "$(realpath "$0")")"

rm -rf ./musl
curl https://musl.cc/x86_64-linux-musl-native.tgz -o ./musl.tgz
tar -xvf ./musl.tgz
rm ./musl.tgz
mv ./x86_64-linux-musl-native ./musl