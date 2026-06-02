#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=scripts/release/common.sh
source "$script_dir/common.sh"

tag="$(release_tag "${1:-}")"
version="$(release_version "$tag")"
artifact_dir="${ARTIFACT_DIR:-$repo_root/dist}"
dist_name="FroxComposband-${version}-osx.dmg"
src_artifact="$repo_root/$dist_name"
artifact_path="$artifact_dir/froxcomposband-${tag}-macos.dmg"

verify_generated_help_tree
rm -f "$src_artifact" "$artifact_path"

make -C "$repo_root/src" -f Makefile.osx clean
make -C "$repo_root/src" -f Makefile.osx dist DIST_VERSION="$version"

mv "$src_artifact" "$artifact_path"
printf '%s\n' "$artifact_path"
