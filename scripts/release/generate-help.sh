#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=scripts/release/common.sh
source "$script_dir/common.sh"

binary_path="${1:-$repo_root/src/froxcomposband}"
export_dir="${2:-}"
tmp_home="$(mktemp -d "$repo_root/.release-home.XXXXXX")"
log_file="$repo_root/dist/generated-help.log"

cleanup() {
  rm -rf "$tmp_home"
}
trap cleanup EXIT

mkdir -p "$(dirname "$log_file")"
prepare_generated_help_dirs
rm -f "$repo_root/lib/edit/help_upd.txt"

env HOME="$tmp_home" TERM=xterm timeout 20s "$binary_path" -mgcu >"$log_file" 2>&1 || true

verify_generated_help_tree

if [[ -n "$export_dir" ]]; then
  copy_generated_help_export "$export_dir"
fi

printf 'Generated help files and stamped %s.\n' "V:$(runtime_version)"
