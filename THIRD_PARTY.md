# Third-party dependencies and provenance

## SFML

- Upstream: https://www.sfml-dev.org/ and https://github.com/SFML/SFML
- Used for graphics, window/input handling, timing and vector types.
- Required API: SFML 3. Locally verified version: 3.0.2. SFML is installed externally, not vendored into this repository.
- License: zlib. The SFML 3.0.2 notice is reproduced in [docs/third-party/SFML-license.md](docs/third-party/SFML-license.md), copied from the installed, unmodified SFML 3.0.2 distribution. Upstream notice: https://github.com/SFML/SFML/blob/3.0.2/license.md

The initial `BoidSystem` drawing scaffold was adapted from the SFML [vertex-array / particle-system example](https://www.sfml-dev.org/tutorials/3.0/graphics/vertex-array/#example-particle-system). Repository commit `0e5064d1a6b2c75e176a72e84fd47720e0ec94db` records this origin. The project changes the example substantially: flocking rules, structure-of-arrays state, spatial queries, force accumulation, boundary behavior and triangle geometry are application work. It is not an unmodified SFML example.

## Roboto Mono

An unmodified copy of Roboto Mono is bundled for the overlay. See [assets/fonts/README.md](assets/fonts/README.md) for the pinned Google Fonts source, original filename and SHA-256 checksum. The full [SIL Open Font License 1.1](assets/fonts/OFL-RobotoMono.txt), including its copyright notice, accompanies the font.

The files were reused from the same verified font asset in the author's Stable Fluids project. No Digital-7 font is included or required.

## Original project code

These third-party notices apply to their respective components. No license has been selected here for the original Boids source code.
