#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс Hand отвечает за обработку ввода игрока (мышь, закрытие окна).
// Он связывает события SDL (клики, выход, изменение размера окна) с логикой игры.
class Hand
{
public:
    // Конструктор: принимает указатель на объект Board,
    // чтобы иметь доступ к размерам окна и истории ходов.
    Hand(Board* board) : board(board)
    {
    }

    // Метод get_cell() ожидает действия игрока и возвращает:
    // - тип ответа (Response)
    // - координаты клетки (xc, yc), если клик был по доске
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;        // событие SDL (мышь, окно, выход)
        Response resp = Response::OK; // по умолчанию — всё нормально
        int x = -1, y = -1;           // координаты клика в пикселях
        int xc = -1, yc = -1;         // координаты клетки на доске

        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // проверяем очередь событий
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT: // если игрок закрыл окно
                    resp = Response::QUIT;
                    break;

                case SDL_MOUSEBUTTONDOWN: // если нажата кнопка мыши
                    x = windowEvent.motion.x; // пиксельная координата X
                    y = windowEvent.motion.y; // пиксельная координата Y

                    // переводим пиксельные координаты в координаты клетки доски
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);

                    // если клик по области "назад" (слева внизу)
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;
                    }
                    // если клик по области "повтор" (слева вверху)
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;
                    }
                    // если клик по клетке доски (0..7)
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;
                    }
                    // иначе — некорректный клик
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;

                case SDL_WINDOWEVENT: // событие окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // пересчёт размеров доски
                        break;
                    }
                }

                // если получен ответ (QUIT, BACK, REPLAY, CELL) — выходим из цикла
                if (resp != Response::OK)
                    break;
            }
        }
        return { resp, xc, yc }; // возвращаем результат
    }

    // Метод wait() — ожидание действия игрока в конце партии.
    // Возвращает Response::QUIT или Response::REPLAY.
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;

        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT: // закрытие окна
                    resp = Response::QUIT;
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED: // изменение размера окна
                    board->reset_window_size();
                    break;

                case SDL_MOUSEBUTTONDOWN: { // клик мышью
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);

                    // если клик по кнопке "повтор"
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                                        break;
                }

                if (resp != Response::OK)
                    break;
            }
        }
        return resp;
    }

private:
    Board* board; // ссылка на доску, чтобы знать размеры и историю ходов
};
