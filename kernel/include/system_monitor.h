#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <stdint.h>

/* Sistem izlencesini baslat (modal app) */
void app_system_monitor(void);

/* CPU kullanim bilgisi (kernel idle hook icin) */
uint32_t sysmon_get_cpu_usage(void);
uint32_t sysmon_get_heap_used(void);
uint32_t sysmon_get_heap_total(void);

#endif /* SYSTEM_MONITOR_H */
