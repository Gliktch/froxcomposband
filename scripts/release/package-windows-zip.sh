#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=scripts/release/common.sh
source "$script_dir/common.sh"

tag="$(release_tag "${1:-}")"
artifact_dir="${ARTIFACT_DIR:-$repo_root/dist}"
staging_dir="$artifact_dir/windows-package"
artifact_path="$artifact_dir/froxcomposband-${tag}-windows-x86_64.zip"

verify_generated_help_tree
rm -rf "$staging_dir"
mkdir -p "$staging_dir"

cp "$repo_root/src/froxcomposband.exe" "$staging_dir/froxcomposband.exe"
cp "$repo_root/README.md" "$staging_dir/README.md"

find "$repo_root/lib" -name Makefile -prune -o -name .deps -prune -o -type f -print0 | while IFS= read -r -d '' file; do
  rel="${file#$repo_root/}"
  mkdir -p "$staging_dir/$(dirname "$rel")"
  cp "$file" "$staging_dir/$rel"
done

(
  cd "$staging_dir"
  rm -f "$artifact_path"
  zip -q -r "$artifact_path" .
)

printf '%s\n' "$artifact_path"
