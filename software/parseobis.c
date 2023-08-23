static esp_err_t parse_obis(uint8_t* obis_data, size_t obis_data_size)
{
    /* Check for obis data integrity */
    if(obis_data[OBIS_INTEGRITY_VAL1_OFFSET] != OBIS_INTEGRITY_VAL1 || obis_data[OBIS_INTEGRITY_VAL2_OFFSET] != OBIS_INTEGRITY_VAL2)
    {
        ESP_LOGE(TAG, "OBIS: Decrypted obis data invalid");
        return ESP_FAIL;
    }

    /* Decode data */
    int curr_offset = 0;



    /* COPIED FROM SOMEWHERE ELSE!!!!!!!!!!! */
    ESP_LOGV(TAG, "Decoding payload");

    int currentPosition = DECODER_START_OFFSET;

    do
    {
        if(plaintext[currentPosition + OBIS_TYPE_OFFSET] != DataType::OctetString)
        {
            ESP_LOGE(TAG, "OBIS: Unsupported OBIS header type");
            return abort();
        }

        uint8_t obisCodeLength = plaintext[currentPosition + OBIS_LENGTH_OFFSET];

        if(obisCodeLength != 0x06)
        {
            ESP_LOGE(TAG, "OBIS: Unsupported OBIS header length");
            return abort();
        }

        uint8_t obisCode[obisCodeLength];
        memcpy(&obisCode[0], &plaintext[currentPosition + OBIS_CODE_OFFSET], obisCodeLength); // Copy OBIS code to array

        currentPosition += obisCodeLength + 2; // Advance past code, position and type

        uint8_t dataType = plaintext[currentPosition];
        currentPosition++; // Advance past data type

        uint8_t dataLength = 0x00;

        CodeType codeType = CodeType::Unknown;

        if(obisCode[OBIS_A] == Medium::Electricity)
        {
            // Compare C and D against code
            if(memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L1, 2) == 0)
            {
                codeType = CodeType::VoltageL1;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L2, 2) == 0)
            {
                codeType = CodeType::VoltageL2;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L3, 2) == 0)
            {
                codeType = CodeType::VoltageL3;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L1, 2) == 0)
            {
                codeType = CodeType::CurrentL1;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L2, 2) == 0)
            {
                codeType = CodeType::CurrentL2;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L3, 2) == 0)
            {
                codeType = CodeType::CurrentL3;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_POWER_PLUS, 2) == 0)
            {
                codeType = CodeType::ActivePowerPlus;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_POWER_MINUS, 2) == 0)
            {
                codeType = CodeType::ActivePowerMinus;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_ENERGY_PLUS, 2) == 0)
            {
                codeType = CodeType::ActiveEnergyPlus;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_ENERGY_MINUS, 2) == 0)
            {
                codeType = CodeType::ActiveEnergyMinus;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_REACTIVE_ENERGY_PLUS, 2) == 0)
            {
                codeType = CodeType::ReactiveEnergyPlus;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_REACTIVE_ENERGY_MINUS, 2) == 0)
            {
                codeType = CodeType::ReactiveEnergyMinus;
            }
            else
            {
                ESP_LOGW(TAG, "OBIS: Unsupported OBIS code");
            }
        }
        else if(obisCode[OBIS_A] == Medium::Abstract)
        {
            if(memcmp(&obisCode[OBIS_C], ESPDM_TIMESTAMP, 2) == 0)
            {
                codeType = CodeType::Timestamp;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_SERIAL_NUMBER, 2) == 0)
            {
                codeType = CodeType::SerialNumber;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_DEVICE_NAME, 2) == 0)
            {
                codeType = CodeType::DeviceName;
            }
            else
            {
                ESP_LOGW(TAG, "OBIS: Unsupported OBIS code");
            }
        }
        else
        {
            ESP_LOGE(TAG, "OBIS: Unsupported OBIS medium");
            return abort();
        }

        uint8_t uint8Value;
        uint16_t uint16Value;
        uint32_t uint32Value;
        float floatValue;

        switch(dataType)
        {
            case DataType::DoubleLongUnsigned:
                dataLength = 4;

                memcpy(&uint32Value, &plaintext[currentPosition], 4); // Copy bytes to integer
                uint32Value = swap_uint32(uint32Value); // Swap bytes

                floatValue = uint32Value; // Ignore decimal digits for now

                if(codeType == CodeType::ActivePowerPlus && this->active_power_plus != NULL && this->active_power_plus->state != floatValue)
                    this->active_power_plus->publish_state(floatValue);
                else if(codeType == CodeType::ActivePowerMinus && this->active_power_minus != NULL && this->active_power_minus->state != floatValue)
                    this->active_power_minus->publish_state(floatValue);

                else if(codeType == CodeType::ActiveEnergyPlus && this->active_energy_plus != NULL && this->active_energy_plus->state != floatValue)
                    this->active_energy_plus->publish_state(floatValue);
                else if(codeType == CodeType::ActiveEnergyMinus && this->active_energy_minus != NULL && this->active_energy_minus->state != floatValue)
                    this->active_energy_minus->publish_state(floatValue);

                else if(codeType == CodeType::ReactiveEnergyPlus && this->reactive_energy_plus != NULL && this->reactive_energy_plus->state != floatValue)
                    this->reactive_energy_plus->publish_state(floatValue);
                else if(codeType == CodeType::ReactiveEnergyMinus && this->reactive_energy_minus != NULL && this->reactive_energy_minus->state != floatValue)
                    this->reactive_energy_minus->publish_state(floatValue);

                break;
            case DataType::LongUnsigned:
                dataLength = 2;

                memcpy(&uint16Value, &plaintext[currentPosition], 2); // Copy bytes to integer
                uint16Value = swap_uint16(uint16Value); // Swap bytes

                if(plaintext[currentPosition + 5] == Accuracy::SingleDigit)
                    floatValue = uint16Value / 10.0; // Divide by 10 to get decimal places
                else if(plaintext[currentPosition + 5] == Accuracy::DoubleDigit)
                    floatValue = uint16Value / 100.0; // Divide by 100 to get decimal places
                else
                    floatValue = uint16Value; // No decimal places

                if(codeType == CodeType::VoltageL1 && this->voltage_l1 != NULL && this->voltage_l1->state != floatValue)
                    this->voltage_l1->publish_state(floatValue);
                else if(codeType == CodeType::VoltageL2 && this->voltage_l2 != NULL && this->voltage_l2->state != floatValue)
                    this->voltage_l2->publish_state(floatValue);
                else if(codeType == CodeType::VoltageL3 && this->voltage_l3 != NULL && this->voltage_l3->state != floatValue)
                    this->voltage_l3->publish_state(floatValue);

                else if(codeType == CodeType::CurrentL1 && this->current_l1 != NULL && this->current_l1->state != floatValue)
                    this->current_l1->publish_state(floatValue);
                else if(codeType == CodeType::CurrentL2 && this->current_l2 != NULL && this->current_l2->state != floatValue)
                    this->current_l2->publish_state(floatValue);
                else if(codeType == CodeType::CurrentL3 && this->current_l3 != NULL && this->current_l3->state != floatValue)
                    this->current_l3->publish_state(floatValue);

                break;
            case DataType::OctetString:
                dataLength = plaintext[currentPosition];
                currentPosition++; // Advance past string length

                if(codeType == CodeType::Timestamp) // Handle timestamp generation
                {
                    char timestamp[21]; // 0000-00-00T00:00:00Z

                    uint16_t year;
                    uint8_t month;
                    uint8_t day;

                    uint8_t hour;
                    uint8_t minute;
                    uint8_t second;

                    memcpy(&uint16Value, &plaintext[currentPosition], 2);
                    year = swap_uint16(uint16Value);

                    memcpy(&month, &plaintext[currentPosition + 2], 1);
                    memcpy(&day, &plaintext[currentPosition + 3], 1);

                    memcpy(&hour, &plaintext[currentPosition + 5], 1);
                    memcpy(&minute, &plaintext[currentPosition + 6], 1);
                    memcpy(&second, &plaintext[currentPosition + 7], 1);

                    sprintf(timestamp, "%04u-%02u-%02uT%02u:%02u:%02uZ", year, month, day, hour, minute, second);

                    this->timestamp->publish_state(timestamp);
                }

                break;
            default:
                ESP_LOGE(TAG, "OBIS: Unsupported OBIS data type");
                return abort();
                break;
        }

        currentPosition += dataLength; // Skip data length

        currentPosition += 2; // Skip break after data

        if(plaintext[currentPosition] == 0x0F) // There is still additional data for this type, skip it
               currentPosition += 6; // Skip additional data and additional break; this will jump out of bounds on last frame
    }
    while (currentPosition <= messageLength); // Loop until arrived at end

    this->receiveBuffer.clear(); // Reset buffer

    ESP_LOGI(TAG, "Received valid data");

    /* PRINT TO MQTT, CHANGE TO ZIGBEE*/
    if(this->mqtt_client != NULL)
    {
        this->mqtt_client->publish_json(this->topic, [=](JsonObject root)
        {
            if(this->voltage_l1 != NULL)
            {
                root["voltage_l1"] = this->voltage_l1->state;
                root["voltage_l2"] = this->voltage_l2->state;
                root["voltage_l3"] = this->voltage_l3->state;
            }
            if(this->current_l1 != NULL)
            {
                root["current_l1"] = this->current_l1->state;
                root["current_l2"] = this->current_l2->state;
                root["current_l3"] = this->current_l3->state;
            }
            if(this->active_power_plus != NULL)
            {
                root["active_power_plus"] = this->active_power_plus->state;
                root["active_power_minus"] = this->active_power_minus->state;
            }
            if(this->active_energy_plus != NULL)
            {
                root["active_energy_plus"] = this->active_energy_plus->state;
                root["active_energy_minus"] = this->active_energy_minus->state;
            }
            if(this->reactive_energy_plus != NULL)
            {
                root["reactive_energy_plus"] = this->reactive_energy_plus->state;
                root["reactive_energy_minus"] = this->reactive_energy_minus->state;
            }
            if(this->timestamp != NULL)
            {
                root["timestamp"] = this->timestamp->state;
            }
        });
    }

    return ESP_OK;
}