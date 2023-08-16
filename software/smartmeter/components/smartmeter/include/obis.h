/**
 * @file dlms.h
 * @brief Many things are copied from: https://github.com/DomiStyle/esphome-dlms-meter
 * @copyright Copyright (c) 2023
 * 
 */

// Multiple inclusion protection
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ===== FOR DATA INTEGRITY CHECK ===== */
/* Not sure about the meaning of these values, but they are always the same */
#define OBIS_INTEGRITY_VAL1_OFFSET      0       /* < Position of first value, which is always the same */
#define OBIS_INTEGRITY_VAL1             0x0F    /* < Value of first integrity check */

#define OBIS_INTEGRITY_VAL2_OFFSET      5       /* < Position of second value, which is always the same */
#define OBIS_INTEGRITY_VAL2             0x0C    /* < Value of second integrity check */

/* ===== DATA STRUCTURE ===== */
#define OBIS_DECODER_START_OFFSET = 20;         /* < Offset for start of OBIS decoding, skip header, timestamp and break block */

#define OBIS_TYPE_OFFSET = 0;
#define OBIS_LENGTH_OFFSET = 1;

#define OBIS_CODE_OFFSET = 2;

#define OBIS_A = 0;
#define OBIS_B = 1;
#define OBIS_C = 2;
#define OBIS_D = 3;
#define OBIS_E = 4;
#define OBIS_F = 5;

/* ===== DEFINITIONS ACCORDING TO SPECIFICATION ===== */
/* === Data Types as per specification === */
enum DataType
{
    NullData = 0x00,
    Boolean = 0x03,
    BitString = 0x04,
    DoubleLong = 0x05,
    DoubleLongUnsigned = 0x06,
    OctetString = 0x09,
    VisibleString = 0x0A,
    Utf8String = 0x0C,
    BinaryCodedDecimal = 0x0D,
    Integer = 0x0F,
    Long = 0x10,
    Unsigned = 0x11,
    LongUnsigned = 0x12,
    Long64 = 0x14,
    Long64Unsigned = 0x15,
    Enum = 0x16,
    Float32 = 0x17,
    Float64 = 0x18,
    DateTime = 0x19,
    Date = 0x1A,
    Time = 0x1B,
    Array = 0x01,
    Structure = 0x02,
    CompactArray = 0x13
};

/* === Medium as per specification === */
/* Defines from which type of meter the data is */
enum Medium
{
    Abstract = 0x00,
    Electricity = 0x01,
    Heat = 0x06,
    Gas = 0x07,
    Water = 0x08
};

/* === Code Types as per specification === */
/* Defines which type of measurement the value stems from */
enum CodeType
{
    Unknown,
    Timestamp,
    SerialNumber,
    DeviceName,
    VoltageL1,
    VoltageL2,
    VoltageL3,
    CurrentL1,
    CurrentL2,
    CurrentL3,
    ActivePowerPlus,
    ActivePowerMinus,
    ActiveEnergyPlus,
    ActiveEnergyMinus,
    ReactiveEnergyPlus,
    ReactiveEnergyMinus
};

/* === Accury of measurement as per specification === */
enum Accuracy
{
    ZeroDigit = 0x00,                               /* < Divide measured value by 1 */
    SingleDigit = 0xFF,                             /* < Divide measured value by 10 */
    DoubleDigit = 0xFE,                             /* < Divide measured value by 100*/
    TripleDigit = 0xFD                              /* < Divide measured value by 1000 */
};

/*
 * Metadata
 */

static uint8_t ESPDM_TIMESTAMP[] = 
{
    0x01, 0x00
};

static const uint8_t ESPDM_SERIAL_NUMBER[] = 
{
    0x60, 0x01
};

static const uint8_t ESPDM_DEVICE_NAME[] = 
{
    0x2A, 0x00
};

/*
 * Voltage
 */

static uint8_t ESPDM_VOLTAGE_L1[] = 
{
    0x20, 0x07
};

static const uint8_t ESPDM_VOLTAGE_L2[] = 
{
    0x34, 0x07
};

static const uint8_t ESPDM_VOLTAGE_L3[] = 
{
    0x48, 0x07
};

/*
 * Current
 */

static const uint8_t ESPDM_CURRENT_L1[] = 
{
    0x1F, 0x07
};

static const uint8_t ESPDM_CURRENT_L2[] = 
{
    0x33, 0x07
};

static const uint8_t ESPDM_CURRENT_L3[] = 
{
    0x47, 0x07
};

/*
 * Power
 */

static const uint8_t ESPDM_ACTIVE_POWER_PLUS[] = 
{
    0x01, 0x07
};

static const uint8_t ESPDM_ACTIVE_POWER_MINUS[] = 
{
    0x02, 0x07
};

/*
 * Active energy
 */

static const uint8_t ESPDM_ACTIVE_ENERGY_PLUS[] = 
{
    0x01, 0x08
};
static const uint8_t ESPDM_ACTIVE_ENERGY_MINUS[] = 
{
    0x02, 0x08
};

/*
 * Reactive energy
 */

static const uint8_t ESPDM_REACTIVE_ENERGY_PLUS[] = 
{
    0x03, 0x08
};
static const uint8_t ESPDM_REACTIVE_ENERGY_MINUS[] = 
{
    0x04, 0x08
};


#ifdef __cplusplus
} // extern "C"
#endif
