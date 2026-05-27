# Releasing AwakeGuard

Use this checklist when you ship a new version. The process is **tag-driven**:
the git tag on the commit you release (`v0.1.1`) is the version stamped into
every binary and the GitHub Release.

## Quick checklist

- [ ] Changes merged to `master` and CI green (if you run checks on PRs)
- [ ] Pick the next version (`vMAJOR.MINOR.PATCH`) — see [Version numbers](#version-numbers)
- [ ] Tag that commit on `master` and push the tag
- [ ] Fast-forward `release` to the same commit and push `release`
- [ ] Wait for [Release workflow](.github/workflows/release.yml) to finish
- [ ] Spot-check the GitHub Release page and one artifact

## Branches and CI

| Branch    | Purpose |
| --------- | ------- |
| `master`  | Day-to-day development |
| `release` | Deploy branch — **pushing here starts a release build** |

Pushing to `release` runs `.github/workflows/release.yml`, which:

1. Requires an **exact** version tag on `HEAD` (e.g. `v0.1.1`)
2. Builds all **nine** artifacts (Win32 + WPF framework-dependent + WPF self-contained × x64/x86/arm64)
3. Publishes a GitHub Release on that tag named `AwakeGuard X.Y.Z`

The workflow **fails** if:

- `HEAD` has no tag
- The tag is not `vMAJOR.MINOR.PATCH` (must start with `v`, three numeric parts)
- The run is not on the `release` branch
- `release` cannot fast-forward to `master` (diverged history)

## Version numbers

**Canonical version for a release:** the git tag on the shipped commit.

| Tag       | App / release version |
| --------- | --------------------- |
| `v0.1.1`  | `0.1.1`               |

**SemVer (informal for this app):**

- **Patch** (`0.1.1` → `0.1.2`) — bug fixes, small behavior fixes
- **Minor** (`0.1.2` → `0.2.0`) — new features, no breaking UX changes
- **Major** (`0.2.0` → `1.0.0`) — breaking changes or “we’re calling it 1.0”

**Local development default** (not used for CI releases) lives in
[`Directory.Build.props`](Directory.Build.props) as `AwakeGuardVersion`. Update
it when you cut a release so `dotnet run` / local MSBuild match what you’re
working toward; CI always overrides from the tag on `release`.

**Do not** hardcode versions in About dialogs or `resource.rc` — they are fed
from MSBuild / the tag at release time.

## Step-by-step (normal release)

Replace `0.1.2` with your new version everywhere below.

### 1. Finish work on `master`

```powershell
git checkout master
git pull origin master
```

Ensure `master` is the commit you want to ship.

### 2. Tag and push the tag

Tag **on `master`** at the commit you are about to release:

```powershell
git tag -a v0.1.2 -m "AwakeGuard 0.1.2"
git push origin v0.1.2
```

Lightweight tags (`git tag v0.1.2`) also work; annotated tags are easier to
browse later.

### 3. Fast-forward `release` and push

`release` must point at the **same commit** as the tag:

```powershell
git checkout release
git pull origin release
git merge --ff-only master
git push origin release
```

`--ff-only` refuses a merge if `release` has diverged — fix history on
`release` before releasing (usually reset `release` to `master` if it was never
meant to diverge).

### 4. Watch CI

Open **Actions → Release** on GitHub. The `resolve-version` job must pass, then
all build jobs, then **Publish GitHub Release**.

Typical runtime: several minutes (nine Windows builds).

### 5. Verify the release

On GitHub **Releases**:

- Tag is `v0.1.2`, title `AwakeGuard 0.1.2`
- Nine assets plus `LICENSE` are attached:

| Artifact | Description |
| -------- | ----------- |
| `AwakeGuard-win32-{x64,x86,arm64}.exe` | Native, no runtime |
| `AwakeGuard-wpf-framework-dependent-win-{x64,x86,arm64}.zip` | Needs .NET 10 Desktop Runtime |
| `AwakeGuard-wpf-self-contained-win-{x64,x86,arm64}.zip` | Bundles runtime |

Optional smoke test:

```powershell
# Installer pulls the latest GitHub Release
irm https://raw.githubusercontent.com/KiwiGeek/AwakeGuard/master/scripts/install.ps1 | iex
```

Or download one `.exe` / `.zip` from the release page, check **Properties →
Details** for version `0.1.2.x` and open **About** in the tray app.

## Verify before you push (optional)

On your machine, after tagging locally but before pushing `release`:

```powershell
git describe --exact-match --tags HEAD   # must print v0.1.2
```

After `merge --ff-only master` on `release`, run the same on `release` — it
must show the same tag.

## Troubleshooting

### CI: “release branch HEAD must have an exact version tag”

- You pushed `release` without tagging that commit, or the tag points at a
  different commit than `release` HEAD.
- **Fix:** Tag the commit at `release` HEAD, push the tag, re-run the workflow
  (or push an empty commit — prefer fixing the tag).

### `merge --ff-only` failed

- `release` has commits not on `master`.
- **Fix:** If those commits were accidental, reset `release` to `master` (only
  if you are sure nothing on `release` should be kept):
  ```powershell
  git checkout release
  git reset --hard master
  git push --force-with-lease origin release
  ```
  Then tag (if needed) and push again.

### Tag already exists on GitHub

- You cannot reuse `v0.1.2` for a different commit. Pick the next patch version
  or delete the remote tag only if that release was never public (avoid rewriting
  published releases).

### GitHub Release job failed but builds succeeded

- Re-run the failed workflow from Actions, or fix permissions (`contents: write`
  is required for releases).

### Manual workflow run

**Actions → Release → Run workflow** only works when:

- Branch is `release`
- `HEAD` has an exact `vX.Y.Z` tag

Same rules as a push to `release`.

## What does *not* trigger a release

- Pushes to `master` only
- Tags on `master` without updating `release`
- Pushes to `release` without a tag on `HEAD`

## Related files

| File | Role |
| ---- | ---- |
| [`.github/workflows/release.yml`](.github/workflows/release.yml) | CI pipeline |
| [`Directory.Build.props`](Directory.Build.props) | Local default version; CI overrides via `/p:AwakeGuardVersion` |
| [`AwakeGuard-win32/msbuild/AwakeGuard.Version.targets`](AwakeGuard-win32/msbuild/AwakeGuard.Version.targets) | Win32 manifest version patching (must be tracked; not under a `build/` folder) |
| [`scripts/install.ps1`](scripts/install.ps1) | Installs from latest GitHub Release |
