# Megusta

This is a decompilation and rewrite of the unreleased Dynamix game "MiniGolf Maniacs". The rationale for this is:

1) Determine any changes to the underlying dependencies that wern't contributed back to their original projects (e.g. openal, plib, glpng, python, dts).
2) More easily play the game on modern systems with QOL improvements.
3) Critique and comment on the structure of the simulation code.

Currently it can be built for Windows, MacOS, Linux, and Emscripten.

You will need a copy of the game assets in order to properly run the game, which is an exercise left to the reader.

See the following docs for relevant info:

- [Build and run](megusta/docs/BUILD.md)
- [Vendored library modifications](megusta/docs/UPSTREAM_MODIFICATIONS.md)

## Underlying process

The original executable was reverse engineered via an LLM. This code was then used to create the new implementation with occasional comparisons with the original executable where constants were missed or code incorrectly interpreted. The re-implementation was largely "vibe coded" with back with relevant feedback and direction to improve output (i.e. not fully automated).

Addresses are listed for certain functions where there is a direct equivalent. Some enhancements were added to make the game work better on modern platforms, or help with porting... in both cases there is no direct equivalent in the original executable.

Since the original executable made use of several open source third party libraries, additional analysis was performed to determine any relevant changes to their underlying code. Patches against these dependencies are supplied in the "patches" folder. See `upstream/manifest.json` for reference code used.

No specific copyright claim is made for the patches or changes in vendored libraries. The reverse engineered game code is licensed under GPL V2. For now, the project should merely be attributed as "megusta".


