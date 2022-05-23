// UltiFS
//
// KERNAL emulation
//
// This is the interface between the UltiFS API and
// regular apps.
//
// Author: Sven Michael Klose <pixel@hugbox.org>

#pragma code-name ("ULTIFS")

#include <lib/ingle/cc65-charmap.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <cbm.h>

#include <lib/ultimem/ultimem.h>
#include <lib/posix/string.h>

#include "ultifs.h"

#define FALSE   0
#define TRUE    -1

// Client registers
#define accu          (*(char*) 0x9c00)
#define xreg          (*(char*) 0x9c01)
#define yreg          (*(char*) 0x9c02)
#define flags         (*(char*) 0x9c03)
#define proc_ustatus  (*(char*) 0x9c04)
#define proc_blk1     (*(int*) 0x9c05)
#define proc_blk2     (*(int*) 0x9c07)
#define proc_blk3     (*(int*) 0x9c09)
#define proc_blk5     (*(int*) 0x9c0b)

// CPU flags
#define FLAG_C          1
#define FLAG_Z          2
#define FLAG_I          4
#define FLAG_D          8
#define FLAG_B          16
#define FLAG_UNUSED     32
#define FLAG_V          64
#define FLAG_N          128

/* KERNAL zero page data */
#define FNLEN   (*(char*)  0xb7)    // File name length
#define LFN     (*(char*)  0xb8)    // Logical file number
#define SA      (*(char*)  0xb9)    // Secondary address
#define FA      (*(char*)  0xba)    // Device number
#define FNAME   (*(char**) 0xbb)    // File name pointer
#define STATUS  (*(char*)  0x90)    // Serial line status byte
#define DFLTN   (*(char*)  0x99)    // Logical input file number
#define DFLTO   (*(char*)  0x9A)    // Logical output file number

/* Serial line error codes */
#define STATUS_NO_DEVICE        0x80
#define STATUS_END_OF_FILE      0x40
#define STATUS_CHECKSUM_ERROR   0x20
#define STATUS_READ_ERROR       0x10
#define STATUS_LONG             0x08
#define STATUS_SHORT            0x04
#define STATUS_TIMEOUT_READ     0x02
#define STATUS_TIMEOUT_WRITE    0x01

/* KERNAL error codes */
#define OSERR_TOO_MANY_FILES        1
#define OSERR_FILE_ALREADY_OPEN     2
#define OSERR_FILE_NOT_OPEN         3
#define OSERR_FILE_NOT_FOUND        4
#define OSERR_DEVICE_NOT_PRESENT    5
#define OSERR_FILE_NOT_IN           6
#define OSERR_FILE_NOT_OUT          7
#define OSERR_MISSING_FILE_NAME     8
#define OSERR_ILLEGAL_DEVICE_NUMBER 9

/* Device error codes (read from command channel #15) */
#define ERR_BYTE_DECODING       24
#define ERR_WRITE               25
#define ERR_WRITE_PROTECT_ON    26
#define ERR_SYNTAX              30
#define ERR_INVALID_COMMAND     31
#define ERR_LONG_LINE           32
#define ERR_INVALID_FILE_NAME   33
#define ERR_NO_FILE_GIVEN       34
#define ERR_FILE_TOO_LARGE      52
#define ERR_FILE_OPEN_FOR_WRITE 60
#define ERR_FILE_NOT_OPEN       61
#define ERR_FILE_NOT_FOUND      62
#define ERR_FILE_EXISTS         63
#define ERR_FILE_TYPE_MISMATCH  64
#define ERR_NO_CHANNEL          70
#define ERR_DISK_FULL           72


typedef struct _channel {
    char *      name;
    bfile *     file;
    char        sa;

    char *      buf;
    char *      bufwptr;
    char *      bufrptr;
} channel;

#define NUM_LFN  32    // May not be the official limit. (pixel)

channel * channels[NUM_LFN];

channel cmd_channel = {
    NULL, NULL, 0, NULL, NULL, NULL
};


void
init_kernal_emulation ()
{
    bzero (channels, sizeof (channels));

    channels[12] = &cmd_channel;

    if (ultimem_unhide () != ULTIMEM_UNHIDE_ID_ULTIMEM) {
        printf ("No UltiMem found - exiting.\n");
        exit (0);
    }

    ultifs_mount ();
}


//
// Read/write client memory
//

char
peek_from_process (char * p)
{
    unsigned char ph = (unsigned) p >> 8;

    if (ph < 0x20 || ph > 0x7f)
        return *p;

    *ULTIMEM_CONFIG2 |= 0xc0;

    if (ph < 0x40) {
        *ULTIMEM_BLK5 = proc_blk1;
        return *(p - (char *) 0x2000 + (char *) 0xa000);
    }
    if (ph < 0x60) {
        *ULTIMEM_BLK5 = proc_blk2;
        return *(p - (char *) 0x4000 + (char *) 0xa000);
    }
    if (ph < 0x80) {
        *ULTIMEM_BLK5 = proc_blk3;
        return *(p - (char *) 0x6000 + (char *) 0xa000);
    }

    return *p;
}

