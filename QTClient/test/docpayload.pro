QT += core testlib widgets
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_docpayload

INCLUDEPATH += ..

SOURCES += \
    tst_docpayload.cpp \
    ../docpayload.cpp \
    ../onlyofficeurl.cpp \
    ../filebubble.cpp \
    ../bubbleframe.cpp

HEADERS += \
    ../docpayload.h \
    ../onlyofficeurl.h \
    ../filebubble.h \
    ../bubbleframe.h \
    ../global.h
