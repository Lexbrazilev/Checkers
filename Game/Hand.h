#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
  public:
    Hand(Board *board) : board(board) // Конструктор, принимает указатель на объект доски.
    {
    }
    tuple<Response, POS_T, POS_T> get_cell() const // Получение координат выбранной клетки.
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        while (true) // Ждем событие пользователя.
        {
            if (SDL_PollEvent(&windowEvent)) // Получаем следующее событие.
            {
                switch (windowEvent.type) // Обрабатываем тип события.
                {
                case SDL_QUIT: // Закрытия окна.
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN: // Нажатие кнопки мыши
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1) // Нажатие на кнопку назад.
                    {
                        resp = Response::BACK;
                    }
                    else if (xc == -1 && yc == 8) // Нажатие на кнопку перезапуск.
                    {
                        resp = Response::REPLAY;
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8) // Нажатие на клетку доски.
                    {
                        resp = Response::CELL;
                    }
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: // Изменения размера окна.
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // Обновление размера доски.
                        break;
                    }
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return {resp, xc, yc};
    }

    Response wait() const // Ожидание действия игрока.
    {
        SDL_Event windowEvent;
        Response resp = Response::OK; // Изначальный статус ответа.
        while (true) // Основной цикл ожидания события.
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT: // Закрытия окна.
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED: // Изменения размера окна.
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {  // Нажатия кнопки мыши.
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8) // Если нажата перезапуска 
                        resp = Response::REPLAY;
                }
                break;
                }
                if (resp != Response::OK) // Если получен не OK ответ, выходим из цикла
                    break;
            }
        }
        return resp;
    }

  private:
    Board *board;
};
