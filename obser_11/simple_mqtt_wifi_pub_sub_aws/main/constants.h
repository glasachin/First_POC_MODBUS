
static const char *TAG = "3CIOT_Product_1";

//--------Wi-Fi related-----------
/*-------------------Wi-Fi Connection Part--------------------*/
// #define WIFI_SSID      "JCBRO"
// #define WIFI_PASS      "jcbro@321"
#define WIFI_SSID      "sachin"
#define WIFI_PASS      "Sachin@dadri3"
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

//--------------------AWS-MQTT-----------------
char mqtt_send_topic[] = "3ciot/product/ABCDEFGHIJ/pump-update";
char mqtt_recv_topic[] = "3ciot/product/ABCDEFGHIJ/pump-status";
char AWS_URL[] = "mqtts://a3v9x3l7dfviqg-ats.iot.us-east-1.amazonaws.com";
uint32_t AWS_PORT = 8883;
char AWS_CLIENT_ID[] = "obser_7";



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