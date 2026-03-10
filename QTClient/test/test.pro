QT += core testlib sql
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_messagecache

INCLUDEPATH += .. 

SOURCES += \
    # tst_messagesyncprotocol.cpp \
    tst_messagecache.cpp \
    ../messagecachedb.cpp \
    # ../messagecacherepository.cpp

HEADERS += \
    ../messagecachedb.h \
    # ../messagecacherepository.h \
    ../userdata.h \
    ../messagesyncprotocol.h \
