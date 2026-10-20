#pragma once

// Значение задаётся CMake через -DSIDDAW_VERSION="X.Y.Z"
// Fallback на случай, если файл открывают в IDE без CMake
#ifndef SIDDAW_VERSION
#define SIDDAW_VERSION "0.3.0"
#endif
