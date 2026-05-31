#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--dry-run] [--workflow <name>] <owner/repo>"
    echo
    echo "Arguments:"
    echo "  --dry-run          Only show which runs would be deleted (no deletion)"
    echo "  --workflow <name>  Only purge runs from this workflow name (optional)"
    echo "  owner/repo         GitHub repository in 'owner/repo' format"
    exit 1
}

log_info() { echo "[INFO] $*"; }
log_warn() { echo "[WARN] $*" >&2; }
log_error() { echo "[ERROR] $*" >&2; exit 1; }

DRYRUN=false
WORKFLOW_FILTER=""

# Parse options
while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRYRUN=true
            shift
            ;;
        --workflow)
            if [[ $# -lt 2 ]]; then
                usage
            fi
            WORKFLOW_FILTER=$2
            shift 2
            ;;
        *)
            if [[ -z "${REPO:-}" ]]; then
                REPO=$1
                shift
            else
                usage
            fi
            ;;
    esac
done

if [[ -z "${REPO:-}" ]]; then
    usage
fi

if ! command -v gh >/dev/null 2>&1; then
    log_error "GitHub CLI (gh) is required but not installed."
fi
if ! command -v jq >/dev/null 2>&1; then
    log_error "jq is required but not installed."
fi

# Ensure user is logged in
if ! gh auth status >/dev/null 2>&1; then
    log_error "You are not logged in to GitHub CLI. Run: gh auth login"
fi

# Get all existing branches
log_info "Fetching existing branches..."
existing_branches=$(gh api -H "Accept: application/vnd.github+json" \
    "/repos/${REPO}/branches?per_page=100" --paginate \
    | jq -r '.[].name' | sort -u)

if [[ -z "$existing_branches" ]]; then
    log_warn "No branches found in repo. Aborting."
    exit 0
fi
log_info "Found $(echo "$existing_branches" | wc -l) branches."

# Fetch workflow runs
log_info "Fetching workflow runs..."
gh api -H "Accept: application/vnd.github+json" \
    "/repos/${REPO}/actions/runs?per_page=100" --paginate \
    | jq -c '.workflow_runs[] | {id: .id, branch: .head_branch, workflow: .name}' \
    | while read -r run; do
        run_id=$(echo "$run" | jq -r '.id')
        run_branch=$(echo "$run" | jq -r '.branch')
        workflow_name=$(echo "$run" | jq -r '.workflow')

        if [[ -z "$run_branch" ]]; then
            log_warn "Run $run_id has no branch (skipping)."
            continue
        fi

        # Skip if branch exists
        if echo "$existing_branches" | grep -qx "$run_branch"; then
            continue
        fi

        # Skip if workflow filter is set and does not match
        if [[ -n "$WORKFLOW_FILTER" && "$workflow_name" != "$WORKFLOW_FILTER" ]]; then
            continue
        fi

        if [[ "$DRYRUN" == true ]]; then
            log_info "[DRY-RUN] Would delete run $run_id (branch: $run_branch, workflow: $workflow_name)"
        else
            log_info "Deleting run $run_id (branch: $run_branch, workflow: $workflow_name)"
            gh api -X DELETE \
                "/repos/${REPO}/actions/runs/${run_id}" >/dev/null || true
        fi
    done

if [[ "$DRYRUN" == true ]]; then
    log_info "Dry-run finished. No runs were deleted."
else
    log_info "Finished purging workflow runs for deleted branches."
fi
