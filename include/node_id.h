#ifndef NODE_ID_H
#define NODE_ID_H

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t get_node_id(char **node_id, size_t *len);

#ifdef __cplusplus
}
#endif

#endif