#ifndef ANSI_COLORS_H
#define ANSI_COLORS_H

// Reset (removes color & formatting)
#define RESET       "\x1b[0m"

#define BLACK       "\x1b[30m"
#define RED         "\x1b[31m"
#define GREEN       "\x1b[32m"
#define YELLOW      "\x1b[33m"
#define BLUE        "\x1b[34m"
#define MAGENTA     "\x1b[35m"
#define CYAN        "\x1b[36m"
#define WHITE       "\x1b[37m"

#define BRIGHT_BLACK   "\x1b[90m"
#define BRIGHT_RED     "\x1b[91m"
#define BRIGHT_GREEN   "\x1b[92m"
#define BRIGHT_YELLOW  "\x1b[93m"
#define BRIGHT_BLUE    "\x1b[94m"
#define BRIGHT_MAGENTA "\x1b[95m"
#define BRIGHT_CYAN    "\x1b[96m"
#define BRIGHT_WHITE   "\x1b[97m"

//Background Colors
#define BG_BLACK       "\x1b[40m"
#define BG_RED         "\x1b[41m"
#define BG_GREEN       "\x1b[42m"
#define BG_YELLOW      "\x1b[43m"
#define BG_BLUE        "\x1b[44m"
#define BG_MAGENTA     "\x1b[45m"
#define BG_CYAN        "\x1b[46m"
#define BG_WHITE       "\x1b[47m"

// Bright Background Colors
#define BG_BRIGHT_BLACK   "\x1b[100m"
#define BG_BRIGHT_RED     "\x1b[101m"
#define BG_BRIGHT_GREEN   "\x1b[102m"
#define BG_BRIGHT_YELLOW  "\x1b[103m"
#define BG_BRIGHT_BLUE    "\x1b[104m"
#define BG_BRIGHT_MAGENTA "\x1b[105m"
#define BG_BRIGHT_CYAN    "\x1b[106m"
#define BG_BRIGHT_WHITE   "\x1b[107m"

// Text Formatting
#define BOLD        "\x1b[1m"
#define DIM         "\x1b[2m"
#define ITALIC      "\x1b[3m"
#define UNDERLINE   "\x1b[4m"
#define BLINK       "\x1b[5m"
#define REVERSE     "\x1b[7m"
#define HIDDEN      "\x1b[8m"

// Cursor & Screen Control
#define CLEAR_SCREEN   "\x1b[2J"
#define CURSOR_HOME    "\x1b[H"
#define CURSOR_POS(y, x) "\x1b[" #y ";" #x "H"

#endif // ANSI_COLORS_H
