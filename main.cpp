#include <iostream>
#include <string>

#include "ConsoleApp.h"
#include "Exceptions.h"
#include "GameCenter.h"
#include "TimeUtils.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    const std::string data_dir = argc > 1 ? argv[1] : "data";

    gc::GameCenter center;
    try {
        if (!center.load_all(data_dir)) {
            std::cout << "Данные не найдены, создаются демо-данные в \"" << data_dir << "\".\n";
            center.seed_demo_data(gc::timeutil::now_minutes());
            center.save_all(data_dir);
        }
    } catch (const gc::GameCenterException& error) {
        std::cerr << "Ошибка загрузки данных: " << error.what() << "\n";
        return 1;
    }

    gc::ConsoleApp app(center, data_dir, std::cin, std::cout);
    app.run();
    return 0;
}
