#ifndef LINE_H
#define LINE_H

#define MAX_LINE_LENGTH     256

typedef struct _line line;

typedef struct _line {
    unsigned    version;
    unsigned    version_deleted;
    line        * newer;
} line;

typedef struct _linestack linestack;

typedef struct _linestack {
    linestack   * prev;
    linestack   * next;
    char        data;
} linestack;

void line_clear         (void);
void line_insert_char   (char c);
void line_delete_char   (void);
void line_redraw        (void);
void line_move_left     (void);
void line_move_right    (void);
void line_commit        (void);

void linestack_delete   (void);
void linestack_insert   (void);
void linestack_open     (void);
void linestack_init     (void);
void linestack_test     (void);

void screen_redraw      (void);

#endif // #ifndef LINE_H
