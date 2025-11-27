#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#define rows 6
#define cols 7

void initialize(char board[rows][cols]) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            board[i][j] = '.';
}

void print(char board[rows][cols]) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++)
            printf("%c ", board[i][j]);
        printf("\n");
    }
    for (int j = 0; j < cols; j++)
        printf("%d ", j + 1);
    printf("\n\n");
}

bool update(char board[rows][cols], int col, char player) {
    if (col < 0 || col >= cols)
        return false;
    for (int i = rows - 1; i >= 0; i--) {
        if (board[i][col] == '.') {
            board[i][col] = player;
            return true;
        }
    }
    return false;
}

bool checkWin(char board[rows][cols], char player) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j <= cols - 4; j++) {
            if (board[i][j] == player &&
                board[i][j + 1] == player &&
                board[i][j + 2] == player &&
                board[i][j + 3] == player)
                return true;
        }
    }

    for (int j = 0; j < cols; j++) {
        for (int i = 0; i <= rows - 4; i++) {
            if (board[i][j] == player &&
                board[i + 1][j] == player &&
                board[i + 2][j] == player &&
                board[i + 3][j] == player)
                return true;
        }
    }

    for (int i = 3; i < rows; i++) {
        for (int j = 0; j <= cols - 4; j++) {
            if (board[i][j] == player &&
                board[i - 1][j + 1] == player &&
                board[i - 2][j + 2] == player &&
                board[i - 3][j + 3] == player)
                return true;
        }
    }

    for (int i = 0; i <= rows - 4; i++) {
        for (int j = 0; j <= cols - 4; j++) {
            if (board[i][j] == player &&
                board[i + 1][j + 1] == player &&
                board[i + 2][j + 2] == player &&
                board[i + 3][j + 3] == player)
                return true;
        }
    }

    return false;
}

bool boardFull(char board[rows][cols]) {
    for (int j = 0; j < cols; j++) {
        if (board[0][j] == '.')
            return false;
    }
    return true;
}

int getColumn(int maxCols) {
    int col;
    while (1) {
        if (scanf("%d", &col) == 1) {
            if (col >= 1 && col <= maxCols)
                return col - 1;
            else
                printf("Invalid column. Enter a number (1-%d): ", maxCols);
        } else {
            while (getchar() != '\n');
            printf("Invalid input. Enter a number (1-%d): ", maxCols);
        }
        fflush(stdout);
    }
}

void copyBoard(char dest[rows][cols], char src[rows][cols]) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            dest[i][j] = src[i][j];
}

void getValidLocations(char board[rows][cols], int valid[], int *validCount) {
    *validCount = 0;
    for (int j = 0; j < cols; j++) {
        if (board[0][j] == '.') {
            valid[(*validCount)++] = j;
        }
    }
}

bool isTerminalNode(char board[rows][cols], char bot, char player) {
    return checkWin(board, bot) || checkWin(board, player) || boardFull(board);
}

int evaluateWindow(char window[4], char bot, char player) {
    int score = 0;
    int botCount = 0, playerCount = 0, emptyCount = 0;
    for (int i = 0; i < 4; i++) {
        if (window[i] == bot) botCount++;
        else if (window[i] == player) playerCount++;
        else emptyCount++;
    }

    if (botCount == 4) score += 10000;
    else if (botCount == 3 && emptyCount == 1) score += 100;
    else if (botCount == 2 && emptyCount == 2) score += 10;

    if (playerCount == 3 && emptyCount == 1) score -= 900;
    else if (playerCount == 2 && emptyCount == 2) score -= 20;

    return score;
}

int scorePosition(char board[rows][cols], char bot, char player) {
    int score = 0;

    int centerCol = cols / 2;
    int centerCount = 0;
    for (int r = 0; r < rows; r++)
        if (board[r][centerCol] == bot) centerCount++;
    score += centerCount * 6;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c <= cols - 4; c++) {
            char window[4];
            for (int k = 0; k < 4; k++) window[k] = board[r][c + k];
            score += evaluateWindow(window, bot, player);
        }
    }

    for (int c = 0; c < cols; c++) {
        for (int r = 0; r <= rows - 4; r++) {
            char window[4];
            for (int k = 0; k < 4; k++) window[k] = board[r + k][c];
            score += evaluateWindow(window, bot, player);
        }
    }

    for (int r = 3; r < rows; r++) {
        for (int c = 0; c <= cols - 4; c++) {
            char window[4];
            for (int k = 0; k < 4; k++) window[k] = board[r - k][c + k];
            score += evaluateWindow(window, bot, player);
        }
    }

    for (int r = 0; r <= rows - 4; r++) {
        for (int c = 0; c <= cols - 4; c++) {
            char window[4];
            for (int k = 0; k < 4; k++) window[k] = board[r + k][c + k];
            score += evaluateWindow(window, bot, player);
        }
    }

    return score;
}

