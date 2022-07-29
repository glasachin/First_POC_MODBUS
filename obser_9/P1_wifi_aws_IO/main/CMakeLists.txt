set(PROJECT_NAME "modbus_master")

idf_component_register(SRCS "master.c"
                        INCLUDE_DIRS "."
                        EMBED_TXTFILES "files/AmazonRootCA1.pem"
                                        "files/obser7certificate.pem.crt"
                                        "files/obser7private.pem.key")
