# trajectory developer guide

This guide is for developers who change `smartmet-library-trajectory`. The library computes
trajectories of massless particles (air parcels, plumes, sounding balloons) through the wind
fields of QueryData. The server's trajectory plugin, the SmartMet Editor and the
`qdtrajectory` program built from this repository use it.

[CLAUDE.md](../CLAUDE.md) has the class overview.

## Contents

1. [Building and testing](#1-building-and-testing)
2. [Computing a trajectory](#2-computing-a-trajectory)
3. [Surface trajectories](#3-surface-trajectories)
4. [3D trajectories](#4-3d-trajectories)
5. [Plumes and balloons](#5-plumes-and-balloons)
6. [Output](#6-output)
7. [Compatibility](#7-compatibility)
8. [Known pitfalls](#8-known-pitfalls)

---

## 1. Building and testing

```bash
make           # libsmartmet-trajectory.so and qdtrajectory
```

There are no tests; CI builds the RPMs only. Check changes by comparing `qdtrajectory` or
trajectory plugin output before and after. The sources are **Latin-1** encoded; open and
save them as such (and use `grep -a`, since some tools treat them as binary).

## 2. Computing a trajectory

```cpp
auto trajectory = std::make_shared<NFmiTrajectory>();
// start point, start time, producer, level, direction, time step, length, plume settings ...
NFmiTrajectorySystem::CalculateTrajectory(trajectory, info);   // info: shared_ptr<NFmiFastQueryInfo>
```

`CalculateTrajectory()` computes the main trajectory (`NFmiSingleTrajector`) and, if plume
particles are requested, each plume particle. A trajector holds the points (lon/lat), and for
3D trajectories also the pressures and heights along the path.

The data decides the method: data with **one level** gives a surface (2D) trajectory, data
with **several levels** (pressure or hybrid) a 3D trajectory.

## 3. Surface trajectories

For each time step (the trajectory's time step, in minutes):

1. wind speed and direction are interpolated at the current point and time
   (`InterpolatedValue(latlon, time)`);
2. the particle moves `speed × time step` metres along the great circle in the wind's
   direction (against it for backward trajectories);
3. the new point is added.

This is a forward Euler step. The trajectory **stops at the first missing wind value**,
for example when it leaves the data area or its time range.

## 4. 3D trajectories

The 3D calculation interpolates speed, direction and vertical velocity at the current
pressure level with `FastPressureLevelValue()`, moves the particle horizontally as above,
and changes its pressure:

* by the vertical velocity: `kFmiVelocityPotential` (used as omega) if present, otherwise
  `kFmiVerticalVelocityMMS`;
* or, for **isentropic** trajectories, so that the particle stays on the potential
  temperature surface it started on (needs `kFmiPotentialTemperature`, and `kFmiPressure`
  for hybrid data).

The height along the path is computed from the data's height parameter. The pressure is
kept between the ground and the top level of the data.

## 5. Plumes and balloons

* A **plume** is a set of extra particles whose start time, place and pressure level are
  randomised within the given ranges, and whose wind speed and direction are perturbed by
  `randFactor` every `randStep` steps. The randomisation uses `rand()`.
* A **sounding balloon** (`NFmiTempBalloonTrajectorSettings`) rises at a given speed, may
  float at a given level, and falls; its pressure follows these phases instead of the
  vertical velocity.

## 6. Output

`qdtrajectory` formats the trajectories with the CTPP2 templates in `tmpl/`: GPX, KML, KMZ,
the `x` variants with `gx:track` time stamps, and XML. The trajectory plugin uses the same
kind of compiled templates (`.c2t`) from its own template directory. The editor saves trajectories with the classes' own `Write` / `Read` format.

## 7. Compatibility

The headers are installed and used by the trajectory plugin and the editor. The classes
use smarttools' `NFmiDataStoringHelpers` for their stored format; keep old stored
trajectories readable. A change in the class layout requires rebuilding the plugin.

## 8. Known pitfalls

* **Missing prerequisites give an empty trajectory, not an error.** A 3D trajectory
  silently stops before its first step if the vertical velocity (or, for isentropic ones,
  the potential temperature or pressure) is missing, and a 2D trajectory stops at the first
  missing wind value.
* **Plumes are not reproducible**: `rand()` is not seeded per trajectory.
* **Isentropic balloon trajectories are not supported**: the combination returns nothing.
* **Speed and direction are interpolated separately** (direction modulo 360), not as
  u and v components, which smooths out turning winds differently from a vector mean.
* **The sources are Latin-1** (§1).
