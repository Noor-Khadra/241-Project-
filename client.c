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

void initialize(char board[rows][cols]) { for (int i=0;i<rows;i++) for (int j=0;j<cols;j++) board[i][j]='.'; }

void printBoardLocal(char board[rows][cols]) {
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

bool boardFull(char board[rows][cols]) { for (int j=0;j<cols;j++) if (board[0][j]=='.') return false; return true; }

int getColumnLocal(int maxCols) {
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
void send_int(int sock, int x) { int32_t net = htonl(x); send_all(sock, &net, sizeof(net)); }
int recv_int(int sock, int *out) { int32_t net; if (recv_all(sock, &net, sizeof(net))<0) return -1; *out = ntohl(net); return 0; }

/* ---------- Bot implementation (minimax + heuristics) ---------- */
void copyBoard(char dst[rows][cols], char src[rows][cols]) { for (int i=0;i<rows;i++) for (int j=0;j<cols;j++) dst[i][j]=src[i][j]; }
void getValidLocations(char board[rows][cols], int valid[], int *count) { *count=0; for (int j=0;j<cols;j++) if (board[0][j]=='.') valid[(*count)++]=j; }
bool isTerminalNode(char board[rows][cols], char bot, char player) { return checkWin(board, bot) || checkWin(board, player) || boardFull(board); }

int evaluateWindow(char window[4], char bot, char player) {
    int score=0, botCount=0, playerCount=0, emptyCount=0;
    for (int i=0;i<4;i++){ if (window[i]==bot) botCount++; else if (window[i]==player) playerCount++; else emptyCount++; }
    if (botCount==4) score += 10000;
    else if (botCount==3 && emptyCount==1) score += 100;
    else if (botCount==2 && emptyCount==2) score += 10;
    if (playerCount==3 && emptyCount==1) score -= 900;
    else if (playerCount==2 && emptyCount==2) score -= 20;
    return score;
}

int scorePosition(char board[rows][cols], char bot, char player) {
    int score=0;
    int centerCol = cols/2;
    int centerCount=0;
    for (int r=0;r<rows;r++) if (board[r][centerCol]==bot) centerCount++;
    score += centerCount * 6;
    for (int r=0;r<rows;r++) for (int c=0;c<=cols-4;c++){ char w[4]; for (int k=0;k<4;k++) w[k]=board[r][c+k]; score += evaluateWindow(w, bot, player); }
    for (int c=0;c<cols;c++) for (int r=0;r<=rows-4;r++){ char w[4]; for (int k=0;k<4;k++) w[k]=board[r+k][c]; score += evaluateWindow(w, bot, player); }
    for (int r=3;r<rows;r++) for (int c=0;c<=cols-4;c++){ char w[4]; for (int k=0;k<4;k++) w[k]=board[r-k][c+k]; score += evaluateWindow(w, bot, player); }
    for (int r=0;r<=rows-4;r++) for (int c=0;c<=cols-4;c++){ char w[4]; for (int k=0;k<4;k++) w[k]=board[r+k][c+k]; score += evaluateWindow(w, bot, player); }
    return score;
}

int minimax(char board[rows][cols], int depth, int alpha, int beta, bool maximizing, char bot, char player, int *bestCol) {
    int valid[cols], cnt;
    getValidLocations(board, valid, &cnt);
    bool terminal = isTerminalNode(board, bot, player);
    if (depth==0 || terminal) {
        if (terminal) {
            if (checkWin(board, bot)) return 100000000;
            else if (checkWin(board, player)) return -100000000;
            else return 0;
        } else return scorePosition(board, bot, player);
    }
    if (maximizing) {
        int value = INT_MIN; int column = valid[0];
        for (int i=0;i<cnt;i++){
            int c = valid[i]; char tmp[rows][cols]; copyBoard(tmp, board);
            if (!update(tmp, c, bot)) continue;
            int sc = minimax(tmp, depth-1, alpha, beta, false, bot, player, NULL);
            if (sc > value) { value = sc; column = c; }
            if (value > alpha) alpha = value;
            if (alpha >= beta) break;
        }
        if (bestCol) *bestCol = column;
        return value;
    } else {
        int value = INT_MAX; int column = valid[0];
        for (int i=0;i<cnt;i++){
            int c = valid[i]; char tmp[rows][cols]; copyBoard(tmp, board);
            if (!update(tmp, c, player)) continue;
            int sc = minimax(tmp, depth-1, alpha, beta, true, bot, player, NULL);
            if (sc < value) { value = sc; column = c; }
            if (value < beta) beta = value;
            if (alpha >= beta) break;
        }
        if (bestCol) *bestCol = column;
        return value;
    }
}

int botMove(char board[rows][cols], char bot, char player, int difficulty) {
    int col;
    if (difficulty == 1) { do { col = rand()%cols; } while (board[0][col] != '.'); return col; }
    for (int j=0;j<cols;j++){ char t[rows][cols]; copyBoard(t, board); if (update(t,j,bot) && checkWin(t,bot)) return j; }
    for (int j=0;j<cols;j++){ char t[rows][cols]; copyBoard(t, board); if (update(t,j,player) && checkWin(t,player)) return j; }
    if (difficulty == 2) {
        if (board[0][cols/2]=='.') return cols/2;
        int offs[] = {0,1,-1,2,-2,3,-3};
        for (int k=0;k<7;k++){ int c=cols/2+offs[k]; if (c>=0 && c<cols && board[0][c]=='.') return c; }
        do{ col = rand()%cols; } while (board[0][col] != '.'); return col;
    }
    int depth = 6;
    int best = -1;
    int sc = minimax(board, depth, INT_MIN+1, INT_MAX-1, true, bot, player, &best);
    if (best < 0 || board[0][best] != '.') {
        if (board[0][cols/2]=='.') best = cols/2;
        else { int offs[] = {0,1,-1,2,-2,3,-3}; for (int k=0;k<7;k++){ int c=cols/2+offs[k]; if (c>=0 && c<cols && board[0][c]=='.'){ best=c; break; } } if (best<0){ do{ best=rand()%cols; } while (board[0][best] != '.'); } }
    }
    return best;
}

/* ---------- Client socket ---------- */
int start_client(const char *ip, int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); exit(1); }
    struct sockaddr_in serv; serv.sin_family = AF_INET; serv.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv.sin_addr) <= 0) { perror("inet_pton"); exit(1); }
    printf("Connecting to %s:%d ...\n", ip, port);
    if (connect(s, (struct sockaddr*)&serv, sizeof(serv)) < 0) { perror("connect"); exit(1); }
    printf("Connected.\n"); return s;
}

