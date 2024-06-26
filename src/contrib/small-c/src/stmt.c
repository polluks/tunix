#include <stdio.h>
#include <error.h>

#include "defs.h"
#include "data.h"
#include "ir.h"
#include "gen.h"
#include "lex.h"
#include "primary.h"
#include "while.h"
#include "expr.h"
#include "preproc.h"
#include "sym.h"
#include "struct.h"
#include "io.h"
#include "stmt.h"

// Statement parser
// Called whenever syntax requires a
// statement.  This routine performs
// that statement and returns a number
// telling which one.
// @param func func is true if we
// require a "function_statement", which
// must be compound, and must contain
// "statement_list" (even if
// "declaration_list" is omitted)
// Returns statement type (see defs.h).
int
statement (int func)
{
    if (!ch () && feof (input))
        return 0;
    lastst = 0;
    if (func) {
        if (match ("{")) {
            do_compound (YES);
            return lastst;
        } else
            perror ("function requires compound statement");
    }
    if (match ("{"))
        do_compound (NO);
    else
        do_statement ();
    return lastst;
}

int
statement_declare ()
{
    if (amatch ("register", 8))
        do_local_declares (DEFAUTO);
    else if (amatch ("auto", 4))
        do_local_declares (DEFAUTO);
    else if (amatch ("static", 6))
        do_local_declares (LSTATIC);
    else if (do_local_declares (AUTO));
    else
        return NO;
    return YES;
}

int
do_local_declares (int stclass)
{
    int type = 0;
    // Tag of struct object being
    // declared.
    int otag;
    int is_struct;
    char sname[NAMESIZE];
    blanks ();
    if ((is_struct = amatch ("struct", 6))
        || amatch ("union", 5)) {
        if (!symname (sname))
            illname ();
        if ((otag = find_tag (sname)) == -1)
            otag = define_struct (sname, stclass, is_struct);
        declare_local (STRUCT, stclass, otag);
    } else if ((type = get_type ()))
        declare_local (type, stclass, -1);
    else if (stclass == LSTATIC
             || stclass == DEFAUTO)
        declare_local (CINT, stclass, -1);
    else
        return 0;
    need_semicolon ();
    return 1;
}

void
do_statement ()
{
    if (amatch ("if", 2)) {
        doif ();
        lastst = STIF;
    } else if (amatch ("while", 5)) {
        dowhile ();
        lastst = STWHILE;
    } else if (amatch ("switch", 6)) {
        doswitch ();
        lastst = STSWITCH;
    } else if (amatch ("do", 2)) {
        dodo ();
        need_semicolon ();
        lastst = STDO;
    } else if (amatch ("for", 3)) {
        dofor ();
        lastst = STFOR;
    } else if (amatch ("return", 6)) {
        doreturn ();
        need_semicolon ();
        lastst = STRETURN;
    } else if (amatch ("break", 5)) {
        dobreak ();
        need_semicolon ();
        lastst = STBREAK;
    } else if (amatch ("continue", 8)) {
        docont ();
        need_semicolon ();
        lastst = STCONT;
    } else if (match (";"));
    else if (amatch ("case", 4)) {
        docase ();
        lastst = statement (NO);
    } else if (amatch ("default", 7)) {
        dodefault ();
        lastst = statement (NO);
    } else if (match ("#asm")) {
        doasm ();
        lastst = STASM;
    } else if (match ("{"))
        do_compound (NO);
    else {
        expression (YES);
        need_semicolon ();
        lastst = STEXP;
    }
}

// Compound statement
// TODO: Figure out arg and decls.
void
do_compound (int func)
{
    int decls = YES;
    ncmp++;
    while (!match ("}")) {
        if (feof (input))
            return;
        if (!decls)
            do_statement ();
        else if (!statement_declare ())
            decls = NO;
    }
    ncmp--;
}

void
doif ()
{
    int fstkp, flab1, flab2;
    int flev;
    flev = local_table_index;
    fstkp = stkp;
    flab1 = getlabel ();
    test (flab1, FALSE);
    statement (NO);
    stkp = gen_modify_stack (fstkp);
    local_table_index = flev;
    if (!amatch ("else", 4)) {
        def_local (flab1);
        return;
    }
    gen_jump (flab2 = getlabel ());
    def_local (flab1);
    statement (NO);
    stkp = gen_modify_stack (fstkp);
    local_table_index = flev;
    def_local (flab2);
}

void
dowhile ()
{
    WHILE ws;
    ws.symbol_idx = local_table_index;
    ws.stack_pointer = stkp;
    ws.type = WSWHILE;
    ws.case_test = getlabel ();
    ws.while_exit = getlabel ();
    addwhile (&ws);
    def_local (ws.case_test);
    test (ws.while_exit, FALSE);
    statement (NO);
    gen_jump (ws.case_test);
    def_local (ws.while_exit);
    local_table_index = ws.symbol_idx;
    stkp = gen_modify_stack (ws.stack_pointer);
    delwhile ();
}

