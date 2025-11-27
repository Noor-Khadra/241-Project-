#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define rows 6
#define cols 7

void initialize(char board[rows][cols]) {
    for (int i=0;i<rows;i++) for (int j=0;j<cols;j++) board[i][j]='.';
}

void printBoard(char board[rows][cols]) {
    for (int i=0;i<rows;i++){
        for (int j=0;j<cols;j++) printf("%c ", board[i][j]);
        printf("\n");
    }
    for (int j=0;j<cols;j++) printf("%d ", j+1);
    printf("\n\n");
}

bool update(char board[rows][cols], int col, char player) {
    if (col<0 || col>=cols) return false;
    for (int i=rows-1;i>=0;i--) if (board[i][col]=='.') { board[i][col]=player; return true; }
    return false;
}

bool checkWin(char board[rows][cols], char player) {
    for (int r=0;r<rows;r++) for (int c=0;c<=cols-4;c++)
        if (board[r][c]==player && board[r][c+1]==player && board[r][c+2]==player && board[r][c+3]==player) return true;

    for (int c=0;c<cols;c++) for (int r=0;r<=rows-4;r++)
        if (board[r][c]==player && board[r+1][c]==player && board[r+2][c]==player && board[r+3][c]==player) return true;

    for (int r=3;r<rows;r++) for (int c=0;c<=cols-4;c++)
        if (board[r][c]==player && board[r-1][c+1]==player && board[r-2][c+2]==player && board[r-3][c+3]==player) return true;

    for (int r=0;r<=rows-4;r++) for (int c=0;c<=cols-4;c++)
        if (board[r][c]==player && board[r+1][c+1]==player && board[r+2][c+2]==player && board[r+3][c+3]==player) return true;

    return false;
}

bool boardFull(char board[rows][cols]) {
    for (int j=0;j<cols;j++) if (board[0][j]=='.') return false;
    return true;
}

int getColumn(int maxCols) {
    int col;
    while (1) {
        if (scanf("%d", &col) == 1) {
            if (col >= 1 && col <= maxCols) return col - 1;
            else printf("Invalid column. Enter a number (1-%d): ", maxCols);
        } else {
            while (getchar() != '\n');
            printf("Invalid input. Enter a number (1-%d): ", maxCols);
        }
        fflush(stdout);
    }
}

/* ---------- Networking helpers ---------- */

int recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    char *p = buf;
    while (total < len) {
        ssize_t r = recv(sock, p + total, len - total, 0);
        if (r <= 0) return -1;
        total += r;
    }
    return 0;
}

int send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = buf;
    while (total < len) {
        ssize_t s = send(sock, p + total, len - total, 0);
        if (s <= 0) return -1;
        total += s;
    }
    return 0;
}

void send_int(int sock, int x) {
    int32_t net = htonl(x);
    send_all(sock, &net, sizeof(net));
}

int recv_int(int sock, int *out) {
    int32_t net;
    if (recv_all(sock, &net, sizeof(net)) < 0) return -1;
    *out = ntohl(net);
    return 0;
}

int send_board_and_flags(int sock, char board[rows][cols], int status, int yourTurn) {
    if (send_all(sock, board, sizeof(char)*rows*cols) < 0) return -1;
    send_int(sock, status);
    send_int(sock, yourTurn);
    return 0;
}

/* ---------- Server socket ---------- */

int start_server(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); exit(1); }
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
    if (listen(s, 1) < 0) { perror("listen"); exit(1); }

    printf("Waiting for client on port %d...\n", port);
    int client = accept(s, NULL, NULL);
    if (client < 0) { perror("accept"); exit(1); }
    close(s);
    printf("Client connected.\n");
    return client;
}

/* ---------- Main ---------- */

int main(int argc, char **argv) {
    srand((unsigned int)time(NULL));
    int port = 9000;
    if (argc >= 2) port = atoi(argv[1]);

    char board[rows][cols];
    initialize(board);

    int clientSock = start_server(port);

    printf("Choose mode:\n1. Player vs Player (client = human)\n2. Player vs Bot (client = bot)\n> ");
    int mode;
    if (scanf("%d", &mode) != 1) { close(clientSock); return 0; }
    if (mode!=1 && mode!=2) mode = 1;
    send_int(clientSock, mode);

    char A = 'X', B = 'O';
    bool gameOver = false;

    while (!gameOver) {
        printBoard(board);

        printf("Server (Player %c) - your move: ", A);
        fflush(stdout);
        int col = getColumn(cols);
        if (!update(board, col, A)) {
            printf("Invalid move. Try again.\n");
            continue;
        }

        int status = 0; // 0 ongoing, 1 serverWins, 2 clientWins, 3 draw
        if (checkWin(board, A)) status = 1;
        else if (boardFull(board)) status = 3;

        if (send_board_and_flags(clientSock, board, status, (status==0)?1:0) < 0) {
            printf("Connection lost while sending board.\n");
            break;
        }

        if (status != 0) {
            if (status == 1) { printBoard(board); printf("Server wins!\n"); }
            else { printBoard(board); printf("Draw!\n"); }
            break;
        }

        printf("Waiting for client's move (Player %c)...\n", B);
        int clientCol;
        if (recv_int(clientSock, &clientCol) < 0) { printf("Connection lost while receiving client's move.\n"); break; }
        printf("Client played column %d\n", clientCol+1);

        if (!update(board, clientCol, B)) {
            printf("Client sent invalid move. Closing.\n");
            break;
        }

        if (checkWin(board, B)) {
            int status2 = 2;
            send_board_and_flags(clientSock, board, status2, 0);
            printBoard(board);
            printf("Client wins!\n");
            break;
        } else if (boardFull(board)) {
            int status2 = 3;
            send_board_and_flags(clientSock, board, status2, 0);
            printBoard(board);
            printf("Draw!\n");
            break;
        } else {
            if (send_board_and_flags(clientSock, board, 0, 0) < 0) { printf("Connection lost sending update.\n"); break; }
        }

    }

    close(clientSock);
    return 0;
}