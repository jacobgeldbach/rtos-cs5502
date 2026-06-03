#include "freertos/semphr.h"

extern SemaphoreHandle_t display_mutex;
extern SemaphoreHandle_t count_mutex;
extern SemaphoreHandle_t count_enabled;
extern int shared_count;

