#pragma once
#include <stdlib.h>

typedef int8_t POS_T; // Тип данных для хранения координат.

struct move_pos
{
    POS_T x, y;             // Координаты начальной позиции.
    POS_T x2, y2;           // Координаты конечной позиции.
    POS_T xb = -1, yb = -1; // Координаты побитой фигуры, если есть.

    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2) // Конструктор для обычного хода без побития.
    {
    }
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb) // Конструктор для хода с побитием.
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    bool operator==(const move_pos &other) const // Сравнения двух объектов move_pos.
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    bool operator!=(const move_pos &other) const // Сравнения двух объектов move_pos с инветрацией результата.
    {
        return !(*this == other);
    }
};
