# Security Keys Dump MQTT API

On request, ZPC seals its currently-assigned S2 (and S0, if any) keys with a given X25519 recipient public key and writes the envelope to a configured path. The report carries only a status code and no key material is exposed to the broker.

The feature is disabled by default. To enable, set `security.security_keys_dump_enable: true` and configure a recipient public key + output path (see [Setup](#setup)).

## Topics

| Type            | Topic                                           |
|-----------------|-------------------------------------------------|
| Request         | `zpc/<home_id>/Network/DumpSecurityKeys`        |
| Report          | `zpc/<home_id>/Network/DumpSecurityKeys/Report` |

## Request payload

The request body is empty.

## Report payload

```json
{ "status": <status_code> }
```

| `status` | Name                | Meaning                                                                                  |
|---------:|---------------------|------------------------------------------------------------------------------------------|
| `0`      | ok                  | Encrypted dump written to `<security_keys_dump_output_dir>/<home_id>.bin`.               |
| `1`      | feature_disabled    | `security_keys_dump_enable` is false.                                                    |
| `2`      | missing_config      | `security_keys_dump_recipient_pubkey_path` or `security_keys_dump_output_dir` is empty.                                        |
| `3`      | pubkey_load_failed  | PEM file missing, unreadable, or not an X25519 public key.                               |
| `4`      | collect_failed      | Internal: keystore extraction failed.                                                    |
| `5`      | seal_failed         | Internal: OpenSSL sealing failed.                                                        |
| `6`      | file_write_failed   | Output-path validation or I/O failed (see ZPC log).                                      |

The report never contains key material, fingerprints or path information.

## Setup

### 1. Generate a recipient keypair (one-time)

```bash
openssl genpkey -algorithm X25519 -out recipient_priv_key.pem
openssl pkey -in recipient_priv_key.pem -pubout -out recipient_pub_key.pem
```

Keep `recipient_priv_key.pem` private for decryption and deploy `recipient_pub_key.pem` to ZPC (see below).

### 2. Deploy `recipient_pub_key.pem` to ZPC

Place `recipient_pub_key.pem` to a location on ZPC (e.g., `/etc/zpc/recipient_pub_key.pem`).

### 3. Configure `zpc.cfg`

```yaml
security:
  security_keys_dump_enable: true
  security_keys_dump_recipient_pubkey_path: /etc/zpc/recipient_pub_key.pem
  security_keys_dump_output_dir: /var/lib/zpc/dumps
```

The encrypted dump file is always named `<home_id>.bin` inside `security_keys_dump_output_dir`, where `<home_id>` is the controller's Z-Wave Home ID as 8 uppercase hex characters. ZPC auto-creates any missing parent directories with mode `0700`. If a parent directory already exists, it is left as-is but must not be world-writable or a symlink (ZPC refuses to write into such a directory).

### 4. Restart ZPC

Restart ZPC to apply the configuration.

### 5. Send a request to dump the security keys

Discover `home_id` and request the dump (steps 5–6) with:

```bash
python3 scripts/request_security_keys_dump.py --config example_dev_config.yaml \
  --output-dir security_keys_dump/dumps
```

Install dependencies once: `pip3 install -r scripts/requirements-security-keys-dump.txt`.

Alternatively, if you already know `<home_id>`:

```bash
mosquitto_pub -t 'zpc/<home_id>/Network/DumpSecurityKeys' -m '{}'
```

### 6. Verify the dump

The report should contain the status code `0`.

### 7. Decrypt the dump

```bash
python3 scripts/decrypt_security_keys_dump.py --priv <path_to_recipient_priv_key.pem> --in <home_id>.bin --out <output_dir>
```

Two files are generated, named `<home_id>.yaml` and `<home_id>.txt`. By default, they go next to `--in`. Pass `--out <output_dir>` to write them into a specific directory (which must already exist).

- `<home_id>.yaml`: human-readable YAML map of the currently-assigned keys as `<class_name>: '<hex_encoded_key>'`:

```yaml
S0: '0123456789ABCDEF0123456789ABCDEF'
S2_UNAUTHENTICATED: 'FEDCBA98765432100123456789ABCDEF'
...
```

- `<home_id>.txt`: the legacy Zniffer-friendly key list, one line per key as `<class_id>;<hex_encoded_key>;1`. `class_id` is `98` for the S0 network key and `9F` for every S2 class:

```text
98;0123456789ABCDEF0123456789ABCDEF;1
9F;FEDCBA98765432100123456789ABCDEF;1
...
```

Only currently-assigned keys are emitted.

## Envelope (encrypted binary dump) format on disk

| Byte Offset | Size | Field | Description |
|-------------|------|-------|-------------|
| 0 | 4 | magic | "ZPCK" (0x5a, 0x50, 0x43, 0x4b) |
| 4 | 1 | version | 0x01 |
| 5 | 32 | zpc_eph_pub | ZPC's ephemeral X25519 public key |
| 37 | 12 | iv | AES-GCM IV |
| 49 | N | ciphertext | AES-GCM ciphertext |
| 49+N | 16 | auth_tag | AES-GCM authentication tag |


## Dump encryption algorithm

```
zpc_eph_priv, zpc_eph_pub := X25519_keygen()
shared_secret             := X25519(zpc_eph_priv, recipient_pub_key)
aes_key                   := HKDF-SHA256(IKM=shared_secret,
                                         salt=zpc_eph_pub || recipient_pub_key,
                                         info=<hkdf_info>,
                                         length=32)
iv                        := CSPRNG(12 B)
aad                       := magic || version || zpc_eph_pub || recipient_pub_key
ciphertext, auth_tag      := AES-256-GCM-Encrypt(aes_key, iv, aad, plaintext)
```

`hkdf_info` binds the derived AES key to this specific protocol so a shared X25519 secret cannot be reused to derive a usable key in an unrelated context. It is not carried in the envelope thus the decryptor must already know which `hkdf_info` corresponds to which `version` byte. For example, for `version = 0x01`, `hkdf_info = b"zpc-keydump-v1"`. Any future change to `hkdf_info` must come with a bump to `version` so older decryptors fail at the version check rather than producing a silent AES-GCM tag validation failure.

## Dump decryption algorithm

```
shared_secret     := X25519(recipient_priv_key, zpc_eph_pub)
aes_key           := HKDF-SHA256(IKM=shared_secret,
                                 salt=zpc_eph_pub || recipient_pub_key,
                                 info=<hkdf_info>,
                                 length=32)
aad               := magic || version || zpc_eph_pub || recipient_pub_key
plaintext         := AES-256-GCM-Decrypt(aes_key, iv, aad, ciphertext, auth_tag)
```
