# node_id (ESP-IDF component)

Small ESP-IDF component that generates a short, human-friendly device ID from the factory MAC.

- Format: `XXXX-XXXC` (Crockford Base32 with 1-char checksum)
- Stable across reboots/updates (derived from factory MAC)
- No dynamic allocation; returns pointer to internal static buffer
- Lightweight dependencies only (efuse + CRC)

## API

```c
#include "node_id.h"
esp_err_t get_node_id(char **node_id, size_t *len);
```

- `node_id` will point to an internal static buffer containing a null-terminated string.
- `len` will contain the length in bytes (excluding the null terminator).

## Usage

Add this component to your project (as a subdirectory or via the component registry). Then:

```c
#include "node_id.h"

char *id = NULL;
size_t len = 0;
if (get_node_id(&id, &len) == ESP_OK) {
    printf("Node ID (%d): %s\n", (int)len, id);
}
```

A minimal example app is provided in `examples/usage/`.

## Requirements

- ESP-IDF >= 5.0

## License

ISC. See `LICENSE`.
