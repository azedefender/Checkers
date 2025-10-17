#pragma once
#include <stdlib.h>

// POS_T — тип для хранения координат клетки на доске (используется int8_t, чтобы экономить память)
typedef int8_t POS_T;

// Структура move_pos описывает один возможный ход шашки
struct move_pos
{
    POS_T x, y;             // начальная клетка (координаты фигуры "откуда")
    POS_T x2, y2;           // конечная клетка (координаты фигуры "куда")
    POS_T xb = -1, yb = -1; // координаты побитой шашки (если есть взятие; -1 = нет взятия)

    // Конструктор для обычного хода (без взятия)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2)
        : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Конструктор для хода со взятием (с указанием побитой шашки)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Оператор сравнения "равно": два хода равны, если совпадают начальная и конечная клетки
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // Оператор сравнения "не равно": противоположность ==
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};
