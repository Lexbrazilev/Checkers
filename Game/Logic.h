#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (  // Генератор случайных чисел.
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color)
    {
        next_move.clear(); // Очищаем вектора.
        next_best_state.clear(); // Очищаем вектора.

        find_first_best_turn(board->get_board(), color, -1, -1, 0); // Вызываем функцию и находим лучший ход.

        vector<move_pos> res; // Вектор ходов добавляем результат.
        int state = 0; // Начальное состояние равное 0.
        do {
            res.push_back(next_move[state]); // Добавляем ходы в результат.
            state = next_best_state[state]; // Переход в следующее состояние.
        } while (state != -1 && next_move[state].x != -1); // Проверяем состояние побитий.
        return res; // Возвращаем результат.
    }

private:
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const // Функция для выполнения хода на доске.
    {
        if (turn.xb != -1) // Проверяем, был ли ход со взятием шашки.
            mtx[turn.xb][turn.yb] = 0; // Если был, то убираем взятую шашку с доски
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7)) // Проверяем, не превратилась ли пешка в дамку.
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещаем шашку на новую позицию.
        mtx[turn.x][turn.y] = 0; // Очищаем старую позицию, откуда шашка ушла.
        return mtx; // Возвращаем новое состояние доски.
    }

    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const // Функция для вычисления оценки состояния доски.
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0; // Инициализация переменных для подсчета шашек и дамок белого и черного игрока.
        for (POS_T i = 0; i < 8; ++i) // Перебираем все клетки доски.
        {
            for (POS_T j = 0; j < 8; ++j) // Считаем количество шашек и дамок каждого цвета.
            {
                w += (mtx[i][j] == 1); // Белая пешка.
                wq += (mtx[i][j] == 3); // Белая дамка.
                b += (mtx[i][j] == 2); // Черная пешка.
                bq += (mtx[i][j] == 4); // Черная дамка.
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // Стоимость белой пешки
                    b += 0.05 * (mtx[i][j] == 2) * (i); // Стоимость черной пешки
                }
            }
        }
        if (!first_bot_color) // Если первый бот играет за черных, меняем местами значения черных и белых шашек.
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0) // Если у белого игрока не осталось фигур возвращаем бессконечность черный выиграл.
            return INF;
        if (b + bq == 0) // Если у черного игрока не осталось фигур возвращаем ноль белый выйграл.
            return 0;
        int q_coef = 4; // Коэффициент, определяющий ценность дамки.
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // Возвращаем отношение количества фигур черного игрока к количеству фигур белого игрока, учитывая вес дамок.
    }

    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
                                double alpha = -1)
    {
        next_move.emplace_back(-1, -1, -1, -1); // Заполняем вектора.
        next_best_state.push_back(-1); // Заполняем вектора.

        if (state != 0) { // Если state не равно 0, загружаем все возможные ходы.
            find_turns(x, y, mtx);
        }
        auto now_turns = turns; // Создаем копию turns что бы сохранить значения.
        auto now_have_beats = have_beats; // Создаем копию have_beats что бы сохранить значения.

        if (!now_have_beats && state != 0) // Если не кого бить и нет уже ходов то ищем заново.
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha); // Запуск рекурсии.
        }
        double best_score = -1; // Лучший счет.
        for (auto turn : now_turns) { // Перебираем все ходы.
            size_t new_state = next_move.size();
            double score;
            if (now_have_beats) // Если есть кого бить, то продолжаем серию.
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);
            }
            else // Если не кого не бьем то переходим в рекурсию.
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            if (score > best_score) // Проверяем что новый результат лучше то обновляем информацию
            {
                best_score = score;
                next_move[state] = turn;
                next_best_state[state] = (now_have_beats ? new_state : -1);
            }
        }
        return best_score; // Возращаем результат.
    }

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
                               double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        if (depth == Max_depth) // Проверка глубины, условия для выхода из рекурсии.
        {
            return calc_score(mtx, (depth % 2 == color)); // Возврат наилучшего результата.
        }
        if (x != -1) // Проверяем есть ли серия побитий.
        {
            find_turns(x, y, mtx);
        }
        else
        {
            find_turns(color, mtx); // Ищем все возможные ходы.
        }

        auto now_turns = turns; // 
        auto now_have_beats = have_beats; // 

        if (!now_have_beats && x != -1) // Если нечего бить проверяем еще.
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty()) { // Проверяем есть ли ходы и кто выйграл.
            return (depth % 2 ? 0 : INF);
        }

        double min_score = INF + 1; // Переменные для minimax алгоритма
        double max_score = -1; // Переменные для minimax алгоритма

        for (auto turn : now_turns)
        {
            double score;

            if (now_have_beats) // Если есть побития то продолжаем серию.
            {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }

            min_score = min(min_score, score); // Обновляем min.
            max_score = max(max_score, score); // Обновляем max.
            // alpha-beta отсечение.
            if (depth % 2) { // Если ход наш
                alpha = max(alpha, max_score); // Двигаем левую границу отсечения.
            }
            else { // Если ход соперника
                beta = min(beta, min_score); // Двигаем правую границу отсечения.
            }
            if (optimization != "O0" && alpha > beta) { // Проверяем оптимизацию отсечения.
                break;
            }
            if (optimization != "O2" && alpha == beta) { // Проверяем оптимизацию отсечения.
                return (depth % 2 ? max_score + 1 : min_score - 1);
            }
        }

        return (depth % 2 ? max_score : min_score); // Возвращаем результат в зависимости от глубины.
    }

