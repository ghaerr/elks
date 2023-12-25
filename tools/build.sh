#!/usr/bin/env bash

# Build the cross tools for the IA16 target

# This script is an entry point for the workflows
# See .github/workflow/*.yml

SCRIPTDIR="$(dirname "$0")"
. "$SCRIPTDIR/../env.sh"
make -C "$SCRIPTDIR" all
