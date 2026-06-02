#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=scripts/release/common.sh
source "$script_dir/common.sh"

tag="$(release_tag "${1:-}")"
version="$(release_version "$tag")"
artifact_dir="${ARTIFACT_DIR:-$repo_root/dist}"
stage_root="$artifact_dir/deb-stage"
artifact_path="$artifact_dir/froxcomposband-${tag}-linux-x86_64.deb"
doc_dir="$stage_root/usr/share/doc/froxcomposband"
var_dir="$stage_root/var/games/froxcomposband"
desktop_dir="$stage_root/usr/share/applications"
pixmaps_dir="$stage_root/usr/share/pixmaps"
hicolor_dir="$stage_root/usr/share/icons/hicolor/256x256/apps"

verify_generated_help_tree
rm -rf "$stage_root"
mkdir -p "$stage_root/DEBIAN" "$doc_dir" "$var_dir" "$desktop_dir" "$pixmaps_dir" "$hicolor_dir"

make -C "$repo_root" DESTDIR="$stage_root" install

mkdir -p "$stage_root/usr/share/froxcomposband/help"
cp -R "$repo_root/lib/help/text" "$stage_root/usr/share/froxcomposband/help/"
cp -R "$repo_root/lib/help/html" "$stage_root/usr/share/froxcomposband/help/"

cp "$repo_root/README.md" "$doc_dir/README.md"
install -m 644 "$repo_root/scripts/release/assets/froxcomposband.desktop" "$desktop_dir/froxcomposband.desktop"
install -m 644 "$repo_root/scripts/release/assets/froxcomposband.png" "$pixmaps_dir/froxcomposband.png"
install -m 644 "$repo_root/scripts/release/assets/froxcomposband.png" "$hicolor_dir/froxcomposband.png"

cat >"$stage_root/DEBIAN/control" <<EOF
Package: froxcomposband
Version: $version
Section: games
Priority: optional
Architecture: amd64
Maintainer: FroxComposband maintainers <noreply@ask.band>
Depends: libc6, libncursesw6
Description: Single-player roguelike dungeon crawler
 FroxComposband is a single-player roguelike dungeon crawler descended
 from Angband, Moria, and FrogComposband.
EOF

dpkg-deb --build "$stage_root" "$artifact_path" >/dev/null
printf '%s\n' "$artifact_path"
