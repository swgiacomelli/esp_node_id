# esp_node_id (ESP-IDF component)

Small ESP-IDF component that generates a short, human-friendly device ID from the factory MAC by default or a custom identiy (node public key).


- Format: `XXXXX-XXXXX-XXX-C` (Crockford Base32 with 1-char checksum)
- Stable across reboots/updates (derived from factory MAC by default)
- Optionally, you can pass a custom identity (e.g., node public key) to generate a deterministic ID.
- Thread-safe initialization (FreeRTOS mutex).
- No dynamic allocation; returns pointer to internal static buffer (never freed).
- Lightweight dependencies only (efuse, SHA256).

## API


```c
#include "node_id.h"
esp_err_t node_id_init(const uint8_t* identity_str, size_t len); // Thread-safe, fallback to MAC if identity_str==NULL
esp_err_t node_id_force_init(const uint8_t* identity_str, size_t len); // Unsafe: forcibly resets node ID
esp_err_t get_node_id(char **node_id, size_t *len);
```


- `get_node_id` will point to an internal static buffer containing a null-terminated string.
- `len` will contain the length in bytes (excluding the null terminator).
- The buffer is never freed and remains valid for the process lifetime.

## Usage

Add this component to your project (as a subdirectory or via the component registry). Then:

```c
#include "node_id.h"

char *id = NULL;
size_t len = 0;
if (get_node_id(&id, &len) == ESP_OK) {
    printf("Node ID (%d): %s\n", (int)len, id);
}

// Optionally, initialize with a custom identity (e.g., public key):
// node_id_init(pubkey, pubkey_len);

// WARNING: node_id_force_init is inherently unsafe if other modules are using the node ID.
// It will overwrite the internal buffer and may break consumers holding a pointer.
// Always copy the node ID string if you need to preserve it after a force init.
```

## Unit Testing

Unit tests are provided in `test/test_node_id.c` and cover:
- Default node ID generation
- Custom identity support
- Robust thread safety (multiple FreeRTOS tasks)

Run with ESP-IDF Unity test runner for validation.

## Example

A minimal example app is provided in `examples/usage/`.

## Requirements

- ESP-IDF >= 5.0

## Install (from Git)
Repo: https://github.com/swgiacomelli/esp_node_id

Submodule example:
```sh
git submodule add https://github.com/swgiacomelli/esp_node_id components/esp_node_id
```

## License

ISC. See `LICENSE`.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history.