public:
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx) // Функция поиска возможных ходов.
    {
        vector<move_pos> res_turns; // Вектор для хранения возможных ходов.
        bool have_beats_before = false; // Флаг, указывающий, были ли ходы с взятием шашки.
        for (POS_T i = 0; i < 8; ++i) // Проходим по всей доске.
        {
            for (POS_T j = 0; j < 8; ++j) 
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color) // Проверяем, что текущая клетка занята шашкой противоположного цвета.
                {
                    find_turns(i, j, mtx); // Вызываем функцию для поиска ходов для конкретной шашки.
                    if (have_beats && !have_beats_before) // Если были ходы с взятием шашки и не было таких ходов на предыдущих шашках.
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns; // Обновляем глобальный список ходов
        shuffle(turns.begin(), turns.end(), rand_eng); // Перемешиваем ходы для рандомизации выбора хода
        have_beats = have_beats_before; // Обновляем флаг наличия взятий
    }

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx) // Функция для поиска возможных ходов для шашки на заданной позиции.
    {
        turns.clear(); // Очищаем вектор ходов, чтобы не накапливать ходы с предыдущих вызовов функции.
        have_beats = false;
        POS_T type = mtx[x][y]; // Получаем тип шашки, находящейся на позиции.
        // Проверяем возможность взятия шашек противника.
        switch (type)
        {
        case 1:
        case 2:
            // Проверяем все возможные направления для взятия шашек для обычных шашек.
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // Проверяем все возможные направления для взятия шашек для дамки.
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
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb); // если ход доступен то помещаем в вектор
                        }
                    }
                }
            }
            break;
        }
        // Если найдены ходы со взятием, то устанавливаем флаг и выходим.
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type) // Проверяем возможность обычных ходов (без взятия).
        {
        case 1:
        case 2:
            // Вычисляем направление движения для белых и черных шашек.
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j]) // Проверяем, что клетка назначения находится на доске и не занята.
                        continue;
                    turns.emplace_back(x, y, i, j); // Добавляем ход в список ходов.
                }
                break;
            }
        default:
            // Проверяем все возможные направления для хода дамки.
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j) // Двигаемся по направлению пока не упремся в край доски
                    {
                        if (mtx[i2][j2]) // Если клетка занята, заканчиваем поиск.
                            break;
                        turns.emplace_back(x, y, i2, j2); // Добавляем ход в список.
                    }
                }
            }
            break;
        }
    }

  public:
    vector<move_pos> turns; //  Ходы найденные функцией find_turns.
    bool have_beats; // Флаг являються ли ходы побитиями.
    int Max_depth; // Максимальная глубина просчета

  private:
    default_random_engine rand_eng;
    string scoring_mode; // Отвечал за потенциальность становлекния шашки дамкой
    string optimization; // Тип оптимизации (О0, О1, О2)
    vector<move_pos> next_move; // Вектор ходов.
    vector<int> next_best_state; // Вектор интов.
    Board *board; // Указатель на доску.
    Config *config; // Указатель на конфиг.
};
