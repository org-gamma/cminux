<!--
Thanks for the pull request. Before you continue, read CONTRIBUTING.md.
Most rejected pull requests are rejected because they violate a design
rule, not because the code was bad.
-->

## What this changes

<!-- One paragraph. What the change does, not why. -->

## Why

<!--
The reason the change is needed. If it fixes a bug, describe the bug. If
it adds a capability, describe the problem it solves.
-->

## Release kind

<!-- Check one. See VERSIONING.md for the definition of each kind. -->

- [ ] Major (changes the documented surface — doc updates included)
- [ ] Minor (adds a capability within the documented surface — no doc updates)
- [ ] Patch (fixes a bug — no doc updates)
- [ ] Not a release (test-only, refactor, CI, build script)

## Checklist

- [ ] The change does not violate any design rule in `CONTRIBUTING.md` section 4.
- [ ] I have added a test that fails before this change and passes after (if behavioral).
- [ ] The full test suite passes locally (`tests\run.bat` or `bash tests/run.sh`).
- [ ] If this is a major change, I have updated `README.md` and every affected file in `docs/`, and bumped the version lines.
- [ ] If this is a minor or patch change, I have not touched `README.md` or `docs/` (except prose typos).
- [ ] If this is a minor, patch, or major change, I have bumped the version string in `src/emitter.cpp`, `src/main.cpp`, and `CMakeLists.txt`.
- [ ] I have not modified the version number inside the pull request itself (version bumps happen at release).

## Related issues

<!-- "Fixes #123" or "See #456" if applicable. Leave blank otherwise. -->