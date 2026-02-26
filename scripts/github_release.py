#!/usr/bin/env python3
"""
GitHub Release Automation Script

Creates Pull Requests from development -> main, then creates GitHub releases
with version tags and release notes.

Usage:
  python github_release.py                    # Full workflow: PR then release
  python github_release.py pr                 # Create PR only
  python github_release.py release            # Create release only
  python github_release.py --dry-run          # Preview without making changes

Requires: GITHUB_TOKEN environment variable (or --token)
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

# Optional: use requests if available, else fall back to urllib
try:
    import requests
    HAS_REQUESTS = True
except ImportError:
    HAS_REQUESTS = False

# GitHub API base
GITHUB_API = "https://api.github.com"


def run_git(*args: str, cwd: Path | None = None) -> str:
    """Run a git command and return stdout."""
    result = subprocess.run(
        ["git"] + list(args),
        capture_output=True,
        text=True,
        cwd=cwd,
    )
    if result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed: {result.stderr}")
    return result.stdout.strip()


def get_repo_info(repo_root: Path) -> tuple[str, str]:
    """Parse owner/repo from git remote origin."""
    url = run_git("config", "--get", "remote.origin.url", cwd=repo_root)
    # Handle both https and ssh URLs
    match = re.search(r"github\.com[:/]([^/]+)/([^/.]+)(?:\.git)?$", url)
    if not match:
        raise RuntimeError(f"Could not parse GitHub repo from remote: {url}")
    owner, repo = match.groups()
    return owner, repo


def get_current_branch(repo_root: Path) -> str:
    """Get current branch name."""
    return run_git("rev-parse", "--abbrev-ref", "HEAD", cwd=repo_root)


def get_commits_ahead(base: str, head: str, repo_root: Path) -> list[str]:
    """Get commit messages between base and head."""
    out = run_git(
        "log", f"{base}..{head}", "--pretty=format:%s", "--no-merges",
        cwd=repo_root,
    )
    return [line for line in out.split("\n") if line.strip()]


def http_request(
    method: str,
    url: str,
    token: str,
    *,
    json_data: dict | None = None,
) -> dict | list:
    """Make an authenticated HTTP request to GitHub API."""
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }

    if HAS_REQUESTS:
        resp = requests.request(
            method,
            url,
            headers=headers,
            json=json_data,
            timeout=30,
        )
        resp.raise_for_status()
        if resp.text:
            return resp.json()
        return {}

    # Fallback: urllib
    import urllib.request
    import urllib.error

    req = urllib.request.Request(url, method=method, headers=headers)
    if json_data:
        req.data = json.dumps(json_data).encode("utf-8")
        req.add_header("Content-Type", "application/json")

    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            data = r.read().decode("utf-8")
            return json.loads(data) if data else {}
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8") if e.fp else ""
        raise RuntimeError(f"GitHub API error {e.code}: {body}")


def create_pull_request(
    owner: str,
    repo: str,
    token: str,
    *,
    head: str = "development",
    base: str = "main",
    title: str | None = None,
    body: str | None = None,
    dry_run: bool = False,
) -> dict | None:
    """Create a PR from head branch to base branch."""
    url = f"{GITHUB_API}/repos/{owner}/{repo}/pulls"

    # Check for existing PR (skip if dry run - no token needed for preview)
    if not dry_run:
        list_url = f"{GITHUB_API}/repos/{owner}/{repo}/pulls?head={owner}:{head}&base={base}&state=open"
        existing = http_request("GET", list_url, token)
        if isinstance(existing, list) and len(existing) > 0:
            pr = existing[0]
            print(f"  -> PR already exists: #{pr['number']} {pr['html_url']}")
            return pr

    payload = {
        "title": title or f"Merge {head} into {base}",
        "head": head,
        "base": base,
        "body": body or f"Automated PR from `{head}` to `{base}`.",
    }

    if dry_run:
        print("  [DRY RUN] Would create PR:")
        print(f"    Title: {payload['title']}")
        print(f"    Head: {head} -> Base: {base}")
        return None

    pr = http_request("POST", url, token, json_data=payload)
    print(f"  -> Created PR #{pr['number']}: {pr['html_url']}")
    return pr


def create_release(
    owner: str,
    repo: str,
    token: str,
    version: str,
    *,
    body: str | None = None,
    target: str = "main",
    dry_run: bool = False,
) -> dict | None:
    """Create a GitHub release with tag."""
    tag = f"v{version}" if not version.startswith("v") else version

    # Check if tag exists (skip if dry run)
    if not dry_run:
        tag_url = f"{GITHUB_API}/repos/{owner}/{repo}/releases/tags/{tag}"
        try:
            existing = http_request("GET", tag_url, token)
            if existing and existing.get("id"):
                print(f"  -> Release {tag} already exists: {existing.get('html_url', '')}")
                return existing
        except Exception:
            pass  # 404 or other error - release doesn't exist, proceed to create

    url = f"{GITHUB_API}/repos/{owner}/{repo}/releases"
    payload = {
        "tag_name": tag,
        "target_commitish": target,
        "name": tag,
        "body": body or f"Release {tag}",
        "draft": False,
        "prerelease": False,
    }

    if dry_run:
        print("  [DRY RUN] Would create release:")
        print(f"    Tag: {tag}")
        print(f"    Target: {target}")
        print(f"    Body preview: {(body or '')[:200]}...")
        return None

    release = http_request("POST", url, token, json_data=payload)
    print(f"  -> Created release {tag}: {release.get('html_url', '')}")
    return release


def load_pr_body(repo_root: Path) -> str | None:
    """Load PR body from PR_*.md or similar files in repo root."""
    candidates = [
        repo_root / "PR_IR_IMPLEMENTATION.md",
        repo_root / "PR_DRAFT.md",
        repo_root / "PULL_REQUEST.md",
    ]
    for p in candidates:
        if p.exists():
            return p.read_text(encoding="utf-8", errors="replace")
    return None


def generate_release_notes(repo_root: Path, base: str, head: str) -> str:
    """Generate release notes from commits."""
    commits = get_commits_ahead(base, head, repo_root)
    if not commits:
        return "No commits in this release."

    lines = ["## Changes\n", ""]
    for msg in commits:
        lines.append(f"- {msg}")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="GitHub automation: Create PR from development -> main, then create release.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "action",
        nargs="?",
        choices=["pr", "release", "all"],
        default="all",
        help="Action: pr (create PR only), release (create release only), all (both, default)",
    )
    parser.add_argument(
        "--token",
        default=os.environ.get("GITHUB_TOKEN"),
        help="GitHub token (default: GITHUB_TOKEN env)",
    )
    parser.add_argument(
        "--head",
        default="development",
        help="Source branch for PR (default: development)",
    )
    parser.add_argument(
        "--base",
        default="main",
        help="Target branch for PR and release (default: main)",
    )
    parser.add_argument(
        "--version",
        help="Release version (e.g. 0.2.0). Required for release action.",
    )
    parser.add_argument(
        "--pr-title",
        help="PR title (default: Merge {head} into {base})",
    )
    parser.add_argument(
        "--pr-body",
        help="PR body. If not set, uses PR_IR_IMPLEMENTATION.md or PR_DRAFT.md if present.",
    )
    parser.add_argument(
        "--release-notes",
        help="Release notes. If not set, auto-generated from commits.",
    )
    parser.add_argument(
        "--release-target",
        help="Commitish for release tag (default: same as --base)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Preview actions without making changes",
    )
    parser.add_argument(
        "--repo",
        default=".",
        help="Path to git repo (default: current directory)",
    )

    args = parser.parse_args()
    repo_root = Path(args.repo).resolve()

    if not args.token and not args.dry_run:
        print("Error: GITHUB_TOKEN not set. Set the env var or use --token.", file=sys.stderr)
        return 1

    try:
        owner, repo = get_repo_info(repo_root)
    except RuntimeError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print(f"Repository: {owner}/{repo}")
    if args.dry_run:
        print("  [DRY RUN MODE - no changes will be made]\n")

    # 1. Create Pull Request
    if args.action in ("pr", "all"):
        print("\n1. Creating Pull Request...")
        pr_body = args.pr_body or load_pr_body(repo_root)
        create_pull_request(
            owner,
            repo,
            args.token,
            head=args.head,
            base=args.base,
            title=args.pr_title,
            body=pr_body,
            dry_run=args.dry_run,
        )

    # 2. Create Release
    if args.action in ("release", "all"):
        print("\n2. Creating Release...")
        if not args.version and args.action == "all":
            # Try to infer version from tags or VERSION file
            version_file = repo_root / "VERSION"
            if version_file.exists():
                args.version = version_file.read_text().strip()
            else:
                print("  Error: --version required for release. Use --version 0.2.0", file=sys.stderr)
                return 1
        elif not args.version:
            print("  Error: --version required for release action.", file=sys.stderr)
            return 1

        release_target = args.release_target or args.base
        release_notes = args.release_notes or generate_release_notes(
            repo_root, args.base, args.head
        )
        create_release(
            owner,
            repo,
            args.token,
            args.version,
            body=release_notes,
            target=release_target,
            dry_run=args.dry_run,
        )

    print("\nDone.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
