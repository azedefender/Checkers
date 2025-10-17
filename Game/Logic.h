#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9; // "бесконечность" для оценки позиций (используется в minimax)

class Logic
{
public:
    // Конструктор: принимает указатели на доску и конфиг
    // Инициализирует генератор случайных чисел (для случайного выбора ходов, если разрешено)
    // Загружает режим оценки и уровень оптимизации из настроек
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    // Основной метод: поиск лучшего хода для бота
    // Возвращает последовательность ходов (например, серия взятий)
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear();
        next_move.clear();

        // запускаем рекурсивный поиск лучшего хода
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        int cur_state = 0;
        vector<move_pos> res;
        // восстанавливаем цепочку ходов из next_move/next_best_state
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        return res;
    }

private:
    // Применение хода к копии доски (матрице)
    // Возвращает новую матрицу после хода
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) // если было взятие — удаляем побитую шашку
            mtx[turn.xb][turn.yb] = 0;

        // превращение в дамку (если шашка дошла до последней линии)
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;

        // перемещаем шашку
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }

    // Функция оценки позиции (чем выше — тем лучше для бота)
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        double w = 0, wq = 0, b = 0, bq = 0; // счётчики белых/чёрных шашек и дамок
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);

                // если включён режим "NumberAndPotential" — учитываем продвижение вперёд
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // белые ценнее ближе к дамкам
                    b += 0.05 * (mtx[i][j] == 2) * (i);     // чёрные ценнее ближе к дамкам
                }
            }
        }

        // если бот играет за чёрных — меняем местами оценки
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }

        // если у соперника нет шашек — победа
        if (w + wq == 0)
            return INF;
        // если у бота нет шашек — поражение
        if (b + bq == 0)
            return 0;

        int q_coef = 4; // коэффициент ценности дамки
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }

        // итоговая оценка: отношение силы бота к силе соперника
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    // Рекурсивный поиск первого лучшего хода (для серии взятий)
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        next_best_state.push_back(-1);
        next_move.emplace_back(-1, -1, -1, -1);
        double best_score = -1;

        if (state != 0)
            find_turns(x, y, mtx); // ищем ходы для конкретной шашки

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // если нет взятий — передаём ход сопернику
        if (!have_beats_now && state != 0)
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;
            if (have_beats_now) // если серия взятий — продолжаем её
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else // иначе — передаём ход сопернику
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            if (score > best_score)
            {
                best_score = score;
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
            }
        }
        return best_score;
    }

    // Рекурсивный minimax с альфа-бета отсечением
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        if (depth == Max_depth) // достигли глубины поиска
        {
            return calc_score(mtx, (depth % 2 == color));
        }

        // если продолжаем серию взятий
        if (x != -1)
        {
            find_turns(x, y, mtx);
        }
        else
            find_turns(color, mtx);

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        if (!have_beats_now && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty()) // если ходов нет — поражение
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1;
        double max_score = -1;

        for (auto turn : turns_now)
        {
            double score = 0.0;
            if (!have_beats_now && x == -1)
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // альфа-бета отсечение
            if (depth % 2)
                alpha = max(alpha, max_score);
            else
                beta = min(beta, min_score);

            if (optimization != "O0" && alpha >= beta)
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }
        return (depth % 2 ? max_score : min_score);
    }

public:
    // Обёртки для поиска ходов
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    // Поиск всех возможных ходов для заданного цвета
    // color = 0 (белые), 1 (чёрные)
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;   // список всех возможных ходов
        bool have_beats_before = false; // флаг: есть ли обязательные взятия

        // перебираем все клетки доски
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // если в клетке есть шашка нужного цвета
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    // ищем ходы для этой шашки
                    find_turns(i, j, mtx);

                    // если нашли взятия — сбрасываем предыдущие ходы
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }

                    // добавляем найденные ходы
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }

        // сохраняем результат
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng); // перемешиваем (если разрешено случайное поведение)
        have_beats = have_beats_before; // фиксируем, есть ли обязательные взятия
    }

    // Поиск ходов для конкретной шашки (x,y)
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y]; // тип фигуры: 1/2 — шашки, 3/4 — дамки

        // --- Проверка взятий ---
        switch (type)
        {
        case 1: // белая шашка
        case 2: // чёрная шашка
            // проверяем возможные прыжки через соседние фигуры
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2; // координаты побитой шашки
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb); // добавляем ход со взятием
                }
            }
            break;

        default: // дамка (3 или 4)
            // проверяем взятия во всех диагональных направлениях
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break; // нельзя бить более одной фигуры подряд
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }

        // если есть взятия — они обязательны
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }

        // --- Проверка обычных ходов ---
        switch (type)
        {
        case 1: // белая шашка
        case 2: // чёрная шашка
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1); // направление движения
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j); // добавляем обычный ход
            }
            break;
        }
        default: // дамка
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2); // добавляем ход дамки
                    }
                }
            }
            break;
        }
    }

  public:
      vector<move_pos> turns; // список возможных ходов
      bool have_beats;        // есть ли обязательные взятия
      int Max_depth;          // максимальная глубина поиска minimax

  private:
      default_random_engine rand_eng; // генератор случайных чисел
      string scoring_mode;            // режим оценки (например, "NumberAndPotential")
      string optimization;            // уровень оптимизации (O0, O1 и т.д.)
      vector<move_pos> next_move;     // вспомогательный массив для восстановления лучшего хода
      vector<int> next_best_state;    // связи между состояниями для цепочек ходов
      Board* board;                   // указатель на доску
      Config* config;                 // указатель на конфигурацию
};

