#include "global.h"
#include <QCryptographicHash>

std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w); // 去掉原来样式
    w->style()->polish(w);
};

QString gate_url_prefix = "";

std::function<QString(QString)> xorString = [](QString input){
    QString result = input;
    int length = input.length();
    length = length % 255;
    for(int i = 0; i < length; ++i) {
        // 对每个字符进行xor
        // 假设所有字符都是ASCII
        result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ static_cast<ushort>(length)));
    }

    return result;
};

std::function<QString(QString)> toMD5 = [](QString input){
    QByteArray md5Hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5);
    QString res = md5Hash.toHex();
    return res;
};
