/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048
#define MAX_HISTORY 100

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
int cursor_pos;
char line_buffer[MAX_BUFFER_LINE];

char *history[MAX_HISTORY];
int history_length = 0;
int history_index = -1;

void add_to_history(char* line) {
  if (history_length < MAX_HISTORY) {
    history[history_length] = strdup(line);
    history_length++;
  }
  history_index=-1;

}

void clear_line() {
    // Move cursor to beginning
    while (cursor_pos > 0) {
        write(1, "\b", 1);
        cursor_pos--;
    }
    // Overwrite with spaces
    for (int i = 0; i < line_length; i++) {
        write(1, " ", 1);
    }
    // Move back to beginning again
    for (int i = 0; i < line_length; i++) {
        write(1, "\b", 1);
    }
    line_length = 0;
}

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
    // Set terminal to raw mode
    tty_raw_mode();

    line_length = 0;
    cursor_pos = 0;
    history_index = history_length; // Start history at the end

    // added changes to history list //

    while (1) {
        char ch;
        read(0, &ch, 1);

        // 1. Printable characters
        if (ch >= 32 && ch < 127) {
            if (line_length < MAX_BUFFER_LINE - 2) {
                // Shift characters to the right for insertion
                for (int i = line_length; i > cursor_pos; i--) {
                    line_buffer[i] = line_buffer[i - 1];
                }
                line_buffer[cursor_pos] = ch;
                line_length++;

                // Redraw line from cursor forward
                write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
                cursor_pos++;

                // Move cursor back to logical position
                for (int i = 0; i < line_length - cursor_pos; i++) {
                    write(1, "\b", 1);
                }
            }
        }
        // 2. Enter Key
        else if (ch == 10) {
            write(1, &ch, 1);
            break;
        }
        // 3. Ctrl-? (Usage)
        else if (ch == 31) {
            // Call your print_usage helper here if needed
        }
        // 4. Ctrl-H or Backspace
        else if (ch == 8 || ch == 127) {
            if (cursor_pos > 0) {
                cursor_pos--;
                for (int i = cursor_pos; i < line_length - 1; i++) {
                    line_buffer[i] = line_buffer[i + 1];
                }
                line_length--;

                write(1, "\b", 1); // Move back visually
                write(1, &line_buffer[cursor_pos], line_length - cursor_pos); // Redraw
                write(1, " ", 1); // Erase last char at end of line
                for (int i = 0; i <= line_length - cursor_pos; i++) {
                    write(1, "\b", 1); // Return to logical position
                }
            }
        }
        // 5. Ctrl-D (Delete)
        else if (ch == 4) {
            if (cursor_pos < line_length) {
                for (int i = cursor_pos; i < line_length - 1; i++) {
                    line_buffer[i] = line_buffer[i + 1];
                }
                line_length--;
                write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
                write(1, " ", 1);
                for (int i = 0; i <= line_length - cursor_pos; i++) {
                    write(1, "\b", 1);
                }
            }
        }
        // 6. Ctrl-A (Home)
        else if (ch == 1) {
            while (cursor_pos > 0) {
                write(1, "\b", 1);
                cursor_pos--;
            }
        }
        // 7. Ctrl-E (End)
        else if (ch == 5) {
            while (cursor_pos < line_length) {
                write(1, "\033[C", 3);
                cursor_pos++;
            }
        }
        // 8. Escape Sequences (Arrows)
        else if (ch == 27) {
            char ch1, ch2;
            read(0, &ch1, 1);
            read(0, &ch2, 1);
            if (ch1 == 91) {
                // UP ARROW
                if (ch2 == 65 && history_length > 0) {
                    if (history_index > 0) {
                        // Clear current line
                        for (int i = 0; i < cursor_pos; i++) write(1, "\b", 1);
                        for (int i = 0; i < line_length; i++) write(1, " ", 1);
                        for (int i = 0; i < line_length; i++) write(1, "\b", 1);

                        history_index--;
                        strcpy(line_buffer, history[history_index]);
                        line_length = strlen(line_buffer);
                        cursor_pos = line_length;
                        write(1, line_buffer, line_length);
                    }
                }
                // DOWN ARROW
                else if (ch2 == 66) {
                    // Clear current line
                    for (int i = 0; i < cursor_pos; i++) write(1, "\b", 1);
                    for (int i = 0; i < line_length; i++) write(1, " ", 1);
                    for (int i = 0; i < line_length; i++) write(1, "\b", 1);

                    if (history_index < history_length - 1) {
                        history_index++;
                        strcpy(line_buffer, history[history_index]);
                    } else {
                        history_index = history_length;
                        line_buffer[0] = 0;
                    }
                    line_length = strlen(line_buffer);
                    cursor_pos = line_length;
                    write(1, line_buffer, line_length);
                }
                // RIGHT ARROW
                else if (ch2 == 67) {
                    if (cursor_pos < line_length) {
                        write(1, "\033[C", 3);
                        cursor_pos++;
                    }
                }
                // LEFT ARROW
                else if (ch2 == 68) {
                    if (cursor_pos > 0) {
                        write(1, "\b", 1);
                        cursor_pos--;
                    }
                }
            }
        }
    }

    // Prepare line for the Lexer
    line_buffer[line_length] = 10; // Add newline
    line_buffer[line_length + 1] = 0; // Null terminate
    return line_buffer;
}
