# Changelog

## [0.2.0] - 2025-09-02
### Added
- Thread-safe initialization of node ID using FreeRTOS mutex.
- Support for custom identity (e.g., public key) as input to node_id_init.
- `node_id_force_init` API for forced re-initialization (unsafe, see docs).

### Changed
- Documentation updated to clarify thread safety and API usage.

### Fixed
- Internal buffer is never freed; always valid for process lifetime.

### Warning
- Forced re-initialization (`node_id_force_init`) is inherently unsafe if other modules are using the node ID.
