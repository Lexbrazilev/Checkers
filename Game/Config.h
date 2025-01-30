#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config() // Обработчик настроек
    {
        reload();
    }

    void reload() // Функция чтения конфигурации игры из файла settings.json и сохранение в переменной config
    {
        std::ifstream fin(project_path + "settings.json"); // Открытие файла settings.json
        fin >> config; // Загружаем JSON данные из файла в переменную config
        fin.close(); // Закрытие файла settings.json
    }

    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config; // Сохранение
};
