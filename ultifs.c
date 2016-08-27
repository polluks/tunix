#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define FALSE   0
#define TRUE    1

#define IMAGE_SIZE      (8 * 1024 * 1024)

#define ULTIFS_ID       "ULTIFS"
#define JOURNAL_START   (64 * 1024)
#define JOURNAL_SIZE    (64 * 1024)
#define BLOCKS_START    (JOURNAL_START + JOURNAL_SIZE)
#define BLOCKS_SIZE     (64 * 1024)
#define NUM_BLOCKS      (BLOCKS_SIZE / sizeof (struct block))
#define FILES_START     (BLOCKS_START + BLOCKS_SIZE)
#define FILES_SIZE      (64 * 1024)
#define DATA_START      (FILES_START + FILES_SIZE)

char image[IMAGE_SIZE];

int32_t journal;
int32_t journal_free;
int32_t blocks;
int32_t files;
int32_t data_free;

#ifdef __GNUC__
#pragma pack(push, 1)   /* Disable struct padding. */
#endif

/*
 * File system/journal header
 */
struct ultifs_header {
    char    is_active;
    char    id[sizeof (ULTIFS_ID)];
    int16_t version;
    int32_t blocks;
    int32_t files;
};

struct block {
    int16_t update;
    int32_t pos;
    int16_t size;
    int16_t next;
};

struct file {
    int16_t update;
    int16_t first_block;
};

#define MAX_FILENAME_LENGTH     16

struct ultifs_dirent {
    short   update;
    char    name[MAX_FILENAME_LENGTH];
    char    type;
    short   file;
    long    size;
};

#ifdef __GNUC__
#pragma pack(pop)
#endif

void
clear_image ()
{
    memset (image, 0xff, IMAGE_SIZE);
}

void
img_write_mem (int32_t pos, void * str, size_t size)
{
    memcpy (&image[pos], str, size);
}

#define IMG_WRITE_STRUCT(pos, x)    img_write_mem (pos, &x, sizeof (x))

void
img_write_string (int pos, char * str)
{
    strcpy (&image[pos], str);
}

void
img_write_char (int pos, char x)
{
    *((char *) &image[pos]) = x;
}

void
img_read_mem (void * dest, int32_t pos, size_t size)
{
    memcpy (dest, &image[pos], size);
}

#define IMG_READ_STRUCT(x, pos)    img_read_mem (&x, pos, sizeof (x))

char
img_read_char (int32_t pos)
{
    return ((char *) &image)[pos];
}

void
mk_bootloader ()
{
    FILE * in;

    in = fopen ("src/flashboot/flashboot.bin", "r");
    fread (image, 65536, 1, in);
    fclose (in);
}

void
mk_journal ()
{
    struct ultifs_header header;

    header.is_active = 0;
    strcpy (header.id, ULTIFS_ID);
    header.version = 1;
    header.blocks = BLOCKS_START;
    header.files = FILES_START;
    IMG_WRITE_STRUCT(JOURNAL_START, header);
}

#define INITIAL_ROOT_DIRECTORY_SIZE     1024

void
mk_root_directory_block ()
{
    struct block block;

    block.update = -1;
    block.pos = DATA_START;
    block.size = INITIAL_ROOT_DIRECTORY_SIZE;
    block.next = -1;
    IMG_WRITE_STRUCT(BLOCKS_START, block);
}

void
mk_root_directory_file ()
{
    struct file file;

    file.update = -1;
    file.first_block = 0;
    IMG_WRITE_STRUCT(FILES_START, file);
}

void
mkfs ()
{
    clear_image ();
    mk_bootloader ();
    mk_journal ();
    mk_root_directory_block ();
    mk_root_directory_file ();
}

void
emit_image ()
{
    FILE * out;

    out = fopen ("image", "w");
    fwrite (image, IMAGE_SIZE, 1, out);
    fclose (out);
}

int32_t
find_free (int32_t start, size_t record_size, int num_records)
{
    int32_t p;
    char i;
    int j;
    char is_free;
    int n = 0;

    p = start;
    for (j = 0; j < num_records; j++) {
        is_free = TRUE;
        for (i = 0; i < record_size; i++) {
            if (img_read_char (p + i) == -1)
                continue;
            is_free = FALSE;
            break;
        }
        if (is_free)
            break;
        p += record_size;
        n++;
    }
    return p;
}

void
get_block (struct block * b, int16_t idx)
{
    int32_t pos = idx * sizeof (struct block) + BLOCKS_START;
    while (1) {
        img_read_mem (b, pos, sizeof (struct block));
        if (b->update == -1)
            return;
        pos = b->update << 3;
    }
}

int32_t
find_last_block_address ()
{
    struct block b;
    int i;
    int32_t highest = 0;
    int32_t tmp;

    for (i = 0; i < NUM_BLOCKS; i++) {
        get_block (&b, i);
        if (b.pos == -1)
            continue;
        tmp = b.pos + b.size;
        if (highest < tmp)
            highest = tmp;
    }
    return highest;
}

void
find_data_free ()
{
    int32_t highest = find_last_block_address ();
    int i;

    for (i = IMAGE_SIZE - 1; i > highest; i--) {
        if (img_read_char (i) == -1)
            continue;
        data_free = i + 1;
        return;
    }
    data_free = highest;
}

void
mount_journal (struct ultifs_header * h)
{
    journal = JOURNAL_START;
    journal_free = find_free (JOURNAL_START + sizeof (struct ultifs_header), 8, (JOURNAL_SIZE - sizeof (struct ultifs_header)) / 8);
}

void
mount_blocks (struct ultifs_header * h)
{
    blocks = h->blocks;
}

void
mount_files (struct ultifs_header * h)
{
    files = h->files;
}

void
mount ()
{
    struct ultifs_header h;

    IMG_READ_STRUCT(h, JOURNAL_START);
    mount_journal (&h);
    mount_blocks (&h);
    mount_files (&h);
    find_data_free ();
    printf ("%d\n", (int) data_free);
}

int16_t
alloc_journal (void * src, size_t size)
{
    int16_t j = journal_free >> 3;
    if (journal_free & 7)
        j++;
    img_write_mem (journal_free, src, size);
    journal_free += size;
    return j;
}

int16_t
alloc_block (int16_t size)
{
    int32_t i = find_free (BLOCKS_START, sizeof (struct block), BLOCKS_SIZE / sizeof (struct block));
    struct block b;
    b.update = -1;
    b.pos = data_free;
    b.size = size;
    b.next = -1;
    data_free += size ? size : 0x10000;
    IMG_WRITE_STRUCT(i, b);
    return i >> 3;
}

int16_t
alloc_file (int16_t b)
{
    int32_t i = find_free (FILES_START, sizeof (struct file), FILES_SIZE / sizeof (struct file));
    struct file f;
    f.update = -1;
    f.first_block = b;
    IMG_WRITE_STRUCT(i, f);
    return i / sizeof (struct file);
}

void
link_block (int16_t idx, int16_t next)
{
    struct block b;
    get_block (&b, idx);
    b.next = next;
    IMG_WRITE_STRUCT(idx, b);
}

int16_t
alloc_block_chain (size_t size)
{
    int16_t first_block = -1;
    int16_t last_block = -1;
    int16_t b;
    size_t s;
    while (size) {
        s = size > 65536 ? 65536 : size;
        b = alloc_block (s);
        if (last_block != -1)
            link_block (last_block, b);
        else
            first_block = b;
        size -= s;
    }
    return first_block;
}

int
main (int argc, char ** argv)
{
    mkfs ();
    mount ();
    alloc_block_chain (0x40000);
    emit_image ();

    return 0;
}