char
poke_to_process (char * p, char v)
{
    unsigned char ph = (unsigned) p >> 8;

    if (ph < 0x20 || ph > 0x7f)
        return *p = v;

    *ULTIMEM_CONFIG2 |= 0xc0;

    if (ph < 0x40) {
        *ULTIMEM_BLK5 = proc_blk1;
        return *(p - (char *) 0x2000 + (char *) 0xa000) = v;
    }
    if (ph < 0x60) {
        *ULTIMEM_BLK5 = proc_blk2;
        return *(p - (char *) 0x4000 + (char *) 0xa000) = v;
    }
    if (ph < 0x80) {
        *ULTIMEM_BLK5 = proc_blk3;
        return *(p - (char *) 0x6000 + (char *) 0xa000) = v;
    }

    return *p = v;
}

char
downcase (char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    return c;
}

void
copy_from_process (char * to, char * from, char len)
{
    while (len--)
        *to++ = downcase (peek_from_process (from++));
}


//
// Channel buffers
//

void
clear_buf (channel * ch)
{
    free (ch->buf);
    ch->buf = NULL;
}

void
add_to_buf (channel * ch, void * ptr, size_t len)
{
    if (!ch->buf)
        ch->buf = ch->bufrptr = ch->bufwptr = malloc (2048);

    memcpy (ch->bufwptr, ptr, len);
    ch->bufwptr += len;
}

char
read_from_buf (channel * ch)
{
    char c = *ch->bufrptr++;

    if (ch->bufrptr == ch->bufwptr)
        clear_buf (ch);

    return c;
}


//
// Control channel #15
//

void
set_device_error (char code)
{
    channel *  ch = channels[LFN];
    char *     response;

    clear_buf (ch);

    if (!code)
        return;

    response = malloc (64);
    sprintf (response, "%d, ERROR, 0, 0", code);
    add_to_buf (ch, response, strlen (response));
    free (response);
}

void
open_command (char * name)
{
    channel *  ch = channels[15];
    char *     msg = "fooooo!!";

    add_to_buf (ch, msg, strlen (msg));

    STATUS = 0;
}


//
// Directory
//

void
make_directory_list (channel * ch)
{
    struct cbm_dirent * dirent = malloc (sizeof (struct cbm_dirent));
    char *         line = malloc (256);
    unsigned       line_addr = 0x1201;
    unsigned char  i;
    unsigned char  l;
    char           c;
    char *         p;
    size_t         len;
    char *         type;

    // Emit start address.
    line[0] = line_addr & 255;
    line[1] = line_addr >> 8;
    add_to_buf (ch, line, 2);

    ultifs_opendir ();

    // Emit directory entries as BASIC lines.
    while (!ultifs_readdir (dirent)) {
        // Skip pointer to next line. (Updated later.)
        p = line + 2;

        // Write file size to line number.
        *p++ = dirent->size & 255;
        *p++ = dirent->size >> 8;

        // Write file name in quotes.
        *p++ = '"';
        l = strnlen (dirent->name, 17);
        for (i = 0; i < l; i++) {
            c = toupper (dirent->name[i]);
            *p++ = c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
        }
        *p++ = '"';

        *p++ = ' ';

        // Write file type.
        switch (dirent->type) {
            case CBM_T_DEL:
                type = "DEL"; break;
            case CBM_T_SEQ:
                type = "SEQ"; break;
            case CBM_T_PRG:
                type = "PRG"; break;
            case CBM_T_USR:
                type = "USR"; break;
            case CBM_T_REL:
                type = "REL"; break;
            case CBM_T_CBM:
                type = "CBM"; break;
            case CBM_T_DIR:
                type = "DIR"; break;
            case CBM_T_LNK:
                type = "LNK"; break;
            case CBM_T_VRP:
                type = "VRP"; break;
            case CBM_T_HEADER:
                type = "HDR"; break;
            case CBM_T_OTHER:
            default:
                type = "???"; break;
        }
        *p++ = *type++;
        *p++ = *type++;
        *p++ = *type;

        *p++ = 0;

        // Update pointer to next line.
        len = p - line;
        line_addr += len;
        line[0] = line_addr & 255;
        line[1] = line_addr >> 8;

        //  Output line.
        add_to_buf (ch, line, len);
    }

    // Emit EOT (high byte of pointer to next line is 0).
    line[0] = line[1] = 0;
    add_to_buf (ch, line, 2);

    free (dirent);
    free (line);

    ultifs_closedir ();
    accu = flags = 0;
}


