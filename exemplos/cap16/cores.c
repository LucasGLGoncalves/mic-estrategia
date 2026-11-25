/* cores.c - conversando com o terminal por sequências de escape */
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
static void prepara_windows(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo;
    GetConsoleMode(h, &modo);
    SetConsoleMode(h, modo | 0x0004); /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
    SetConsoleOutputCP(CP_UTF8);
}
#else
static void prepara_windows(void) {}
#endif

#define ESC "\x1b"

int main(void) {
    prepara_windows();
    printf(ESC "[2J" ESC "[H"); /* limpa a tela e leva o cursor ao canto */
    printf(ESC "[1mAs 16 cores clássicas" ESC "[0m\n");
    for (int i = 0; i < 8; i++) printf(ESC "[%dm  %d  ", 40 + i, i);
    printf(ESC "[0m\n");
    for (int i = 0; i < 8; i++) printf(ESC "[%dm  %d  ", 100 + i, i + 8);
    printf(ESC "[0m\n\n");

    printf(ESC "[1m216 cores em cubo, mais 24 tons de cinza" ESC "[0m\n");
    for (int g = 0; g < 6; g++) {
        for (int r = 0; r < 6; r++) {
            for (int b = 0; b < 6; b++)
                printf(ESC "[48;5;%dm  ", 16 + 36 * r + 6 * g + b);
            printf(ESC "[0m ");
        }
        printf("\n");
    }
    for (int i = 232; i < 256; i++) printf(ESC "[48;5;%dm   ", i);
    printf(ESC "[0m\n\n");

    printf(ESC "[1mCores de 24 bits: um degradê do pôr do sol" ESC "[0m\n");
    for (int i = 0; i < 72; i++) {
        int r = 40 + i * 215 / 71, g = 20 + (i < 36 ? i * 4 : (71 - i) * 4),
            b = 120 - i * 100 / 71;
        printf(ESC "[48;2;%d;%d;%dm ", r, g, b);
    }
    printf(ESC "[0m\n\n");

    printf(ESC "[1mEstilos:" ESC "[0m " ESC "[1mnegrito" ESC "[0m " ESC
               "[2mapagado" ESC "[0m " ESC "[4msublinhado" ESC "[0m " ESC
               "[7minvertido" ESC "[0m\n\n");

    printf(ESC "[1mMolduras e blocos:" ESC "[0m\n");
    printf("┌──────┐ ╔══════╗ ░░▒▒▓▓██ ▀▄▀▄ ▲ ▼ ◄ ► • ·\n");
    printf("│ Ouro │ ║ Ferro║ ▏▎▍▌▋▊▉█\n");
    printf("└──────┘ ╚══════╝\n");

    printf(ESC "[20;50H" ESC "[33m@" ESC
               "[0m <- escrito na linha 20, coluna 50");
    printf(ESC "[22;1H");
    return 0;
}
