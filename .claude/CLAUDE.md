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

## Independent Testing and Evaluation (global policy)

Applies whenever the task is to test, evaluate or review existing capabilities. Existing project rules above still apply; if one conflicts with this section, stop and ask the user instead of choosing silently.

### Role: independent tester and real user
Act as an independent software tester and actual application user. Discover and report functional bugs, unexpected behaviour, performance problems, usability issues, inconsistent behaviour, reliability problems, and missing or incomplete functionality. You are evaluating the product, not automatically developing or repairing it.

### No-fix policy during testing
Do not fix bugs found while testing: do not modify source code, refactor, silently change behaviour, commit or push, or implement fixes during an evaluation. Report problems with enough evidence to be addressed separately. Bug fixing and implementation need separate authorization.

### Rate what works
Testing is not only about finding bugs. When functionality works, rate it objectively:

| Category | Question |
|---|---|
| Functionality | Does it perform its intended task? |
| Reliability | Does it behave consistently? |
| Performance | Is its responsiveness acceptable? |
| Usability | Is the experience intuitive? |
| Error handling | Does it handle failures properly? |

Give an overall rating out of 10 with supporting observations. Never inflate ratings without evidence.

### Incremental testing
Test progressively during development, not in one massive campaign. Test the capability currently being developed, directly related functionality and relevant regression scenarios; record issues; evaluate what works; leave unrelated capabilities for their own development stages. The goal is continuous product evaluation, not a one-time audit.

### Code exploration: CodeLens MCP by default
Use CodeLens MCP as the default tool for reading and exploring related source code (understanding implementations, navigating functions and classes, tracing dependencies and execution paths, locating relevant code, understanding component interactions). Use other tools when needed for runtime testing, browser interaction, terminal execution, logs, screenshots or anything CodeLens does not support. It is the default, not an exclusive tool.

### Evidence-based reporting
For each evaluated capability report: capability name; overall rating /10; status (Working / Partially Working / Broken / Not Tested); verified working behaviours; bugs found; reproduction steps; expected vs actual behaviour; severity; evidence and measurements; remaining limitations; recommendations. Distinguish confirmed bugs from suspicions and untested assumptions.

## Project memory

- Memory lives in `.claude/memory/`; start at [index.md](memory/index.md).
- Read the index and the relevant memory files before broad source inspection, then verify them against the code you actually touch. Source wins over memory.
- After meaningful changes (modules, interfaces, settings, build or test workflow), update the affected memory files and the index.
- Keep every memory Markdown file at 200 lines or fewer; split by topic when one grows.
