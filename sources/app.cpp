#include "app.h"
#include "mainwindow.h"

void Application::init()
{
    setApplicationName(tr("Early reflections"));

    MainWindow* window = new MainWindow;
    window->setWindowTitle(applicationDisplayName());
    window->show();
}

///
int main(int argc, char* argv[])
{
    Application app(argc, argv);

    app.init();

    return app.exec();
}