void
dodo ()
{
    WHILE ws;
    ws.symbol_idx = local_table_index;
    ws.stack_pointer = stkp;
    ws.type = WSDO;
    ws.body_tab = getlabel ();
    ws.case_test = getlabel ();
    ws.while_exit = getlabel ();
    addwhile (&ws);
    def_local (ws.body_tab);
    statement (NO);
    if (!match ("while")) {
        perror ("missing while");
        return;
    }
    def_local (ws.case_test);
    test (ws.body_tab, TRUE);
    def_local (ws.while_exit);
    local_table_index = ws.symbol_idx;
    stkp = gen_modify_stack (ws.stack_pointer);
    delwhile ();
}

void
dofor ()
{
    WHILE ws;
    WHILE *pws;
    ws.symbol_idx = local_table_index;
    ws.stack_pointer = stkp;
    ws.type = WSFOR;
    ws.case_test = getlabel ();
    ws.incr_def = getlabel ();
    ws.body_tab = getlabel ();
    ws.while_exit = getlabel ();
    addwhile (&ws);
    pws = readwhile ();
    needbrack ("(");
    if (!match (";")) {
        expression (YES);
        need_semicolon ();
    }
    def_local (pws->case_test);
    if (!match (";")) {
        expression (YES);
        gen_test_jump (pws->body_tab, TRUE);
        gen_jump (pws->while_exit);
        need_semicolon ();
    } else
        pws->case_test = pws->body_tab;
    def_local (pws->incr_def);
    if (!match (")")) {
        expression (YES);
        needbrack (")");
        gen_jump (pws->case_test);
    } else
        pws->incr_def = pws->case_test;
    def_local (pws->body_tab);
    statement (NO);
    gen_jump (pws->incr_def);
    def_local (pws->while_exit);
    local_table_index = pws->symbol_idx;
    stkp = gen_modify_stack (pws->stack_pointer);
    delwhile ();
}

void
doswitch ()
{
    WHILE ws;
    WHILE *ptr;
    ws.symbol_idx = local_table_index;
    ws.stack_pointer = stkp;
    ws.type = WSSWITCH;
    ws.case_test = swstp;
    ws.body_tab = getlabel ();
    ws.incr_def = ws.while_exit = getlabel ();
    addwhile (&ws);
    gen_ldaci (0);   // TODO!!! -> LDACSP
    gen_local (ws.body_tab);
    gen_push (REGA);
    needbrack ("(");
    expression (YES);
    needbrack (")");
    // '?case' will adjust the stack 
    stkp = stkp + INTSIZE;
    gen_jump_case ();
    statement (NO);
    ptr = readswitch ();
    gen_jump (ptr->while_exit);
    dumpsw (ptr);
    def_local (ptr->while_exit);
    local_table_index = ptr->symbol_idx;
    stkp = gen_modify_stack (ptr->stack_pointer);
    swstp = ptr->case_test;
    delwhile ();
}

void
docase ()
{
    int val;
    val = 0;
    if (readswitch ()) {
        if (!number (&val))
            if (!quoted_char (&val)) {
                perror ("bad case label");
                return;
            }
        addcase (val);
        if (!match (":")) {
            perror ("missing colon");
            return;
        }
    } else {
        perror ("no active switch");
        return;
    }
}

void
dodefault ()
{
    WHILE *ptr;
    int lab;
    if ((ptr = readswitch ())) {
        ptr->incr_def = lab = getlabel ();
        def_local (lab);
        if (!match (":"))
            perror ("missing colon");
    } else
        perror ("no active switch");
}

void
doreturn ()
{
    if (!endst ())
        expression (YES);
    gen_jump (fexitlab);
}

void
dobreak ()
{
    WHILE *ptr;
    if (!(ptr = readwhile ()))
        return;
    gen_modify_stack (ptr->stack_pointer);
    gen_jump (ptr->while_exit);
}

void
docont ()
{
    WHILE *ptr;
    if ((ptr = findwhile ())) {
        gen_modify_stack (ptr->stack_pointer);
        if (ptr->type == WSFOR)
            gen_jump (ptr->incr_def);
        else
            gen_jump (ptr->case_test);
    }
}

// Dump switch table.
void
dumpsw (WHILE * ws)
{
    int i, j;
    gen_data_segment ();
    def_local (ws->body_tab);
    if (ws->case_test != swstp) {
        j = ws->case_test;
        while (j < swstp) {
            gen_dataw ();
            i = 4;
            while (i--) {
                outn (swstcase[j]);
                outb (',');
                gen_local (swstlab[j++]);
                if (!i || j >= swstp) {
                    break;
                }
                outb (',');
            }
        }
    }
    gen_dataw ();
    gen_local (ws->incr_def);
    outs (",0");
    gen_code_segment ();
}
