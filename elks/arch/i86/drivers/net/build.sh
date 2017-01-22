#!/bin/bash

# Build binaries for target

as86 -0 -o ne2k-phy.o ne2k-phy.s

ld86 -0 -d -M -o ne2k.bin ne2k-phy.o > ne2k.map
