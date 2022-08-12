/*
This firware is to control two pumps.
-------The Master connects to the Wi-Fi with given credentials----------
ssid:
pass:
*/
#include "driver/gpio.h"
#include "string.h"
#include "esp_log.h"
#include "modbus_params.h"  // for modbus parameters structures
#include "mbcontroller.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>
#include "constants.h"
#include "cJSON.h"


char vtg_str[6], cur_str[6];
float voltage, current;
cJSON *root;
cJSON *channels;
cJSON *channel1, *channel2, *spare_channel;
pump P1, P2; // creating PUMP instances
pump_prev P1_prev, P2_prev; // To store previous states
spare SP; // Spare Instance
spare_prev SP_prev; //To Store Previous state
int input_changed = 0; // To store the input change status
static EventGroupHandle_t s_wifi_event_group;
int msg_id_sent = 0;
int msg_id_recv = 0;
int mqtt_sent = 0;
int mqtt_recv = 0;
esp_mqtt_client_handle_t client;
char received_topic[100];
char received_data[500];

static void detect_command(char* );

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client;
    client = event->client;
    int msg_id; 
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        msg_id_sent = esp_mqtt_client_subscribe(client, mqtt_send_topic, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id_sent);

        msg_id_recv = esp_mqtt_client_subscribe(client, mqtt_recv_topic, 1);
        ESP_LOGI(TAG, "receive subscribe successful, msg_id=%d", msg_id_recv);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_sent = 0;
        mqtt_recv = 0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        if (event->msg_id == msg_id_sent)
        {
            mqtt_sent = 1;
            printf("Send Topic subscribed\n");
        }
        if(event->msg_id == msg_id_recv)
        {
            mqtt_recv = 1;
            printf("Receive Topic subscribed\n");
        }
           
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        if (event->msg_id == msg_id_sent)
        {
            mqtt_sent = 0;
            printf("Send Topic unsubscribed\n");
        }
        if(event->msg_id == msg_id_recv)
        {
            mqtt_recv = 0;
            printf("Receive Topic unsubscribed\n");
        }
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        // This will trigger whenever there is data on subscribed topic.
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        // printf("Raw Topic: %s", event->topic);
        sprintf(received_topic,"%.*s", event->topic_len, event->topic);
        if(strcmp(received_topic,mqtt_recv_topic) == 0)
        {
            // printf("Received DATA=%.*s\r\n", event->data_len, event->data);
            sprintf(received_data,"%.*s",event->data_len, event->data);
            detect_command(received_data);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = AWS_URL,
        .port = AWS_PORT,
        .client_id = AWS_CLIENT_ID,
        .cert_pem = (const char *)cert_start,
        .client_cert_pem = (const char *)certificate_start,
        .client_key_pem = (const char *)private_start,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void publish_to_aws(char* pub_data)
{
    int pub_status;
    if (mqtt_sent == 1)
    {
        pub_status = esp_mqtt_client_publish(client, mqtt_send_topic, pub_data, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", pub_status);
    }
}

static void send_input_data()
{
    root = cJSON_CreateObject();
    //-------Make final JSON Object-------------
    cJSON_AddStringToObject(root, "product_id", product_id);
    cJSON_AddStringToObject(root, "registration_code", registration_code);
    channels = cJSON_AddArrayToObject(root, "channels");
    
    // channels = cJSON_CreateArray();
    channel1 = cJSON_CreateObject();
    channel2 = cJSON_CreateObject();
    spare_channel = cJSON_CreateObject();

    //---Create JSON for Channel 1-------
    cJSON_AddStringToObject(channel1, "channel_id", channel_id_1);
    cJSON_AddStringToObject(channel1, "name", "pump_1");
    cJSON_AddNumberToObject(channel1, "power", P1.input[0]);
    cJSON_AddNumberToObject(channel1, "status", P1.input[1]);
    cJSON_AddNumberToObject(channel1, "dry_run", P1.input[2]);
    cJSON_AddNumberToObject(channel1, "trip", P1.input[3]);

    //---Create JSON for Channel 1-------
    cJSON_AddStringToObject(channel2, "channel_id", channel_id_2);
    cJSON_AddStringToObject(channel2, "name", "pump_2");
    cJSON_AddNumberToObject(channel2, "power", P2.input[0]);
    cJSON_AddNumberToObject(channel2, "status", P2.input[1]);
    cJSON_AddNumberToObject(channel2, "dry_run", P2.input[2]);
    cJSON_AddNumberToObject(channel2, "trip", P2.input[3]);

    //---Create JSON for Spare-------
    cJSON_AddStringToObject(spare_channel, "channel_id", spare_id);
    cJSON_AddStringToObject(spare_channel, "name", "spare");
    cJSON_AddNumberToObject(spare_channel, "sp_in_0", SP.input[0]);
    cJSON_AddNumberToObject(spare_channel, "sp_in_1", SP.input[1]);
    cJSON_AddNumberToObject(spare_channel, "sp_in_2", SP.input[2]);
    cJSON_AddNumberToObject(spare_channel, "sp_in_3", SP.input[3]);

    //--------create array of object--------
    cJSON_AddItemToArray(channels, channel1);
    cJSON_AddItemToArray(channels, channel2);
    cJSON_AddItemToArray(channels, spare_channel);

    // char  *post_data = "{\"voltage\":\"3.14\",\"current\":\"5.46\"}";
    char *post_data = cJSON_Print(root);
    publish_to_aws(post_data);
}

static void detect_command(char* cmd)
{
    printf("Received Data is: %s", cmd);

    
}

static void read_input_task(void *pvParameters)
{
    int in_pin;

    while(1)
    {
        // Read the status of INPUT pins and store them
        printf("Reading INPUT PINS\n");

        // Read INPUT PINS of pumps
        for(in_pin = 0; in_pin < P_IN_LEN; in_pin++)
        {
            P1.input[in_pin] = gpio_get_level(P1_IN_PINS[in_pin]);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            P2.input[in_pin] = gpio_get_level(P2_IN_PINS[in_pin]);
            vTaskDelay(1 / portTICK_PERIOD_MS);
        } 

        // Read INPUT PINS of SPARE
        for(in_pin = 0; in_pin < SP_IN_LEN; in_pin++)
        {
            SP.input[in_pin] = gpio_get_level(SPARE_IN_PINS[in_pin]);
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }

        vTaskDelay(1000*120 / portTICK_PERIOD_MS);
    }

}

static void input_change_task(void *pvParameters)
{
    int check_pin;
    while(1)
    {
        // Check if there is any change in the input pin value
        printf("Checking the change in Input\n");
        
        for(check_pin = 0; check_pin < P_IN_LEN; check_pin++)
        {
            if(P1_prev.input[check_pin] != P1.input[check_pin])
            {
                printf("Change Detected ==> Prev P1.%d = %d, Current P1.%d = %d\n",check_pin,P1_prev.input[check_pin],
                    check_pin, P1.input[check_pin]);
                input_changed = 1;
                break;
            }
            else if(P2_prev.input[check_pin] != P2.input[check_pin])
            {
                printf("Change Detected ==> Prev P2.%d = %d, Current P2.%d = %d\n",check_pin,P2_prev.input[check_pin],
                    check_pin, P2.input[check_pin]);
                input_changed = 1;
                break;
            }
            else
                continue;
        }

        for(check_pin = 0; check_pin < SP_IN_LEN; check_pin++)
        {
            if(SP_prev.input[check_pin] != SP.input[check_pin])
            {
                printf("Change Detected ==> Prev SP.%d = %d, Current SP.%d = %d\n",check_pin,SP_prev.input[check_pin],
                    check_pin, SP.input[check_pin]);
                input_changed = 1;
                break;
            }
            else
                continue;
        }

        
        // assign current PIN values to the Prev pin value buffer
        for(check_pin = 0; check_pin < P_IN_LEN; check_pin++)
        {
            P1_prev.input[check_pin] = P1.input[check_pin];
            P2_prev.input[check_pin] = P2.input[check_pin];
        }

        for(check_pin = 0; check_pin < SP_IN_LEN; check_pin++)
        {
            SP_prev.input[check_pin] = SP.input[check_pin];
        }
        
        vTaskDelay(1000*120 / portTICK_PERIOD_MS);
    }

}

static void alive_payload_task(void *pvParameters)
{
    while(1)
    {
        // Read the status of INPUT pins and store them
        printf("Module is Alive and Running\n");
        publish_to_aws("{\"alive\":\"True\"}");
        vTaskDelay(1000*60 / portTICK_PERIOD_MS);
    }

}

void system_init()
{
    int i;
    //-------wi-fi-------
     //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // -------MQTT Related------------
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default()); //already done in wifi
    mqtt_app_start();

    // Set Direction of Input-Output Pins
    for (i = 0; i < P_IN_LEN; i++)
    {
        gpio_reset_pin(P1_IN_PINS[i]);
        gpio_set_direction(P1_IN_PINS[i], GPIO_MODE_INPUT);
        gpio_pullup_en(P1_IN_PINS[i]);

        gpio_reset_pin(P2_IN_PINS[i]);
        gpio_set_direction(P2_IN_PINS[i], GPIO_MODE_INPUT);
        gpio_pullup_en(P2_IN_PINS[i]);
    }

    for (i = 0; i < P_OUT_LEN; i++)
    {
        gpio_reset_pin(P1_OUT_PINS[i]);
        gpio_set_direction(P1_OUT_PINS[i], GPIO_MODE_OUTPUT);

        gpio_reset_pin(P2_OUT_PINS[i]);
        gpio_set_direction(P2_OUT_PINS[i], GPIO_MODE_OUTPUT);
    }

    for (i = 0; i < SP_IN_LEN; i++)
    {
        gpio_reset_pin(SPARE_IN_PINS[i]);
        gpio_set_direction(SPARE_IN_PINS[i], GPIO_MODE_INPUT);
        gpio_pullup_en(SPARE_IN_PINS[i]);
    }

    for (i = 0; i < SP_OUT_LEN; i++)
    {
        gpio_reset_pin(SPARE_OUT_PINS[i]);
        gpio_set_direction(SPARE_OUT_PINS[i], GPIO_MODE_OUTPUT);
    }

}

void app_main(void)
{
    system_init();
    send_input_data();
    // Task to keep reading the Input Status after every 1sec
    xTaskCreate(&read_input_task, "Read_INPUT_PINS", 1024, NULL, 5, NULL);
    // Task to keep sending the Output to Pins after every 1sec

    // Task to keep track of the input change. This should change the flag so that information
    // Information can be sent to cloud immediately.
    xTaskCreate(&input_change_task, "INPUT_DATA_CHAGE_CHECK", 2*1024, NULL, 5, NULL);
    
    // Task to get the topic from CLOUD and decode the value. This should change the flash so that
    // corresponding action can be taken.

    // Task to send alive payload to the Cloud after every 1 min.
    xTaskCreate(&alive_payload_task, "MODULE_ALIVE_TASK", 3*1024, NULL, 5, NULL);

    while(1)
    {
        //1. Keep checking flag to send the data to cloud if there is any change.
        if(input_changed == 1)
        {
            // Send data to the cloud
            printf("Sending Data to Cloud");
            send_input_data();
            input_changed = 0;
        }
        
        //2. 
        

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
