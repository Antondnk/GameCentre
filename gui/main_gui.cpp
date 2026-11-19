#include <QApplication>
#include <QMessageBox>
#include <QStringList>

#include <string>

#include "Exceptions.h"
#include "GameCenter.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include "Theme.h"
#include "TimeUtils.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    app.setStyleSheet(app_stylesheet());

    const QStringList args = QCoreApplication::arguments();
    const std::string data_dir = args.size() > 1 ? args.at(1).toStdString() : "data";

    gc::GameCenter center;
    try {
        if (!center.load_all(data_dir)) {
            center.seed_demo_data(gc::timeutil::now_minutes());
            center.save_all(data_dir);
        }
    } catch (const gc::GameCenterException& error) {
        QMessageBox::critical(nullptr, "Ошибка загрузки данных", QString::fromUtf8(error.what()));
        return 1;
    }

    const auto commit = [&center, &data_dir] { center.save_all(data_dir); };

    // Цикл позволяет сменить пользователя без перезапуска программы.
    while (true) {
        LoginDialog login(center, commit);
        if (login.exec() != QDialog::Accepted) {
            break;
        }
        MainWindow window(center, login.user(), data_dir);
        window.show();
        app.exec();
        if (!window.logout_requested()) {
            break;
        }
    }
    return 0;
}
