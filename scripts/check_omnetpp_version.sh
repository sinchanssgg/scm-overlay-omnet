#!/usr/bin/env bash
# check_omnetpp_version.sh — Compare pinned submodule vs upstream OMNeT++ tags
# Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
submodule_dir="$repo_root/third_party/omnetpp"
upstream_url="https://github.com/omnetpp/omnetpp"

if [[ ! -d "$submodule_dir/.git" && ! -f "$submodule_dir/.git" ]]; then
  echo "OMNeT++ submodule not found at third_party/omnetpp"
  echo "Run: git submodule update --init --recursive"
  exit 1
fi

current_commit="$(git -C "$submodule_dir" rev-parse HEAD)"
current_tag="$(git -C "$submodule_dir" describe --tags --exact-match 2>/dev/null || true)"

latest_tag="$(git ls-remote --tags "$upstream_url" \
  | awk '{print $2}' \
  | sed 's#refs/tags/##' \
  | grep '^omnetpp-[0-9]\+\.[0-9]\+\.[0-9]\+$' \
  | sort -V \
  | tail -n 1)"

latest_commit="$(git ls-remote --tags "$upstream_url" "refs/tags/$latest_tag" | awk '{print $1}')"

echo "OMNeT++ Version Status"
echo "- Submodule path : third_party/omnetpp"
echo "- Current commit : $current_commit"

if [[ -n "$current_tag" ]]; then
  echo "- Current tag    : $current_tag"
else
  echo "- Current tag    : (not exactly on a tag)"
fi

echo "- Latest upstream: $latest_tag"
echo "- Latest commit  : $latest_commit"

if [[ "$current_commit" == "$latest_commit" ]]; then
  echo "Status: Up to date with latest upstream tag."
else
  echo "Status: Newer upstream tag available or pinned to different commit."
fi
