#pragma once
#include <QMainWindow>
#include <memory>

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
