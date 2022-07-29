
static const char *TAG = "3CIOT_Product_1";

//--------Wi-Fi related-----------
/*-------------------Wi-Fi Connection Part--------------------*/
#define WIFI_SSID      "JCBRO"
#define WIFI_PASS      "jcbro@321"
#define MAXIMUM_RETRY  3

//------AWS Certificates Information-----------------
extern const uint8_t cert_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t cert_end[]   asm("_binary_AmazonRootCA1_pem_end");
extern const uint8_t certificate_start[] asm("_binary_obser7certificate_pem_crt_start");
extern const uint8_t certificate_end[]   asm("_binary_obser7certificate_pem_crt_end");
extern const uint8_t private_start[] asm("_binary_obser7private_pem_key_start");
extern const uint8_t private_end[]   asm("_binary_obser7private_pem_key_end");

//-----------TOPIC and other http client configurations---------
/*----HTTP Related----*/
esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}

esp_http_client_config_t config_post = {
    .url = "https://a3v9x3l7dfviqg-ats.iot.us-east-1.amazonaws.com:8443/topics/client/100/motor/101/?qos=1",
    .method = HTTP_METHOD_POST,
    .cert_pem = (const char *)cert_start,
    .client_cert_pem = (const char *)certificate_start,
    .client_key_pem = (const char *)private_start,
    .event_handler = client_event_post_handler
};