#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=scripts/release/common.sh
source "$script_dir/common.sh"

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <generated-help-export-dir>" >&2
  exit 1
fi

apply_generated_help_export "$1"
verify_generated_help_tree
