ANSI Art Samples (for future color-capable viewer)

- These files contain classic ANSI escape sequences (e.g., ESC[31m for red) and CP437-style box art.
- Current viewers in this repo do not parse ANSI color; these are test assets for future features.
- You can still open them in the test_pattern app’s text viewer (File → Open Animation File…) to preview raw codes.

Files
- rainbow_title.ans: Rainbow-colored “TVISION” title with a border.
- boxes.ans: Box-drawing characters with a few colored labels.
- wave.ans: A simple color gradient wave using background colors.

Tip
- On terminals that support ANSI, `cat` the files to preview colors: `cat rainbow_title.ans`.

