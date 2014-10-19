# Change this Variables for your project
# TEMPLATE can be 'app' for application, 'so' for shared library, 'ar' for static library
TARGET = libwebsockets-test-server
TEMPLATE = app

SOURCES += ./test/test-server.c

INCLUDEPATH += /home/colin/sdk/vmf_client/include/

INCLUDEPATH += /home/colin/sdk/osal/include/
INCLUDEPATH += /home/colin/sdk/libwebsockets/include/



INSTALL_DIR	:= ../../ai-sdk-forMiddleware-NGI/vgw/

#INCLUDEPATH += ../../../service/osal/public/
#LIBS += -L../../../service/osal/lib/ -lOSAL

LIBS += -L/home/colin/sdk/vmf_client/lib/ -lvmf_client
LIBS += -L/home/colin/sdk/vmflibwebsockets/lib/ -lwebsockets


