# Dockly – working notes for Claude

## Build and commit

- Build with `build.bat` in the repo root (`build.bat nodeploy` skips the deploy step).
  It does a clean rebuild through `scripts\build.ps1`, then launches the app.
- Commit messages live in `commit.txt` in the repo root. This file is git-ignored,
  so it is never tracked itself.
- Before running `build.bat`, write the commit message to `commit.txt`.
  Keep it in the normal git format: a short subject line, a blank line, then the body.
- On a successful build, `build.bat` runs `git add -A`, `git commit -F commit.txt`, and `git push`.
  If the build fails, nothing is committed or pushed.
- End `commit.txt` with this trailer line:

  Co-Authored-By: Claude Haiku 5.5 <noreply@anthropic.com>