//
// KERNAL emulation
//

void
free_channel ()
{
    channel * ch = channels[LFN];

    clear_buf (ch);
    free (ch->name);
    free (ch);

    channels[LFN] = NULL;
}

char
ultifs_kopen ()
{
    char *     name = NULL;
    bfile *    found_file;
    channel *  ch;

    if (!LFN) {
        accu = OSERR_FILE_NOT_IN;
        goto error2;
    }
    if (SA != 15 && channels[LFN]) {
        accu = OSERR_FILE_ALREADY_OPEN;
        goto error2;
    }

    accu = flags = 0;

    if (FNLEN) {
        name = malloc (FNLEN + 1);
        copy_from_process (name, FNAME, FNLEN);
        name[FNLEN] = 0;
    }

    ch = malloc (sizeof (channel));
    ch->sa = SA;
    ch->buf = NULL;
    channels[LFN] = ch;

    if (SA == 15) {
        open_command (name);
        return TRUE;
    }

    if (FNLEN == 1 && *name == '$') {
        make_directory_list (ch);
        return TRUE;
    }

    found_file = ultifs_open (ultifs_pwd, name, 0);
    if (!found_file) {
        accu = OSERR_FILE_NOT_FOUND;
        goto error;
    }

    ch->file = found_file;

    return TRUE;

error:
    free_channel ();
error2:
    flags = FLAG_C;

    return FALSE;
}

void
ultifs_kchkin ()
{
    flags = 0;
    if (accu = channels[LFN] ? 0 : OSERR_FILE_NOT_OPEN)
        flags = FLAG_C;
}

void
ultifs_kchkout ()
{
    if (!channels[LFN]) {
        accu = OSERR_FILE_NOT_OPEN;
        goto error;
    }
    if (SA != 15) {
        accu = OSERR_FILE_NOT_OUT;
        goto error;
    }

    accu = flags = 0;
    accu = OSERR_FILE_NOT_OUT;
    return;

error:
    flags = FLAG_C;
}

char
ultifs_kbasin ()
{
    channel *  ch = channels[LFN];
    bfile *    file;

    accu = flags = STATUS = 0;

    if (!ch)
        goto file_not_open;

    if (ch->buf)
        return accu = read_from_buf (ch);

    file = ch->file;
    if (file->pos >= file->size)
        goto end_of_file;

    return accu = bfile_read (file);

end_of_file:
    STATUS = STATUS_END_OF_FILE;

    return 0;

file_not_open:
    accu = OSERR_FILE_NOT_OPEN;
    flags = FLAG_C;
    return 0;
}

void
ultifs_kbsout ()
{
    accu = OSERR_DEVICE_NOT_PRESENT;
    STATUS = STATUS_TIMEOUT_WRITE;
    flags = FLAG_C;
}

void
ultifs_kclose ()
{
    channel * ch = channels[LFN];
    if (!ch)
        return;

    if (ch->file)
        bfile_close (ch->file);
    free_channel ();
}

void
ultifs_kclall ()
{
    char  old_LFN = LFN;

    for (LFN = 0; LFN < NUM_LFN; LFN++)
        if (channels[LFN])
            ultifs_kclose ();

    LFN = old_LFN;
}

void
ultifs_kload ()
{
    char           do_verify = accu;
    char           old_LFN = LFN;
    char *         addr;
    unsigned char  addr_l;
    unsigned char  addr_h;

    STATUS = flags = 0;
    LFN = NUM_LFN - 1;

    if (!ultifs_kopen ())
        goto end;

    // Read destination address.
    addr_l = ultifs_kbasin ();
    addr_h = ultifs_kbasin ();

    // When SA=0 pverride with address to YX.
    if (!SA) {
        addr_l = xreg;
        addr_h = yreg;
    }

    // Read all bytes.
    addr = (char *) ((unsigned) addr_h << 8 | (unsigned) addr_l);
    while (!STATUS) {
        ultifs_kbasin ();
        if (STATUS & STATUS_END_OF_FILE)
            break;
        poke_to_process (addr++, accu);
    }

    ultifs_kclose ();

    // Return next free address.
    xreg = (unsigned) addr & 255;
    yreg = (unsigned) addr >> 8;

end:
    LFN = old_LFN;
}

void
ultifs_ksave ()
{
    char    old_LFN = LFN;
    char *  ptr = *(char **) accu;
    char *  end = (char*) (yreg << 8 + xreg);

    STATUS = flags = 0;
    LFN = NUM_LFN - 1;

    if (!ultifs_kopen ())
        goto end;

    while (!STATUS) {
        accu = peek_from_process (ptr++);
        ultifs_kbsout ();
        if (end == ptr)
            break;
    }

    ultifs_kclose ();

end:
    LFN = old_LFN;
}
