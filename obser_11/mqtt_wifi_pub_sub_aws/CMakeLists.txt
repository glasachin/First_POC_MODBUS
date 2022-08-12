# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/protocols/modbus/mb_example_common)
set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common"
						"$ENV{IDF_PATH}/examples/common_components/esp-aws-iot/libraries/backoffAlgorithm"
						"$ENV{IDF_PATH}/examples/common_components/esp-aws-iot/libraries/coreMQTT"
						"$ENV{IDF_PATH}/examples/common_components/esp-aws-iot/libraries/common/posix_compat"
	)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/files/AmazonRootCA1.pem" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/files/obser7certificate.pem.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/files/obser7private.pem.key" TEXT)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mqtt_wifi_pub_subs_aws)
