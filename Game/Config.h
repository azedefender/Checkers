#pragma once
#include <fstream>                 // для работы с файловыми потоками (чтение settings.json)
#include <nlohmann/json.hpp>       // библиотека для работы с JSON
using json = nlohmann::json;       // упрощаем обращение: теперь можно писать json вместо nlohmann::json

#include "../Models/Project_path.h" // заголовок, где хранится путь к файлам проекта (например, project_path)

// Класс Config отвечает за загрузку и хранение конфигурации игры из файла settings.json
class Config
{
public:
    // Конструктор: при создании объекта Config сразу загружает настройки из файла
    Config()
    {
        reload();
    }

    // Метод reload() перечитывает файл settings.json и обновляет объект config
    void reload()
    {
        std::ifstream fin(project_path + "settings.json"); // открываем файл настроек
        fin >> config;                                     // читаем JSON в объект config
        fin.close();                                       // закрываем файл
    }

    // Перегруженный оператор () позволяет удобно получать доступ к настройкам
    // Например: config("WindowSize", "Width") вернёт значение ширины окна
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config; // объект, в котором хранится вся структура настроек из settings.json
};
