QT       += core gui \
    quick

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    adduseritem.cpp \
    applyfriend.cpp \
    applyfrienditem.cpp \
    applyfriendlist.cpp \
    applyfriendpage.cpp \
    authenfriend.cpp \
    bubbleframe.cpp \
    chatdialog.cpp \
    chatitembase.cpp \
    chatpage.cpp \
    chatuserlist.cpp \
    chatuserwidget.cpp \
    chatview.cpp \
    clickedbtn.cpp \
    clickedlabel.cpp \
    clickedoncelabel.cpp \
    contactuseritem.cpp \
    contactuserlist.cpp \
    customedit.cpp \
    findfaildlg.cpp \
    findsuccessdialog.cpp \
    friendlabel.cpp \
    global.cpp \
    grouptipitem.cpp \
    httpmanager.cpp \
    listitembase.cpp \
    loadingdialog.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    messagetextedit.cpp \
    picturebubble.cpp \
    register.cpp \
    resetdialog.cpp \
    searchlist.cpp \
    statewidget.cpp \
    tcpmanager.cpp \
    textbubble.cpp \
    timerbtn.cpp \
    usermanager.cpp

HEADERS += \
    adduseritem.h \
    applyfriend.h \
    applyfrienditem.h \
    applyfriendlist.h \
    applyfriendpage.h \
    authenfriend.h \
    bubbleframe.h \
    chatdialog.h \
    chatitembase.h \
    chatpage.h \
    chatuserlist.h \
    chatuserwidget.h \
    chatview.h \
    clickedbtn.h \
    clickedlabel.h \
    clickedoncelabel.h \
    contactuseritem.h \
    contactuserlist.h \
    customedit.h \
    findfaildlg.h \
    findsuccessdialog.h \
    friendlabel.h \
    global.h \
    grouptipitem.h \
    httpmanager.h \
    listitembase.h \
    loadingdialog.h \
    login.h \
    mainwindow.h \
    messagetextedit.h \
    picturebubble.h \
    register.h \
    resetdialog.h \
    searchlist.h \
    singleton.h \
    statewidget.h \
    tcpmanager.h \
    textbubble.h \
    timerbtn.h \
    userdata.h \
    usermanager.h

FORMS += \
    adduseritem.ui \
    applyfriend.ui \
    applyfrienditem.ui \
    applyfriendpage.ui \
    authenfriend.ui \
    chatdialog.ui \
    chatpage.ui \
    chatuserwidget.ui \
    contactuseritem.ui \
    findfaildlg.ui \
    findsuccessdialog.ui \
    friendlabel.ui \
    grouptipitem.ui \
    loadingdialog.ui \
    login.ui \
    mainwindow.ui \
    register.ui \
    resetdialog.ui

RC_ICONS = ./img/favicon.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

QT += core gui network
