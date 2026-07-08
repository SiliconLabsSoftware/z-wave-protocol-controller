# Debian package (`.deb`) after build

On Linux, the project configures **CPack** with the **DEB** generator so you can produce a Debian package from the same `install()` rules used for a normal install.

## Prerequisites

- Debian or Ubuntu build host (or compatible) with build dependencies from `ci/dependencies/apt-packages-base.txt` at the repository root.
- CMake 3.25+ and Ninja (the `debian` **workflow** includes a `package` step; see `CMakePresets.json` at the repository root).

## Install prefix

The `debian` configure preset sets **`CMAKE_INSTALL_PREFIX`** from the parent environment (`$penv{CMAKE_INSTALL_PREFIX}` in `CMakePresets.json`). If unset, configure falls back to **`/usr/local`**. Override with the environment variable or `-DCMAKE_INSTALL_PREFIX=...`.

For FHS paths and the `/var/lib/zpc` install rule in `applications/zpc/CMakeLists.txt`, set **`CMAKE_INSTALL_PREFIX=/usr`** when packaging (see `applications/zpc/CMakeLists.txt`).

## Build and generate the `.deb`

**Typical sequence (matches CI):**

```sh
CMAKE_INSTALL_PREFIX=/usr cmake --workflow --preset debian
```

That performs configure, build, and CPack (`package`). The workflow’s `package` step requires CMake 3.25+ and a Linux host (CPack is not enabled on macOS).

**Equivalent without the workflow preset:**

```sh
cmake --preset debian -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build/debian
cmake --build build/debian --target package
```

CPack writes the `.deb` under `build/debian/`.

Runtime **`Depends:`** are set in `cmake/include/cpack.cmake` (`CPACK_DEBIAN_PACKAGE_DEPENDS`). Override at configure time with `-DCPACK_DEBIAN_PACKAGE_DEPENDS='...'` if needed.

## Cross-compilation

When using `cmake/arm64_debian.cmake` or `cmake/armhf_debian.cmake`, `CPACK_DEBIAN_PACKAGE_ARCHITECTURE` is preset for the target. Align `CMAKE_INSTALL_PREFIX` and sysroot with your cross environment.

## macOS

CPack Debian packaging is **not** enabled on Apple hosts (`CMakeLists.txt` gates the CPack include). The `package` workflow step will fail there; build the package on Linux or in CI.

## CI

Debian packaging runs in GitHub Actions on native Linux runners via [`.github/workflows/zpc-build-linux.yaml`](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/.github/workflows/zpc-build-linux.yaml). Pull requests run full CI in [`.github/workflows/zpc-ci.yaml`](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/.github/workflows/zpc-ci.yaml), which calls the Linux build workflow after lint checks. Pushes to `main` trigger the Linux build workflow directly. Tagged releases (`zpc-v*`) run [`.github/workflows/zpc-release.yaml`](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/.github/workflows/zpc-release.yaml), which calls the Linux build workflow and publishes `.deb` assets to GitHub Releases. CI produces `amd64` and `arm64` packages only. Jobs configure with `CMAKE_INSTALL_PREFIX=/usr`, run CPack, and upload `.deb` files with `actions/upload-artifact` (**1-day retention**). Download packages from the workflow run’s **Artifacts** panel. Configure needs a repository state where `git describe` succeeds (see `cmake/include/version.cmake`).
