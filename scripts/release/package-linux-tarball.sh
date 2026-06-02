#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=scripts/release/common.sh
source "$script_dir/common.sh"

tag="$(release_tag "${1:-}")"
artifact_dir="${ARTIFACT_DIR:-$repo_root/dist}"
staging_dir="$artifact_dir/froxcomposband-${tag}-linux-x86_64"
artifact_path="$artifact_dir/froxcomposband-${tag}-linux-x86_64.tar.gz"

verify_generated_help_tree
rm -rf "$staging_dir"
mkdir -p "$staging_dir"

install -m 755 "$repo_root/src/froxcomposband" "$staging_dir/froxcomposband"
install -m 644 "$repo_root/README.md" "$staging_dir/README.md"
rsync -a --delete --exclude='Makefile' --exclude='.deps' "$repo_root/lib/" "$staging_dir/lib/"

tar -C "$artifact_dir" -czf "$artifact_path" "$(basename "$staging_dir")"
printf '%s\n' "$artifact_path"
