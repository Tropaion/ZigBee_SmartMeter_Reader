/**
 * @file dlms.h
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// Multiple inclusion protection
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ===== FOR OBIS DATA INTEGRITY CHECK ===== */
/* Not sure about the meaning of these values, but they are always the same */
#define OBIS_INTEGRITY_VAL1             0x0F
#define OBIS_INTEGRITY_VAL1_OFFSET      0

#define OBIS_INTEGRITY_VAL2             0x0C
#define OBIS_INTEGRITY_VAL2_OFFSET      5


#ifdef __cplusplus
} // extern "C"
#endif
