#include <ingle/cc65-charmap.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include <simpleio/libsimpleio.h>

#include "liblisp.h"

extern void error (char * msg);

lispptr
read_list ()
{
    cons * c;
    cons * start = NULL;
    cons * last = NULL;

    while (1) {
        skip_spaces ();
        if (eof ())
            error ("Closing paren?");
        if (in () == ')')
            return start;
        putback ();

        if (eof ())
            error ("Closing paren?");
        PUSH(start);
        PUSH(last);
        if (in () == '.')
            c = lisp_read ();
        else {
            putback ();
            c = lisp_make_cons (lisp_read (), nil);
        }
        POP(last);
        POP(start);
        if (last)
            last->cdr = c;
        else
            start = c;
        last = c;
    }
}

lispptr
read_number ()
{
    char * p = buffer;
    for (p = buffer; !eof () && isdigit (in ()); p++)
        *p = last_in;
    *p = 0;
    putback ();
    return lisp_make_number (atoi (buffer));
}

bool __fastcall__
our_isalpha (char c)
{
    return !isspace (c) && c != '(' && c != ')' && c != ';';
}

lispptr
read_symbol ()
{
    char * p;
    for (p = buffer; !eof () && our_isalpha (in ()); p++)
        *p = last_in;
    putback ();
    return lisp_make_symbol (buffer, p - buffer);
}

lispptr
read_string ()
{
    char * p;
    for (p = buffer; !eof () && in () != '"'; p++)
        if (last_in == '\\')
            *p = in ();
        else
            *p = last_in;
    return lisp_make_symbol (buffer, p - buffer);
}

lispptr
read_quoted (lispptr which)
{
    return lisp_make_cons (which, lisp_make_cons (lisp_read (), nil));
}

lispptr
lisp_read ()
{
    skip_spaces ();
    if (eof ())
        return nil;
    in ();
    if (last_in == ';')  {
        while (in () >= ' ')
            if (eof ())
                return nil;
        while (in () < ' ')
            if (eof ())
                return nil;
        putback ();
    }
    if (last_in == '(')
        return read_list ();
    if (last_in == '\'')
        return read_quoted (quote);
    if (last_in == '"')
        return read_string ();
    if (last_in == ')') {
        error ("Unexpected closing paren.");
        return nil;
    }
    putback ();
    // TODO: Negative numbers.
    if (isdigit (last_in))
        return read_number ();
    return read_symbol ();
}
