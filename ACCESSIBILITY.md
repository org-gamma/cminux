# Accessibility

cminux is a command-line compiler. It has no graphical interface, no
website, and no interactive prompts. The accessibility surface is the
terminal, the error messages, the documentation, and the source code —
and each of those has a small set of people it can fail.

This document says what cminux commits to, what it does not, and how to
report a barrier.

## Priorities

cminux prioritizes three things, in this order:

1. **Error messages that can be read.** Every error names the file, line,
   column, and a plain sentence describing what went wrong. Errors are
   printed to stderr as plain text with no color, no Unicode box drawing,
   and no cursor movement. They render correctly in screen readers,
   braille displays, and plain terminals.

2. **Output that does not depend on color.** cminux does not colorize
   anything. There is no `--color` flag because there is no color. This is
   deliberate: color is a common source of accessibility barriers in
   developer tools, and cminux avoids the problem by not having one.

3. **Documentation that is readable as text.** README.md and every file in
   docs/ are plain Markdown, no images, no diagrams that require visual
   interpretation, no code examples that depend on syntax highlighting.
   They render the same in a terminal, in a plain text editor, and in a
   browser.

cminux does not claim WCAG conformance, because it has no web interface
to conform. If you are looking for accessibility guidance on a project
that has a website, this is not the right document.

## Contributor expectations

Contributions should:

- Not introduce color, animation, or Unicode that changes meaning.
  (Em-dashes and other typographic characters are fine; emoji that
  replace words are not.)
- Write error messages that read as a full sentence, not as a code or
  abbreviation. "expected '[' after 'import'" is fine. "E001" is not.
- Keep documentation in plain Markdown. No Mermaid diagrams, no images
  that contain text that is not also in the surrounding prose.
- Not add command-line options that require a specific terminal emulator
  or a specific font to render.

There is no automated accessibility test. Review is manual, by whoever
reviews the pull request, against the expectations above.

## Reporting accessibility issues

Open an issue at the repository's issue tracker with the label
`accessibility`. If the issue is a security concern, follow SECURITY.md
instead.

Useful context:

- What you were trying to do.
- What happened instead.
- Your operating system and terminal (or screen reader, or braille
  display — whichever applies).
- The exact error message or output, copied as text rather than as a
  screenshot.

You do not need to disclose a disability to report a barrier. Describe
what you observed; that is enough.

### Severity

cminux uses two labels, not a full severity scale:

- **Barrier** — something is impossible or very hard to do because of how
  cminux presents information. For example, an error message that a screen
  reader reads as one long unbroken string with no line breaks.
- **Friction** — something is harder than it should be, but there is a
  workaround. For example, a documentation code block that is not
  indented in a way some markdown renderers handle well.

Maintainers assign or confirm the label during triage. You do not need to
pick one when you report.

### How we respond

You will get an acknowledgement within 7 days. If the issue is a barrier,
it will be fixed or a workaround documented before the next release. If it
is friction, it will be triaged alongside other work and may wait. You will
be told which it is.

## Ownership and maintenance

cminux is maintained by a single person. Accessibility issues are handled
by the same person as everything else. There is no separate accessibility
team and no review cadence beyond "when an issue is filed." If this
changes, this section will change.

## Supported environments

cminux is tested on:

- Windows 10 and 11, in `cmd.exe` and in Windows Terminal.
- Linux, in common terminals (gnome-terminal, konsole, xterm).
- macOS, in Terminal.app.

It is not tested with screen readers or braille displays. If you use one
and encounter a barrier, that is exactly the kind of issue this document
is asking you to report.

## Known limitations

- **No screen reader testing.** cminux's output has been reviewed by hand
  for readability but has not been tested with NVDA, JAWS, VoiceOver, or
  Orca. This is a known gap.
- **No color-blindness concerns.** cminux has no color, so this does not
  apply.
- **Documentation is English-only.** Translations are welcome but none
  exist.
- **Error messages use `file:line:col` format.** This format is compact
  and standard among compilers, but it is read as a single token by some
  screen readers. If this is a barrier for you, say so and a `--verbose`
  error format with one line per field can be discussed.

If a limitation above affects you, report it. This list is not a claim
that no barriers exist; it is the list of what is known today.

## Feedback and improvements

To suggest changes to this document, open an issue with the label
`accessibility`. To report an active barrier, use the reporting process
above — the barrier takes priority over the wording of this document.