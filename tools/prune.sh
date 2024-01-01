#!/usr/bin/env bash

# Prune the cross tools to save disk space

# This script is an entry point for the workflows
# See .github/workflow/*.yml

SCRIPTDIR="$(dirname "$0")"
. "$SCRIPTDIR/../env.sh"
make -C "$SCRIPTDIR" prune
