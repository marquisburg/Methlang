# Scripts

## github_release.py

CLI tool to automate GitHub Pull Requests and Releases.

### Setup

1. Install dependencies:
   ```powershell
   pip install -r scripts/requirements.txt
   ```

2. Create a GitHub Personal Access Token with `repo` scope:
   - GitHub → Settings → Developer settings → Personal access tokens
   - Or use `gh auth token` if you have GitHub CLI

3. Set the token:
   ```powershell
   $env:GITHUB_TOKEN = "your_token_here"
   ```

### Usage

**Full workflow** (PR from development → main, then create release):
```powershell
python scripts/github_release.py --version 0.2.0
```

**PR only:**
```powershell
python scripts/github_release.py pr
```

**Release only** (after merging):
```powershell
python scripts/github_release.py release --version 0.2.0
```

**Dry run** (preview without making changes):
```powershell
python scripts/github_release.py --dry-run --version 0.2.0
```

### Options

| Option | Description |
|--------|-------------|
| `--head` | Source branch (default: development) |
| `--base` | Target branch (default: main) |
| `--version` | Release version, e.g. 0.2.0 (required for release) |
| `--pr-body` | PR description (default: uses PR_IR_IMPLEMENTATION.md or PR_DRAFT.md if present) |
| `--release-notes` | Release notes (default: auto-generated from commits) |
| `--release-target` | Branch/commit to tag for release (default: main) |
| `--dry-run` | Preview only, no API calls |

### Workflow

1. Work on `development` branch
2. Run `python scripts/github_release.py --version X.Y.Z`
3. Script creates PR from development → main
4. Script creates release with tag vX.Y.Z and auto-generated notes from commits
5. Merge the PR on GitHub when ready

If you prefer to release only after merging, run `pr` first, merge, then run `release`.
