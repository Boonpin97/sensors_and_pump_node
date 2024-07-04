
#include <iostream>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "jsoncpp/value.h"
#include "jsoncpp/json.h"

#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"

#include "wifi_utils.h"

#include "firebase_config.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#include <stdio.h>
#include <driver/gpio.h>
#include <dht.h>

using namespace ESPFirebase;

#define SENSOR_TYPE DHT_TYPE_DHT11
#define RELAY_GPIO GPIO_NUM_23

Json::Value sensor_data, pump_data;
int prev_pump = 0;
int prev_moisture = 0;
bool listen_to_pump = true; // true:pump, false:moisture
bool moisture_bool = false;
// Read Temperature and Humidity
void dht_test(void *pvParameters)
{
    float temperature, humidity;

    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);

    if (dht_read_float_data(SENSOR_TYPE, GPIO_NUM_15, &humidity, &temperature) == ESP_OK)
    {
        printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
        sensor_data["Humidity"] = humidity;
        sensor_data["Temperature"] = temperature;
    }

    else
        printf("Could not read data from sensor\n");

    // while (1)
    // {
    //     if (dht_read_float_data(SENSOR_TYPE, GPIO_NUM_15, &humidity, &temperature) == ESP_OK)
    //         printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
    //     else
    //         printf("Could not read data from sensor\n");

    //     // If you read the sensor data too often, it will heat up
    //     // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    // }
}

// Read Water Sensor
void water_sensor(void *pvParameters)
{
    int raw_value;
    float voltage;
    int isWet = 0;

    adc1_config_width((adc_bits_width_t)ADC_WIDTH_BIT_DEFAULT);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
    raw_value = adc1_get_raw(ADC1_CHANNEL_5);
    voltage = (raw_value / 4096.0) * 3300.0;
    if (voltage < 1000)
    {
        moisture_bool = false;
        isWet = 0;
    }
    else
    {
        moisture_bool = true;
        isWet = 1;
    }

    printf("Voltage: %f| Water detected: %d\n", voltage, isWet);
    sensor_data["Moisture"] = isWet;
}

// Write to relay
void relay_control(RTDB database)
{
    Json::Value root = database.getData("/Actuators"); // retrieve person3 from database, set it to "" to get entire database

    // Configure the GPIO pin for the relay as output
    if (root["Pump"] != prev_pump)
    {
        printf("Setting flag to true\n");
        listen_to_pump = true; // Listent to pump
    }
    else if (moisture_bool != prev_moisture)
    {
        printf("Setting flag to false\n");
        listen_to_pump = false; // Listent to moisture
    }

    prev_pump = root["Pump"].asBool();
    prev_moisture = moisture_bool;
    printf("pump: %d, moisture:%d\n", prev_pump, prev_moisture);
    if (listen_to_pump == true)
    {
        printf("Checking pump\n");
        if (prev_pump == 1)
        {
            gpio_set_level(RELAY_GPIO, 1);
            pump_data["Pump"] = 1;
        }
        else
        {
            gpio_set_level(RELAY_GPIO, 0);
            pump_data["Pump"] = 0;
        }
    }
    else
    {
        printf("Checking moisture\n");
        if (prev_moisture == 1)
        {
            gpio_set_level(RELAY_GPIO, 0);
            pump_data["Pump"] = 0;
        }
        else
        {
            gpio_set_level(RELAY_GPIO, 1);
            pump_data["Pump"] = 1;
        }
    }
    database.putData("Actuators", pump_data);
}

extern "C" void app_main(void)
{
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
    wifiInit(SSID, PASSWORD); // blocking until it connects

    // // Config and Authentication
    FirebaseApp app = FirebaseApp(API_KEY);
    RTDB db = RTDB(&app, DATABASE_URL);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
        dht_test(NULL);
        water_sensor(NULL);
        relay_control(db);
        db.putData("Sensors", sensor_data);
    }

    // Read data
    // Json::Value root = db.getData("/Sensors"); // retrieve person3 from database, set it to "" to get entire database
    // ESP_LOGI("MAIN", "Humidity: %s", root["Humidity"].toStyledString().c_str());
    // ESP_LOGI("MAIN", "Temperature: %s", root["Temperature"].toStyledString().c_str());
    // ESP_LOGI("MAIN", "Moisture: %s", root["Moisture"].toStyledString().c_str());
    // printf("Retrieved data\n");
}
