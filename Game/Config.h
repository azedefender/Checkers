#pragma once
#include <fstream>                 // ��� ������ � ��������� �������� (������ settings.json)
#include <nlohmann/json.hpp>       // ���������� ��� ������ � JSON
using json = nlohmann::json;       // �������� ���������: ������ ����� ������ json ������ nlohmann::json

#include "../Models/Project_path.h" // ���������, ��� �������� ���� � ������ ������� (��������, project_path)

// ����� Config �������� �� �������� � �������� ������������ ���� �� ����� settings.json
class Config
{
public:
    // �����������: ��� �������� ������� Config ����� ��������� ��������� �� �����
    Config()
    {
        reload();
    }

    // ����� reload() ������������ ���� settings.json � ��������� ������ config
    void reload()
    {
        std::ifstream fin(project_path + "settings.json"); // ��������� ���� ��������
        fin >> config;                                     // ������ JSON � ������ config
        fin.close();                                       // ��������� ����
    }

    // ������������� �������� () ��������� ������ �������� ������ � ����������
    // ��������: config("WindowSize", "Width") ����� �������� ������ ����
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config; // ������, � ������� �������� ��� ��������� �������� �� settings.json
};
