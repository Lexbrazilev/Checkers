#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Начало игры шашки.
    int play()
    {
        auto start = chrono::steady_clock::now(); // Записываем начальное время для измерения продолжительности игры.
        if (is_replay) // Если это повтор игры то перезагружаем логику и конфигурацию и перерисовыаем доску.
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else // Если это начало новой игры, отрисовываем доску.
        {
            board.start_draw();
        }
        is_replay = false; // Сбрасываем флаг повтора.

        int turn_num = -1; // Инициализируем счетчик ходов.
        bool is_quit = false; // Инициализируем флаг выхода из игры.
        const int Max_turns = config("Game", "MaxNumTurns"); // Получаем максимальное количество ходов.
        while (++turn_num < Max_turns) // Начинаем цикл игровых ходов.
        {
            beat_series = 0; // Обнуляем счетчик серии ходов.
            logic.find_turns(turn_num % 2); // Находим возможные ходы для текущего игрока.
            if (logic.turns.empty()) // Если нет возможных ходов, прерываем игру.
                break;
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel")); // Устанавливаем глубину поиска для бота.
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot"))) // Проверяем что текущий игрок бот.
            {
                auto resp = player_turn(turn_num % 2); // Даем ход игроку и обработка возможных вариантов.
                if (resp == Response::QUIT) // Если игрок нажал "Выход" и завершил игру.
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY) // Если игрок нажал "Переиграть".
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK) // Если игрок нажал "Назад" то откатываеться ход и уменьшаеться счетчик ходов и взятия фигур.
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else // Если сейчас играет бот то даем ему ход.
                bot_turn(turn_num % 2);
        }
        auto end = chrono::steady_clock::now(); // Записываем конечное время для измерения продолжительности игры.
        ofstream fout(project_path + "log.txt", ios_base::app); // Открываем файл записываем время игры в лог и закрываем его.
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        if (is_replay) // Повтор игры.
            return play();
        if (is_quit) // Конец игры.
            return 0;
        int res = 2; // Результат игры.
        if (turn_num == Max_turns)
        {
            res = 0; // Ничья по ходам.
        }
        else if (turn_num % 2)
        {
            res = 1; // Победа черных.
        }
        board.show_final(res); // Отображение итогового результата.
        auto resp = hand.wait(); // Ожидаем действия игрока например перезапуск игры.
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res; // Возвращаем результат игры.
    }

  private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now(); // Записываем время начала хода бота для измерения времени выполнения.

        auto delay_ms = config("Bot", "BotDelayMS"); // Получаем задержку для хода бота из конфигурации.
        // Создаем новый поток, который будет выполнять задержку.
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color); // Логика поиска лучших ходов для бота 
        th.join(); // Ожидаем завершения потока задержки.
        bool is_first = true;
        // making moves
        for (auto turn : turns) // Перебираем все полученные ходы.
        {
            if (!is_first) // Если это не первый ход, то делаем задержку.
            {
                SDL_Delay(delay_ms); 
            }
            is_first = false;
            beat_series += (turn.xb != -1);
            board.move_piece(turn, beat_series);
        }

        auto end = chrono::steady_clock::now(); // Записываем время окончания хода бота.
        ofstream fout(project_path + "log.txt", ios_base::app); // Открываем файл лога.
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n"; // Записываем в лог время выполнения хода бота.
        fout.close(); // Закрываем файл лога.
    }

    Response player_turn(const bool color) // Функция обработки хода игрока.
    {
        // Возвращаем 1 если выход.
        vector<pair<POS_T, POS_T>> cells; // Вектор пар координат для хранения возможных ходов.
        for (auto turn : logic.turns) // Заполняем список координатами из возможных ходов.
        {
            cells.emplace_back(turn.x, turn.y); // Стартовые координаты хода.
        }
        board.highlight_cells(cells); // Подсвечиваем клетки с возможными ходами на доске.
        move_pos pos = {-1, -1, -1, -1}; // Переменная для хранения координат хода.
        POS_T x = -1, y = -1; // Координаты для первого клика (выбора шашки).
        // Цикл ожидания первого хода игрока.
        while (true)
        {
            auto resp = hand.get_cell(); // Ждем клик(нажатия на клетку) от игрока.
            if (get<0>(resp) != Response::CELL) // Если выбор игрока не клетка, возвращаем ответ.
                return get<0>(resp);
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; // Получаем координаты выбранной клетки.

            bool is_correct = false;
            for (auto turn : logic.turns) // Проверяем является ли клетка доступная для хода.
            {
                if (turn.x == cell.first && turn.y == cell.second) // Если координаты клетки совпадают с координатами начала возможного хода.
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second}) // Если координаты клетки совпадают с координатами конца возможного хода.
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1) // Если найден правильный ход, выходим.
                break;
            if (!is_correct) // Если не правильный, то сбрасываем.
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1; // Сбрасываем координаты первой клетки.
                y = -1;
                continue;
            }
            x = cell.first; // Сохраняем координаты выбранной шашки.
            y = cell.second;
            board.clear_highlight(); // Очищаем выделения.
            board.set_active(x, y); // Выделяем текущую клетку как активную.
            vector<pair<POS_T, POS_T>> cells2; // Собираем клетки для хода.
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2); // Подсвечиваем возможные ходы.
        }
        board.clear_highlight(); // Очищаем подсветку.
        board.clear_active(); // Убираем выделение с активной шашки.
        board.move_piece(pos, pos.xb != -1); // Перемещаем шашку.
        if (pos.xb == -1) // Если нет съедаемых фигур, то возвращаем ОК.
            return Response::OK;
        // Продолжаем серию побитий.
        beat_series = 1; // Инициализируем счётчик серии битья.
        while (true) // Цикл ожидания продолжения взятий.
        {
            logic.find_turns(pos.x2, pos.y2); // Находим съедаемые фигуры.
            if (!logic.have_beats)
                break;

            vector<pair<POS_T, POS_T>> cells; // Собираем список клеток для дальнейшего хода.
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells); // Подсвечиваем возможные клетки для взятия.
            board.set_active(pos.x2, pos.y2); // Подсвечиваем активную шашку.
            // trying to make move
            while (true)
            {
                auto resp = hand.get_cell(); // Ожидаем выбор игрока
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; // Получаем координаты выбранной клетки.

                bool is_correct = false;
                for (auto turn : logic.turns) // Проверяем выбранную клетку на корректность.
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct) // Если не корректная клетка, повторяем процесс.
                    continue;

                board.clear_highlight(); // Очистка подсветок
                board.clear_active();
                beat_series += 1; // Увеличиваем серию побитй
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK;
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
