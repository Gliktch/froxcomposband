#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

generated_help_txt=(
  Classes.txt
  Demigods.txt
  Demons.txt
  Disciples.txt
  Draconians.txt
  DragonRealms.txt
  Dragons.txt
  MonsterRaces.txt
  Orcs.txt
  Personalities.txt
  Races.txt
  Warlocks.txt
  Weaponmasters.txt
)

generated_help_csv=(
  PossessorStats.csv
  Skills-Class.csv
  Skills-Racial.csv
  Spells.csv
)

release_tag() {
  local value="${RELEASE_TAG:-${1:-}}"
  if [[ -z "$value" ]]; then
    echo "RELEASE_TAG is required (for example: v7.2.6-beta)." >&2
    exit 1
  fi
  printf '%s\n' "$value"
}

release_version() {
  local tag
  tag="$(release_tag "${1:-}")"
  printf '%s\n' "${tag#v}"
}

runtime_version() {
  local major minor patch extra
  major="$(awk '/^#define VER_MAJOR /{print $3}' "$repo_root/src/defines.h")"
  minor="$(awk '/^#define VER_MINOR /{print $3}' "$repo_root/src/defines.h")"
  patch="$(awk '/^#define VER_PATCH /{gsub(/"/, "", $3); print $3}' "$repo_root/src/defines.h")"
  extra="$(awk '/^#define VER_EXTRA /{print $3}' "$repo_root/src/defines.h")"
  printf '%s.%s.%s.%s\n' "$major" "$minor" "$patch" "$extra"
}

prepare_generated_help_dirs() {
  mkdir -p "$repo_root/lib/help/text" "$repo_root/lib/help/html"
}

verify_generated_help_tree() {
  local expected_version
  expected_version="V:$(runtime_version)"

  if ! grep -qx "$expected_version" "$repo_root/lib/edit/help_upd.txt"; then
    echo "Expected $expected_version in lib/edit/help_upd.txt after help generation." >&2
    exit 1
  fi

  local file
  for file in "${generated_help_txt[@]}" "${generated_help_csv[@]}"; do
    if [[ ! -f "$repo_root/lib/help/$file" ]]; then
      echo "Missing generated help file: lib/help/$file" >&2
      exit 1
    fi
  done

  for file in faq.txt version.txt start.txt; do
    if [[ ! -f "$repo_root/lib/help/text/$file" ]]; then
      echo "Missing generated text help file: lib/help/text/$file" >&2
      exit 1
    fi
  done

  for file in faq.html version.html start.html; do
    if [[ ! -f "$repo_root/lib/help/html/$file" ]]; then
      echo "Missing generated HTML help file: lib/help/html/$file" >&2
      exit 1
    fi
  done
}

copy_generated_help_export() {
  local export_dir="$1"
  local file

  rm -rf "$export_dir"
  mkdir -p "$export_dir/lib/help" "$export_dir/lib/edit"

  for file in "${generated_help_txt[@]}" "${generated_help_csv[@]}"; do
    cp "$repo_root/lib/help/$file" "$export_dir/lib/help/$file"
  done

  cp -R "$repo_root/lib/help/text" "$export_dir/lib/help/"
  cp -R "$repo_root/lib/help/html" "$export_dir/lib/help/"
  cp "$repo_root/lib/edit/help_upd.txt" "$export_dir/lib/edit/help_upd.txt"
}

apply_generated_help_export() {
  local export_dir="$1"
  local file

  mkdir -p "$repo_root/lib/help" "$repo_root/lib/edit"

  for file in "${generated_help_txt[@]}" "${generated_help_csv[@]}"; do
    cp "$export_dir/lib/help/$file" "$repo_root/lib/help/$file"
  done

  rm -rf "$repo_root/lib/help/text" "$repo_root/lib/help/html"
  mkdir -p "$repo_root/lib/help/text" "$repo_root/lib/help/html"
  cp -R "$export_dir/lib/help/text/." "$repo_root/lib/help/text/"
  cp -R "$export_dir/lib/help/html/." "$repo_root/lib/help/html/"
  cp "$export_dir/lib/edit/help_upd.txt" "$repo_root/lib/edit/help_upd.txt"
}
