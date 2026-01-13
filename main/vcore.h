#ifndef VCORE_H
#define VCORE_H

#include "esp_err.h"

esp_err_t vcore_init(void);
void vcore_set_target(float volts);
void vcore_update(void);

#endif // VCORE_H