/* ---------- Main ---------- */
int main(int argc, char **argv) {
    srand((unsigned int)time(NULL));
    if (argc < 2) { printf("Usage: %s <server_ip> [port]\n", argv[0]); return 0; }
    const char *server_ip = argv[1];
    int port = 9000; if (argc >= 3) port = atoi(argv[2]);

    int sock = start_client(server_ip, port);

    int mode;
    if (recv_int(sock, &mode) < 0) { printf("Failed to receive mode. Exiting.\n"); close(sock); return 0; }
    printf("Mode from server: %s\n", (mode==1)?"PVP (you are human Player 2)":"PVB (you are bot Player 2)");

    char board[rows][cols];
    initialize(board);
    char A='X', B='O';

    while (1) {
        // receive board
        if (recv_all(sock, board, sizeof(board)) < 0) { printf("Server closed connection.\n"); break; }
        int status; if (recv_int(sock, &status) < 0) { printf("Server closed (status).\n"); break; }
        int yourTurn; if (recv_int(sock, &yourTurn) < 0) { printf("Server closed (turn).\n"); break; }

        // show board (helpful for human)
        printBoardLocal(board);

        if (status != 0) {
            if (status == 1) printf("Server (Player %c) WINS.\n", A);
            else if (status == 2) printf("Client (Player %c) WINS.\n", B);
            else printf("Draw.\n");
            break;
        }

        if (yourTurn == 1) {
            int chosenCol = -1;
            if (mode == 1) {
                printf("Your turn (Player %c). Enter column (1-%d): ", B, cols);
                fflush(stdout);
                chosenCol = getColumnLocal(cols);
            } else {
                int difficulty = 3;
                chosenCol = botMove(board, B, A, difficulty);
                printf("Bot chooses column %d\n", chosenCol + 1);
            }

            send_int(sock, chosenCol);
            update(board, chosenCol, B);
        }
    }

    close(sock);
    return 0;
}