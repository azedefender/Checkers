#pragma once
#include <chrono>   // для измерения времени игры и ходов
#include <thread>   // для задержек (имитация времени раздумий бота)

#include "../Models/Project_path.h" // путь к ресурсам проекта
#include "Board.h"   // класс доски (отрисовка и хранение состояния)
#include "Config.h"  // класс конфигурации (чтение настроек из settings.json)
#include "Hand.h"    // класс для обработки ввода игрока (мышь/клавиатура)
#include "Logic.h"   // класс логики игры (генерация ходов, проверка правил)

class Game
{
public:
    // Конструктор: инициализирует доску, руку игрока и логику
    // Также очищает лог-файл (log.txt)
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")),
        hand(&board),
        logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Основной метод: запуск партии в шашки
    int play()
    {
        auto start = chrono::steady_clock::now(); // время начала партии

        // Если это повтор (REPLAY), то перезапускаем логику и перерисовываем доску
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            board.start_draw(); // первый запуск отрисовки доски
        }
        is_replay = false;

        int turn_num = -1;        // номер текущего хода
        bool is_quit = false;     // флаг выхода из игры
        const int Max_turns = config("Game", "MaxNumTurns"); // ограничение по количеству ходов

        // Основной цикл игры
        while (++turn_num < Max_turns)
        {
            beat_series = 0; // количество последовательных взятий
            logic.find_turns(turn_num % 2); // генерируем возможные ходы для текущего игрока
            if (logic.turns.empty()) // если ходов нет — игра окончена
                break;

            // Устанавливаем глубину поиска для бота (уровень сложности)
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // Если ходит человек
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                auto resp = player_turn(turn_num % 2); // обработка хода игрока
                if (resp == Response::QUIT) // выход из игры
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY) // перезапуск партии
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK) // откат хода
                {
                    // если бот играл предыдущим цветом и есть история ходов
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback(); // откат последнего хода
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                bot_turn(turn_num % 2); // если ходит бот
        }

        // Подсчёт времени партии
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: "
            << (int)chrono::duration<double, milli>(end - start).count()
            << " millisec\n";
        fout.close();

        // Обработка завершения партии
        if (is_replay)
            return play();
        if (is_quit)
            return 0;

        int res = 2; // результат: 0 — ничья, 1 — победа чёрных, 2 — победа белых
        if (turn_num == Max_turns)
        {
            res = 0; // ничья по лимиту ходов
        }
        else if (turn_num % 2)
        {
            res = 1; // победа чёрных
        }
        board.show_final(res); // показать финальный экран

        auto resp = hand.wait(); // ожидание действия игрока
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play(); // перезапуск
        }
        return res;
    }

private:
    // Ход бота
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();

        auto delay_ms = config("Bot", "BotDelayMS"); // задержка перед ходом
        thread th(SDL_Delay, delay_ms);              // имитация "раздумий" бота
        auto turns = logic.find_best_turns(color);   // поиск лучшего хода
        th.join();

        bool is_first = true;
        // выполнение хода (или серии взятий)
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1); // учёт серии взятий
            board.move_piece(turn, beat_series);
        }

        // логируем время хода бота
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: "
            << (int)chrono::duration<double, milli>(end - start).count()
            << " millisec\n";
        fout.close();
    }

    // Ход игрока
    Response player_turn(const bool color)
    {
        // Подсветка возможных фигур для хода
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);

        move_pos pos = { -1, -1, -1, -1 }; // выбранный ход
        POS_T x = -1, y = -1;            // координаты выбранной фигуры

        // Цикл выбора хода игроком
        while (true)
        {
            auto resp = hand.get_cell(); // ожидание клика игрока
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp); // если игрок нажал "выход" или "повтор"

            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };
            bool is_correct = false;

            // проверка, выбрана ли корректная фигура или ход
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }

            if (pos.x != -1) // если выбран ход
                break;

            if (!is_correct) // если выбор некорректный — сброс
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }

            // если выбрана корректная фигура — подсветить её возможные ходы
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);

            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }

        // выполнение хода
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);

        if (pos.xb == -1) // если не было взятия
            return Response::OK;

        // если было взятие — проверяем возможность продолжения серии
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // цикл выбора следующей клетки для продолжения взятия
            while (true)
            {
                auto resp = hand.get_cell(); // ждём клик игрока
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp); // если игрок нажал "выход" или "повтор"

                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };
                bool is_correct = false;

                // проверяем, допустим ли выбранный ход
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn; // сохраняем выбранный ход
                        break;
                    }
                }
                if (!is_correct)
                    continue; // если ход некорректный — ждём новый ввод

                // если ход корректный — выполняем его
                board.clear_highlight();
                board.clear_active();
                beat_series += 1; // увеличиваем счётчик серии взятий
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK; // ход игрока завершён успешно
    }

  private:
      Config config;   // объект конфигурации (читает settings.json)
      Board board;     // игровая доска (отрисовка и хранение состояния)
      Hand hand;       // обработка ввода игрока (мышь/клавиатура)
      Logic logic;     // логика игры (генерация ходов, проверка правил)
      int beat_series; // количество последовательных взятий в текущем ходе
      bool is_replay = false; // флаг перезапуска партии
};

