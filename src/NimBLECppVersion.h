/*
 * Copyright 2020-2025 Ryan Powell <ryan@nable-embedded.io> and
 * esp-nimble-cpp, NimBLE-Arduino contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NIMBLE_CPP_VERSION_H_
#define NIMBLE_CPP_VERSION_H_

/** @brief NimBLE-Arduino library major version number. */
#define NIMBLE_CPP_VERSION_MAJOR 2

/** @brief NimBLE-Arduino library minor version number. */
#define NIMBLE_CPP_VERSION_MINOR 3

/** @brief NimBLE-Arduino library patch version number. */
#define NIMBLE_CPP_VERSION_PATCH 9

/**
 * @brief Macro to create a version number for comparison.
 * @param major Major version number.
 * @param minor Minor version number.
 * @param patch Patch version number.
 * @details Example usage:
 * @code{.cpp}
 * #if NIMBLE_CPP_VERSION >= NIMBLE_CPP_VERSION_VAL(2, 0, 0)
 *   // Using NimBLE-Arduino v2 or later
 * #endif
 * @endcode
 */
#define NIMBLE_CPP_VERSION_VAL(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))

/**
 * @brief The NimBLE-Arduino library version as a single integer for compile-time comparison.
 * @details Format: (major << 16) | (minor << 8) | patch
 */
#define NIMBLE_CPP_VERSION \
    NIMBLE_CPP_VERSION_VAL(NIMBLE_CPP_VERSION_MAJOR, NIMBLE_CPP_VERSION_MINOR, NIMBLE_CPP_VERSION_PATCH)

/** @cond NIMBLE_CPP_INTERNAL */
#define NIMBLE_CPP_VERSION_STRINGIFY_IMPL(x) #x
#define NIMBLE_CPP_VERSION_STRINGIFY(x)      NIMBLE_CPP_VERSION_STRINGIFY_IMPL(x)
/** @endcond */

/** @brief NimBLE-Arduino library version as a string. */
#define NIMBLE_CPP_VERSION_STR                         \
    NIMBLE_CPP_VERSION_STRINGIFY(NIMBLE_CPP_VERSION_MAJOR) "." \
    NIMBLE_CPP_VERSION_STRINGIFY(NIMBLE_CPP_VERSION_MINOR) "." \
    NIMBLE_CPP_VERSION_STRINGIFY(NIMBLE_CPP_VERSION_PATCH)

#endif // NIMBLE_CPP_VERSION_H_
