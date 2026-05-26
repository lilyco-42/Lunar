#pragma once
#include <stddef.h>
#include <stdint.h>

void su03t_init(void);
void su03t_send(const uint8_t *data, size_t len);
void su03t_start_monitor(void);
void su03t_on_cmd(void (*cb)(int msgid));
void su03t_speak_temp(float temp);
void su03t_speak_humi(int humi);
