#pragma once
#include <QApplication>

class Application : public QApplication {
public:
    using QApplication::QApplication;
    void init();
};
