# Getting started

This page walks through the dependencies, NCP setup, build, configuration, and first run of ZPC. For a step-by-step end-to-end Switch On/Off walkthrough (including the MQTT topics used during inclusion and control), see the [Switch On/Off demo](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/README.md#switch-onoff-demo) in the project README.

## Prerequisites

ZPC builds on **Linux** and **macOS**. The following environments have been tested:

- **Linux** — Debian Trixie with **GCC 14.x** (CI uses the same environment; see `ci/docker/Dockerfile`)
- **macOS** — **Clang 17.x** and **Clang 21.x** (AppleClang)

CMake rejects unsupported compilers at configure time — see `cmake/include/compiler_options.cmake`.

- **CMake** 3.25+ and **Ninja** — required by the `macos` and `debian` presets (`CMakePresets.json`)

## Install dependencies

See [Prerequisites](#prerequisites) for tested platforms and compilers.

### macOS

```bash
brew install $(cat ci/dependencies/brew-packages.txt)
pip3 install -r ci/dependencies/requirements.txt
```

Code formatting uses **LLVM 19** — run `./ci/scripts/clang-format.sh format` or `check`. See [CONTRIBUTING.md](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/CONTRIBUTING.md#formatting) and the optional pre-commit hook (`./.githooks/install.sh`).

Static analysis uses **LLVM 19** (`llvm@19` from `brew-packages.txt`) — run `./ci/scripts/clang-tidy.sh check`. See [CONTRIBUTING.md](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/CONTRIBUTING.md#clang-tidy).

### Linux (Debian)

```bash
apt-get install -y $(cat ci/dependencies/apt-packages-base.txt)
pip3 install -r ci/dependencies/requirements.txt
```

On Debian/Ubuntu, the formatting script uses **`clang-format-19`** — install it from `ci/dependencies/apt-packages-clang-format.txt`. See [CONTRIBUTING.md](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/CONTRIBUTING.md#formatting).

Static analysis uses **`clang-tidy-19`** from `ci/dependencies/apt-packages-clang-tidy.txt` (with `apt-packages-base.txt`) — run `./ci/scripts/clang-tidy.sh check`. See [CONTRIBUTING.md](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/CONTRIBUTING.md#clang-tidy).

### Python virtual environment

To avoid installing Python packages into the system environment, create and activate a [virtual environment](https://docs.python.org/3/library/venv.html) before running `pip3 install`:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip3 install -r ci/dependencies/requirements.txt
```

Keep the virtual environment activated when you run `cmake --workflow --preset <preset>` — CMake invokes `python3` at configure time for command-class generation.

## Set up the MQTT broker

ZPC uses any MQTT broker; the examples assume [Mosquitto](https://mosquitto.org/download/) on the default port:

```bash
mosquitto -p 1883
```

### Docker

[`ci/docker/Dockerfile`](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/ci/docker/Dockerfile) defines two images:

| Image | Use |
|---|---|
| **`zpc-dev`** | Development environment — build, clang-format, clang-tidy, and run. Published as `ghcr.io/<owner>/zpc:main` when docker-related files change on main, and as `ghcr.io/<owner>/zpc:<version>` after a successful release. |
| **`zpc-test`** | CI sanity check — builds the `.deb`, installs runtime deps, and runs `zpc --help` during image build. Not published. |

[`ci/docker/docker-compose.yml`](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/ci/docker/docker-compose.yml) exposes two profiles:

**Out-of-box (`oob`)** — builds the runtime image, starts Mosquitto, and runs ZPC for a new user:

```bash
cd ci/docker
docker compose --profile oob up --build
```

Set `ZPC_SERIAL=/dev/ttyACM0` to attach an NCP serial device; leave it unset to start without hardware (`/dev/null` is used as a placeholder). Edit `example_config.yaml` for `zpc.serial` or IP NCP settings when using a real device.

**Development (`dev`)** — Mosquitto plus an interactive dev shell with the repo mounted at `/home/zpc`:

```bash
cd ci/docker
docker compose --profile dev run --rm -it zpc-dev bash
```

One command — starts Mosquitto, drops you into the container, removes it when you exit.

Build, format, tidy, and test inside the container as you would on a native Debian host.

## Set up the NCP

Flash the **Z-Wave NCP Serial API Controller** firmware onto your Silicon Labs board and note the serial port it enumerates as (for example `/dev/ttyACM0` on Linux or `/dev/tty.usbmodem*` on macOS). ZPC also supports IP-attached NCPs.

## Build ZPC

ZPC uses CMake presets. Use `macos` on macOS and `debian` on Debian (and inside Docker):

```bash
cmake --workflow --preset <preset>
```

On Linux, you can produce a `.deb` package after the build — see [Debian packaging](packaging-debian.md).

## Configure ZPC

Use the `example_config.yaml` in the repository root as a template and point ZPC at it with `--conf <path>`. The minimum settings under the `zpc:` section are:

- **NCP connection** — either a serial port or an IP/port pair:

```yaml
zpc:
  serial: /dev/ttyACM0
  # or, for an IP-attached NCP:
  # ip_address: '192.168.1.2'
  # ip_port: 4901
```

- **Connection log file** — where serial/IP communication of the Z-Wave module is logged:

```yaml
zpc:
  connection_log_file: /path/to/connection.log
```

- **Datastore file** — path to the persistent attribute-store database:

```yaml
zpc:
  datastore_file: /path/to/database.db
```

## Run ZPC

```bash
./build/<preset>/applications/zpc/zpc --conf <path_to_config_file> --log.level d
```

ZPC is controlled entirely over MQTT — it does not read from the terminal. See the [MQTT API index](../components/mqtt_api/doc/mqtt_api_index.md) for the available commands.

## What's next

- **Discover the Home ID and start adding devices** — see the [MQTT API index](../components/mqtt_api/doc/mqtt_api_index.md) for every command and report topic, and the [Inclusion flow](sequences/inclusion_flow.md) for the full sequence (including S2/DSK).
- **Per-device control** — see the [Command Classes MQTT Interface](../components/command_classes/doc/generated/mqtt_interface.md).
- **OTA firmware updates** — see the [OTA Firmware Manager](../components/ota/docs/ota.md).
