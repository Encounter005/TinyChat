#include "global.h"

std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w); // 去掉原来样式
    w->style()->polish(w);
};

QString gate_url_prefix = "";
