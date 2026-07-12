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
desktop_dir="$stage_root/usr/share/applications"
pixmaps_dir="$stage_root/usr/share/pixmaps"
hicolor_dir="$stage_root/usr/share/icons/hicolor/256x256/apps"
config_dir="$stage_root/etc/froxcomposband"
share_dir="$stage_root/usr/share/froxcomposband"

verify_generated_help_tree
rm -rf "$stage_root"
mkdir -p "$stage_root/DEBIAN" "$doc_dir" "$desktop_dir" "$pixmaps_dir" "$hicolor_dir"

make -C "$repo_root" DESTDIR="$stage_root" install

rm -rf "$config_dir" "$share_dir"
mkdir -p "$config_dir" "$share_dir"
rsync -a --delete --exclude='Makefile' --exclude='.deps' "$repo_root/lib/edit/" "$config_dir/edit/"
rsync -a --delete --exclude='Makefile' --exclude='.deps' "$repo_root/lib/pref/" "$config_dir/pref/"
rsync -a --delete --exclude='Makefile' --exclude='.deps' "$repo_root/lib/file/" "$share_dir/file/"
rsync -a --delete --exclude='Makefile' --exclude='.deps' "$repo_root/lib/help/" "$share_dir/help/"
rsync -a --delete --exclude='Makefile' --exclude='.deps' "$repo_root/lib/info/" "$share_dir/info/"
rsync -a --delete --exclude='Makefile' --exclude='.deps' "$repo_root/lib/xtra/" "$share_dir/xtra/"

cp "$repo_root/README.md" "$doc_dir/README.md"
install -m 644 "$repo_root/scripts/release/assets/froxcomposband.desktop" "$desktop_dir/froxcomposband.desktop"
install -m 644 "$repo_root/scripts/release/assets/froxcomposband.png" "$pixmaps_dir/froxcomposband.png"
install -m 644 "$repo_root/scripts/release/assets/froxcomposband.png" "$hicolor_dir/froxcomposband.png"

find "$stage_root" -type d -exec chmod 755 {} +
find "$stage_root" -type f -exec chmod 644 {} +
chmod 755 "$stage_root/usr/games/froxcomposband"

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

cat >"$stage_root/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -eu

log_dir="/var/log/froxcomposband"
log_file="$log_dir/install-test.log"
tmp_home="$(mktemp -d)"
status=0

mkdir -p "$log_dir"

if HOME="$tmp_home" XDG_DATA_HOME="$tmp_home/.local/share" \
    /usr/games/froxcomposband -T="$log_file" >/dev/null 2>&1; then
    status=0
else
    status=$?
fi

{
    printf '\nDebian post-install smoke status: %s\n' "$status"
    printf 'This diagnostic is intentionally non-blocking.\n'
} >>"$log_file" 2>/dev/null || true

rm -rf "$tmp_home"
exit 0
EOF
chmod 755 "$stage_root/DEBIAN/postinst"

dpkg-deb --root-owner-group --build "$stage_root" "$artifact_path" >/dev/null
printf '%s\n' "$artifact_path"
