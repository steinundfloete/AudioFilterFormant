# AudioFilterFormant – Changelog

## [1.2.0] – 2025-11-10
### Added
- Audio-rate modulation inputs for vowel and brightness (2 extra AudioStream inputs).
- Breath noise and consonant excitation controls (`setBreath`, `setConsonant`).
- Bypass with zero-CPU dry-through when bypassed or mix == 0.

## [1.1.0] – 2025-11-09
### Added
- `setMix(float mix)` function for dry/wet balance (0.0 = dry, 1.0 = fully filtered).
- GitHub-ready Arduino library structure, README, LICENSE, and example.

## [1.0.0] – 2025-11-08
### Added
- Initial release: smooth, morphable 3-band formant filter.
- Male/female/child morphing.
- Brightness (vocal tract length) and Q loudness compensation.
