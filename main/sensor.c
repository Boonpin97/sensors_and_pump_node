#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <driver/gpio.h>
#include <esp_adc_cal.h>
#include <dht.h>


#define SENSOR_TYPE DHT_TYPE_DHT11
#define RELAY_GPIO 23


void dht_test(void *pvParameters)
{
    float temperature, humidity;

    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);

    while (1)
    {
        if (dht_read_float_data(SENSOR_TYPE, 15, &humidity, &temperature) == ESP_OK)
            printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
        else
            printf("Could not read data from sensor\n");

        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void water_sensor(){
    int raw_value;
    float voltage;
    bool isWet = 0;

    adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
    
    while(1){
        raw_value = adc1_get_raw(ADC1_CHANNEL_5);
        voltage = (raw_value/4096.0)*3300.0;
        if(voltage <2000){
            isWet = 0;
        }
        else if (voltage>2300)
        {
            isWet = 1;
        }
        
        printf("Voltage: %f| Water detected: %d\n", voltage, isWet);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }   
}

void relay_control(void *pvParameter)
{
    // Configure the GPIO pin for the relay as output
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);

    while (1)
    {
        // Turn ON the relay (set GPIO pin to HIGH)
        gpio_set_level(RELAY_GPIO, 1);
        printf("Relay is ON\n");
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2 seconds

        // Turn OFF the relay (set GPIO pin to LOW)
        gpio_set_level(RELAY_GPIO, 0);
        printf("Relay is OFF\n");
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2 seconds
    }
}

void app_main()
{
    dht_test(NULL);
    
    //xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    //xTaskCreate(water_sensor, "water_sensor", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    //xTaskCreate(relay_control, "relay_control", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}