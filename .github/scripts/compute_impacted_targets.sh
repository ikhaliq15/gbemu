#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 <workspace> <base-sha> <head-sha> <output-dir>" >&2
  exit 1
fi

WORKSPACE_DIR="$1"
BASE_SHA="$2"
HEAD_SHA="$3"
OUTPUT_DIR="$4"

BASE_WORKTREE="${OUTPUT_DIR}/base"
HEAD_WORKTREE="${OUTPUT_DIR}/head"
STARTING_HASHES="${OUTPUT_DIR}/starting-hashes.json"
FINAL_HASHES="${OUTPUT_DIR}/final-hashes.json"
IMPACTED_TARGETS="${OUTPUT_DIR}/impacted-targets.txt"

rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

cleanup() {
  git -C "${WORKSPACE_DIR}" worktree remove --force "${BASE_WORKTREE}" >/dev/null 2>&1 || true
  git -C "${WORKSPACE_DIR}" worktree remove --force "${HEAD_WORKTREE}" >/dev/null 2>&1 || true
}

trap cleanup EXIT

git -C "${WORKSPACE_DIR}" worktree add --detach "${BASE_WORKTREE}" "${BASE_SHA}"
git -C "${WORKSPACE_DIR}" worktree add --detach "${HEAD_WORKTREE}" "${HEAD_SHA}"

bazel run @bazel-diff//cli:bazel-diff -- \
  generate-hashes \
  --excludeExternalTargets \
  -w "${BASE_WORKTREE}" \
  -b bazel \
  "${STARTING_HASHES}"

bazel run @bazel-diff//cli:bazel-diff -- \
  generate-hashes \
  --excludeExternalTargets \
  -w "${HEAD_WORKTREE}" \
  -b bazel \
  "${FINAL_HASHES}"

bazel run @bazel-diff//cli:bazel-diff -- \
  get-impacted-targets \
  -w "${HEAD_WORKTREE}" \
  -b bazel \
  -sh "${STARTING_HASHES}" \
  -fh "${FINAL_HASHES}" \
  -tt Rule \
  -o "${IMPACTED_TARGETS}"
