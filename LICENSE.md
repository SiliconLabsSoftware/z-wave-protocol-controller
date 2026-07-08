# Licensing terms

Source code in this repository is licensed to you under the terms of a default
license from Silicon Laboratories Inc. This default license and exceptions to
this default licensing are set forth below.

The default license is the Master Software License Agreement (MSLA)
(https://www.silabs.com/about-us/legal/master-software-license-agreement),
which applies unless otherwise noted.

Some files use different licensing terms. If so, they will be clearly marked at
the beginning of the file.

Some code from third parties and external projects has been included in or
linked by this repository under separate licenses. Those components and their
licenses are listed below.

---

## BSD 3-Clause License

```
scripts/command_class_generator/zwave.xml
scripts/command_class_generator/zwave.xsd
```

Copyright 2022-2025 Z-Wave Alliance <https://z-wavealliance.org/>

---

## Public Domain / The Unlicense

```
components/zwave/zwave_transports/s2/libs/zw-libs2/crypto/aes/aes.c
components/zwave/zwave_transports/s2/libs/zw-libs2/crypto/curve25519/base.c
components/zwave/zwave_transports/s2/libs/zw-libs2/crypto/curve25519/smult.c
```

`aes.c` — tiny-AES128-C by kokke, released into the public domain (see
`crypto/aes/unlicense.txt`).

`curve25519/base.c`, `curve25519/smult.c` — derived from public domain code by
D. J. Bernstein and Matthew Dempsky (version 20081011).

---

## MIT License

The following components are used as build-time or runtime dependencies and are
distributed under the MIT License:

- **{fmt}** (https://github.com/fmtlib/fmt, v12.1.0)
- **yaml-cpp** (https://github.com/jbeder/yaml-cpp)
- **nlohmann/json** (https://github.com/nlohmann/json)

---

## Apache License 2.0

The following component is used as a runtime dependency and is distributed
under the Apache License, Version 2.0
(https://www.apache.org/licenses/LICENSE-2.0):

- **OpenSSL** (https://www.openssl.org/)

---

## Eclipse Public License 2.0 / Eclipse Distribution License 1.0

The following component is used as a build-time dependency and is distributed
under the dual EPL-2.0 / EDL-1.0 license:

- **Eclipse Paho MQTT C/C++** (https://github.com/eclipse/paho.mqtt.cpp, v1.5.3)

---

## Public Domain (SQLite)

The following component is used as a build-time dependency and is released into
the public domain:

- **SQLite** (https://www.sqlite.org/, amalgamation 3.53.0)
