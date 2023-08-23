/**
 * @file zb_electricity_meter_update.h
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// Multiple inclusion protection
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* Typedef to choose phase to update */
typedef enum {
    PhaseA,
    PhaseB,
    PhaseC
} phase_t;

/**
 * @brief Update total active power of electrical measurement cluster
 * 
 * @param power Power to update
 * @return esp_err_t 
 */
esp_err_t zb_update_total_active_power(int32_t power);

/**
 * @brief Update voltage value of electrical measurement cluster
 * 
 * @param phase Which phase to update
 * @param voltage Voltage to update
 * @return esp_err_t 
 */
esp_err_t zb_update_voltage(phase_t phase, int16_t voltage);

/**
 * @brief Update voltage value of electrical measurement cluster
 * 
 * @param phase Which phase to update
 * @param current Current to update
 * @return esp_err_t 
 */
esp_err_t zb_update_current(phase_t phase, int16_t current);

#ifdef __cplusplus
}
#endif