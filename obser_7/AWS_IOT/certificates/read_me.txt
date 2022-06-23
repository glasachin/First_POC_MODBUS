end-points: a3v9x3l7dfviqg-ats.iot.us-east-1.amazonaws.com

------------------------------------------------------------------------
HTTPS Link format: https://IoT_data_endpoint/topics/url_encoded_topic_name?qos=1"
Curl command (use_this_command to send data to topic on AWS IoT)

Example:
curl --tlsv1.2 \
    --cacert Amazon-root-CA-1.pem \
    --cert device.pem.crt \
    --key private.pem.key \
    --request POST \
    --data "{ \"message\": \"Hello, world\" }" \
    "https://IoT_data_endpoint:8443/topics/topic?qos=1"


curl --tlsv1.2 --cacert AmazonRootCA1.pem --cert obser7certificate.pem.crt --key obser_7-private.pem.key --request POST --data "{ \"message\": \"Sachin Sharma\" }" "https://a3v9x3l7dfviqg-ats.iot.us-east-1.amazonaws.com:8443/topics/obser_7_topic?qos=1"

------------------------------------------------------------------------
