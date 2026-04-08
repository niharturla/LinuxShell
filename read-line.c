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

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char * history [] = {
  "ls -al | grep x", 
  "ps -e",
  "cat read-line-example.c",
  "vi hello.c",
  "make",
  "ls -al | grep xxx | grep yyy"
};
int history_length = sizeof(history)/sizeof(char *);

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
    tty_raw_mode();
    int line_length = 0;
    int cursor_pos = 0; // Initialize cursor at start

    while (1) {
        char ch;
        read(0, &ch, 1);

        if (ch >= 32 && ch < 127) {
            // Printable Character: Handle Insertion
            if (line_length < MAX_BUFFER_LINE - 2) {
                // Shift characters to the right to make room
                for (int i = line_length; i > cursor_pos; i--) {
                    line_buffer[i] = line_buffer[i-1];
                }
                line_buffer[cursor_pos] = ch;
                line_length++;
                
                // Write the new character and everything after it
                write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
                cursor_pos++;

                // Move cursor back to the correct position
                for (int i = 0; i < line_length - cursor_pos; i++) {
                    write(1, "\b", 1);
                }
            }
        }
        else if (ch == 10) { // ENTER
            write(1, &ch, 1);
            break;
        }
        else if (ch == 1) { // Ctrl-A: HOME
            while (cursor_pos > 0) {
                write(1, "\b", 1);
                cursor_pos--;
            }
        }
        else if (ch == 5) { // Ctrl-E: END
            while (cursor_pos < line_length) {
                write(1, "\033[C", 3);
                cursor_pos++;
            }
        }
        else if (ch == 4) { // Ctrl-D: DELETE
            if (cursor_pos < line_length) {
                for (int i = cursor_pos; i < line_length - 1; i++) {
                    line_buffer[i] = line_buffer[i+1];
                }
                line_length--;
                // Redraw tail + clear last char
                write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
                write(1, " ", 1);
                // Move back
                for (int i = 0; i <= line_length - cursor_pos; i++) {
                    write(1, "\b", 1);
                }
            }
        }
        else if (ch == 8 || ch == 127) { // Backspace (Ctrl-H or 127)
            if (cursor_pos > 0) {
                cursor_pos--;
                for (int i = cursor_pos; i < line_length - 1; i++) {
                    line_buffer[i] = line_buffer[i+1];
                }
                line_length--;
                
                write(1, "\b", 1); // Visual back
                write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
                write(1, " ", 1); // Clear end
                for (int i = 0; i <= line_length - cursor_pos; i++) {
                    write(1, "\b", 1);
                }
            }
        }
        else if (ch == 27) { // ESC sequence
            char ch1, ch2;
            read(0, &ch1, 1);
            read(0, &ch2, 1);
            if (ch1 == 91) {
                if (ch2 == 68) { // LEFT ARROW
                    if (cursor_pos > 0) {
                        write(1, "\b", 1);
                        cursor_pos--;
                    }
                }
                else if (ch2 == 67) { // RIGHT ARROW
                    if (cursor_pos < line_length) {
                        write(1, "\033[C", 3);
                        cursor_pos++;
                    }
                }
                else if (ch2 == 65) { // UP ARROW (History Logic)
                    // ... [Existing history erase logic from your prompt] ...
                    // Reset cursor_pos to line_length after loading history
                    cursor_pos = line_length;
                }
            }
        }
    }

    line_buffer[line_length] = 10;
    line_buffer[line_length+1] = 0;
    return line_buffer;
}
