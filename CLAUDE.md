# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

`smartmet-library-trajectory` is a C++17 library for massless particle trajectory calculations in meteorological wind fields. It computes forward/backward trajectories by integrating particle motion through gridded wind data (QueryData format). Part of the SmartMet Server ecosystem by the Finnish Meteorological Institute.

## Build commands

```bash
make                  # Build library (libsmartmet-trajectory.so) and qdtrajectory CLI tool
make clean            # Clean build artifacts
make format           # Run clang-format on all source files
make rpm              # Build RPM package
make install          # Install (PREFIX=/usr by default)
```

There is no test directory or test suite in this project.

## Dependencies

Requires system-installed SmartMet libraries: `newbase`, `smarttools`, `locus`, `macgyver`, `gis`. Also links against Boost (iostreams, locale, program_options, regex, thread), CTPP2 (template engine), libpqxx, and GDAL. Headers are expected at `/usr/include/smartmet/`.

## Code architecture

The library has two distinct consumers:

1. **Library API** (`trajectory/`) — used by SmartMet Editor (desktop) and SmartMet Server's trajectory plugin
2. **CLI tool** (`main/qdtrajectory.cpp`) — standalone command-line trajectory calculator

### Core classes (all in `trajectory/`)

- **`NFmiTrajectorySystem`** — top-level orchestrator. Manages a collection of trajectories, holds UI-related settings (from SmartMet Editor), and contains the static `CalculateTrajectory()` method that performs the actual integration. The calculation logic (2D surface and 3D pressure-level) lives in `CalculateSingleTrajectory` and `CalculateSingle3DTrajectory`.
- **`NFmiTrajectory`** — represents one trajectory computation: start point (lat/lon), time, producer, pressure level, direction, and plume settings. Contains one main `NFmiSingleTrajector` plus optional plume particles.
- **`NFmiSingleTrajector`** — a single particle track: vectors of (lat/lon) points, pressures, and heights along the computed path. Holds per-particle random perturbation values for plume dispersion.
- **`NFmiTempBalloonTrajectorSettings`** — models radiosonde balloon flight phases (rising, floating, falling) with vertical speed profiles and omega/pressure-change calculations.

### Output templates (`tmpl/`)

CTPP2 templates (`.tmpl` compiled to `.c2t`) for output formats: GPX, KML, KMLx (with gx:track), KMZ, KMZx, and XML. Used by the `qdtrajectory` CLI tool.

### Key design notes

- Class names use the `NFmi` prefix (legacy FMI convention shared across all SmartMet libraries)
- Comments in Finnish are common throughout the codebase
- The library depends on `NFmiInfoOrganizer` and `NFmiProducerSystem` from `smarttools` for data access in the Editor context; the CLI tool bypasses these by using `NFmiFastQueryInfo` directly
- `FmiDirection` enum (`kForward`/`kBackward`) controls trajectory integration direction
- Data types: 0=surface, 1=pressure level, 2=model level, 3=historical data
- Serialization uses custom `Write`/`Read` methods with `operator<<`/`operator>>` overloads (not standard formats)

## CI

CircleCI builds RPMs on RHEL 8 and RHEL 10 Docker images (`fmidev/smartmet-cibase-{8,10}`). Tests are disabled in CI (`.circleci/disable-tests-in-ci` marker file exists).
