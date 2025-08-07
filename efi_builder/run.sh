#!/bin/bash

cd payload
./build.sh
cd ../

cd cpio
./phase2.sh
cd ../

cd testing
./testwithqemu.sh
cd ../