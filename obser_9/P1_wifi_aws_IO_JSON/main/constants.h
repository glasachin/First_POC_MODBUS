
static const char *TAG = "3CIOT_Product_1";

//--------Wi-Fi related-----------
/*-------------------Wi-Fi Connection Part--------------------*/
#define WIFI_SSID      "JCBRO"
#define WIFI_PASS      "jcbro@321"
#define MAXIMUM_RETRY  3
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//------AWS Certificates Information-----------------
extern const uint8_t cert_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t cert_end[]   asm("_binary_AmazonRootCA1_pem_end");
extern const uint8_t certificate_start[] asm("_binary_obser7certificate_pem_crt_start");
extern const uint8_t certificate_end[]   asm("_binary_obser7certificate_pem_crt_end");
extern const uint8_t private_start[] asm("_binary_obser7private_pem_key_start");
extern const uint8_t private_end[]   asm("_binary_obser7private_pem_key_end");

//-----------TOPIC and other http client configurations---------
// char aws_iot_url[] = "https://a3v9x3l7dfviqg-ats.iot.us-east-1.amazonaws.com:8443/topics/client/100/motor/101/?qos=1";
char aws_iot_url[] = "https://a3v9x3l7dfviqg-ats.iot.us-east-1.amazonaws.com:8443/topics/3ciot/product/ABCDEFGHIJ/pump-update?qos=1";
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
    .url = aws_iot_url,
    .method = HTTP_METHOD_POST,
    .cert_pem = (const char *)cert_start,
    .client_cert_pem = (const char *)certificate_start,
    .client_key_pem = (const char *)private_start,
    .event_handler = client_event_post_handler
};

//--------Pump Structure--------
#define P_IN_LEN 4
#define P_OUT_LEN 2
#define SP_IN_LEN 4
#define SP_OUT_LEN 4

typedef struct PUMP
{
    int input[P_IN_LEN];
    int output[P_OUT_LEN];
} pump; 

typedef struct PUMP_PREV
{
    int input[P_IN_LEN];
} pump_prev;

typedef struct SPARE
{
    int input[SP_IN_LEN];
    int output[SP_OUT_LEN];
} spare;

typedef struct SPARE_PREV
{
    int input[SP_IN_LEN];
} spare_prev;

 int P1_IN_PINS[P_IN_LEN] = {12,13,14,15};
 int P1_OUT_PINS[P_OUT_LEN] = {26,27};
 int P2_IN_PINS[P_IN_LEN] = {34,35,36,39};
 int P2_OUT_PINS[P_OUT_LEN] = {32,33};
 int SPARE_IN_PINS[SP_IN_LEN] = {18,19,21,22};
 int SPARE_OUT_PINS[SP_OUT_LEN] = {2,4,5,23};

 //-----------JSON Payload Related----------
 char product_id[] = "ABCDEFGHIJ";
 char registration_code[] = "1234567890";
 char channel_id_1[] = "1000";
 char channel_id_2[] = "2000";
 char spare_id[] = "spare";