int minimax(char board[rows][cols], int depth, int alpha, int beta, bool maximizingPlayer, char bot, char player, int *bestCol) {
    int valid[cols];
    int validCount;
    getValidLocations(board, valid, &validCount);

    bool isTerminal = isTerminalNode(board, bot, player);
    if (depth == 0 || isTerminal) {
        if (isTerminal) {
            if (checkWin(board, bot)) return 100000000;   // very large positive
            else if (checkWin(board, player)) return -100000000; // very large negative
            else return 0; // draw
        } else {
            return scorePosition(board, bot, player);
        }
    }

    if (maximizingPlayer) {
        int value = INT_MIN;
        int column = valid[0];
        for (int i = 0; i < validCount; i++) {
            int col = valid[i];
            char temp[rows][cols];
            copyBoard(temp, board);
            if (!update(temp, col, bot)) continue;
            int newScore = minimax(temp, depth - 1, alpha, beta, false, bot, player, NULL);
            if (newScore > value) { value = newScore; column = col; }
            if (value > alpha) alpha = value;
            if (alpha >= beta) break;
        }
        if (bestCol) *bestCol = column;
        return value;
    } else {
        int value = INT_MAX;
        int column = valid[0];
        for (int i = 0; i < validCount; i++) {
            int col = valid[i];
            char temp[rows][cols];
            copyBoard(temp, board);
            if (!update(temp, col, player)) continue;
            int newScore = minimax(temp, depth - 1, alpha, beta, true, bot, player, NULL);
            if (newScore < value) { value = newScore; column = col; }
            if (value < beta) beta = value;
            if (alpha >= beta) break;
        }
        if (bestCol) *bestCol = column;
        return value;
    }
}

int botMove(char board[rows][cols], char bot, char player, int difficulty) {
    int col;

    if (difficulty == 1) {
        do {
            col = rand() % cols;
        } while (board[0][col] != '.');
        printf("Bot chooses column %d (Easy)\n", col + 1);
        return col;
    }

    for (int j = 0; j < cols; j++) {
        char temp[rows][cols];
        copyBoard(temp, board);
        if (update(temp, j, bot) && checkWin(temp, bot)) {
            printf("Bot chooses column %d (Winning Move)\n", j + 1);
            return j;
        }
    }

    for (int j = 0; j < cols; j++) {
        char temp[rows][cols];
        copyBoard(temp, board);
        if (update(temp, j, player) && checkWin(temp, player)) {
            printf("Bot chooses column %d (Blocking Move)\n", j + 1);
            return j;
        }
    }

    if (difficulty == 2) {
        if (board[0][cols / 2] == '.') {
            printf("Bot chooses column %d (Center Preference)\n", cols / 2 + 1);
            return cols / 2;
        }

        int offsets[] = {0, 1, -1, 2, -2, 3, -3};
        for (int k = 0; k < 7; k++) {
            int c = cols / 2 + offsets[k];
            if (c >= 0 && c < cols && board[0][c] == '.') {
                printf("Bot chooses column %d (Strategic Fallback)\n", c + 1);
                return c;
            }
        }

        do {
            col = rand() % cols;
        } while (board[0][col] != '.');
        printf("Bot chooses column %d (Random Fallback)\n", col + 1);
        return col;
    }

    if (difficulty == 3) {
        for (int j = 0; j < cols; j++) {
            char temp[rows][cols];
            copyBoard(temp, board);
            if (update(temp, j, bot) && checkWin(temp, bot)) {
                printf("Bot chooses column %d (Winning Move)\n", j + 1);
                return j;
            }
        }
        for (int j = 0; j < cols; j++) {
            char temp[rows][cols];
            copyBoard(temp, board);
            if (update(temp, j, player) && checkWin(temp, player)) {
                printf("Bot chooses column %d (Blocking Move)\n", j + 1);
                return j;
            }
        }

        int depth = 6;

        int bestCol = -1;
        int score = minimax(board, depth, INT_MIN + 1, INT_MAX - 1, true, bot, player, &bestCol);

        if (bestCol < 0 || board[0][bestCol] != '.') {
            if (board[0][cols / 2] == '.') {
                bestCol = cols / 2;
            } else {
                int offsets[] = {0, 1, -1, 2, -2, 3, -3};
                for (int k = 0; k < 7; k++) {
                    int c = cols / 2 + offsets[k];
                    if (c >= 0 && c < cols && board[0][c] == '.') {
                        bestCol = c;
                        break;
                    }
                }
                if (bestCol < 0) {
                    do { bestCol = rand() % cols; } while (board[0][bestCol] != '.');
                }
            }
        }

        printf("Bot chooses column %d (Hard, score %d)\n", bestCol + 1, score);
        return bestCol;
    }

    do {
        col = rand() % cols;
    } while (board[0][col] != '.');
    printf("Bot chooses column %d (Fallback)\n", col + 1);
    return col;
}

int main() {
    srand((unsigned int)time(NULL));
    char A, B;
    int mode, difficulty = 0;

    printf("Welcome to Connect Four!\n");
    printf("Choose mode:\n1. Player vs Player\n2. Player vs Bot\n> ");
    if (scanf("%d", &mode) != 1) return 0;

    printf("Player A symbol: ");
    scanf(" %c", &A);

    if (mode == 1) {
        printf("Player B symbol: ");
        scanf(" %c", &B);
    } else {
        B = 'O';
        printf("Bot symbol is '%c'\n", B);
        printf("Choose difficulty:\n1. Easy\n2. Medium\n3. Hard\n> ");
        scanf("%d", &difficulty);
        printf("You selected ");
        if (difficulty == 1) printf("Easy\n");
        else if (difficulty == 2) printf("Medium\n");
        else printf("Hard\n");
    }

    char player = A;
    char board[rows][cols];
    initialize(board);

    while (true) {
        print(board);
        int col;

        if (mode == 2 && player == B) {
            col = botMove(board, B, A, difficulty);
        } else {
            printf("Player %c, enter column (1-%d): ", player, cols);
            fflush(stdout);
            col = getColumn(cols);
        }

        if (!update(board, col, player)) {
            printf("Invalid move. Try again.\n");
            continue;
        }

        if (checkWin(board, player)) {
            print(board);
            if (mode == 2 && player == B)
                printf("Bot wins!\n");
            else
                printf("Player %c wins!\n", player);
            break;
        }

        if (boardFull(board)) {
            print(board);
            printf("It's a draw!\n");
            break;
        }

        player = (player == A) ? B : A;
    }

    return 0;
}