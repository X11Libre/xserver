#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 --org ORG --repo REPO --branch BRANCH --keep N [--dry-run] [--debug]"
    exit 1
}

log_info() { echo "[INFO] $*"; }
log_warn() { echo "[WARN] $*" >&2; }
log_error() { echo "[ERROR] $*" >&2; }

ORG=""
REPO=""
BRANCH=""
KEEP=0
DRY_RUN=0
DEBUG=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --org) ORG="$2"; shift 2 ;;
        --repo) REPO="$2"; shift 2 ;;
        --branch) BRANCH="$2"; shift 2 ;;
        --keep) KEEP="$2"; shift 2 ;;
        --dry-run) DRY_RUN=1; shift ;;
        --debug) DEBUG=1; shift ;;
        *) usage ;;
    esac
done

if [[ -z "$ORG" || -z "$REPO" || -z "$BRANCH" || "$KEEP" -le 0 ]]; then
    usage
fi

if ! command -v gh >/dev/null 2>&1; then
    log_error "GitHub CLI 'gh' is required but not found"
    exit 1
fi

if [[ $DEBUG -eq 1 ]]; then
    set -x
fi

log_info "Fetching runs for $ORG/$REPO on branch $BRANCH ..."

runs=$(
    gh api \
        -H "Accept: application/vnd.github+json" \
        "/repos/$ORG/$REPO/actions/runs?branch=$BRANCH&per_page=100" \
        --jq '.workflow_runs[] | "\(.id)|\(.run_number)|\(.conclusion)|\(.created_at)"' |
        sort -t'|' -k2,2nr
)

mapfile -t entries <<<"$runs"

declare -A keep

# keep N most recent
count=0
for entry in "${entries[@]}"; do
    ((count++)) || true
    run_id="${entry%%|*}"
    if ((count <= KEEP)); then
        keep["$run_id"]=1
    fi
done

# helper to find neighbor entry (previous/next successful)
get_neighbor_entry() {
    local idx="$1"
    local direction="$2" # -1 for previous, +1 for next
    local new_idx=$((idx + direction))
    if ((new_idx >= 0 && new_idx < ${#entries[@]})); then
        echo "${entries[$new_idx]}"
    fi
}

# find broken runs and mark them + neighbors
for idx in "${!entries[@]}"; do
    IFS='|' read -r run_id number conclusion created <<<"${entries[$idx]}"
    jobs=$(gh run view "$run_id" -R "$ORG/$REPO" --json jobs \
               --template '{{range .jobs}}{{.conclusion}}{{"\n"}}{{end}}')
    if echo "$jobs" | grep -qE 'failure|cancelled|timed_out'; then
        keep["$run_id"]=1
        for dir in -1 1; do
            neighbor=$(get_neighbor_entry "$idx" "$dir" || true)
            if [[ -n "$neighbor" ]]; then
                IFS='|' read -r n_id n_num n_conc n_created <<<"$neighbor"
                if [[ "$n_conc" == "success" ]]; then
                    keep["$n_id"]=1
                fi
            fi
        done
    fi
done

# decide what to keep/delete
for entry in "${entries[@]}"; do
    IFS='|' read -r run_id number conclusion created <<<"$entry"
    if [[ -n "${keep[$run_id]:-}" ]]; then
        log_info "KEEP   run_id=$run_id number=$number status=$conclusion created=$created"
    else
        if [[ "$DRY_RUN" -eq 1 ]]; then
            log_info "Would delete run $run_id ($number)"
        else
            log_info "Deleting run $run_id ($number)"
            gh api --method DELETE -H "Accept: application/vnd.github+json" \
                "/repos/$ORG/$REPO/actions/runs/$run_id" || true
        fi
    fi
done
