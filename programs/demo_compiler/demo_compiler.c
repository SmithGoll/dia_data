/*
 * MIT License
 * 
 * Copyright (c) 2025 SmithGoll
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <errno.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

//////////////////////////////////////
//////// BUFFERED WCHAR ARRAY ////////
//////////////////////////////////////

#define WCHAR_BUFFER_BLOCK_SIZE 1024

typedef struct wchar_buffer_s {
    wchar_t buf[WCHAR_BUFFER_BLOCK_SIZE];
    unsigned int used_size;
    struct wchar_buffer_s* next;
} wchar_buffer_t;

wchar_buffer_t* wchar_buffer_new()
{
    wchar_buffer_t* res = (wchar_buffer_t*)malloc(sizeof(wchar_buffer_t));
    if (!res)
        return NULL;

    res->used_size = 0;
    res->next = NULL;
    return res;
}

void wchar_buffer_free(wchar_buffer_t* buffer)
{
    wchar_buffer_t* t;
    while (buffer) {
        t = buffer->next;
        free(buffer);
        buffer = t;
    }
    return;
}

wchar_buffer_t* wchar_buffer_get_avaliable_buffer(wchar_buffer_t* buffer)
{
    if (!buffer)
        return NULL;

    while (buffer->used_size == WCHAR_BUFFER_BLOCK_SIZE) {
        if (buffer->next == NULL) {
            buffer->next = wchar_buffer_new();
            return buffer->next;
        }

        buffer = buffer->next;
    }
    return buffer;
}

size_t wchar_buffer_get_length(wchar_buffer_t* buffer)
{
    size_t size = 0;

    while (buffer != NULL) {
        size += buffer->used_size;
        buffer = buffer->next;
    }
    return size;
}

wchar_t* wchar_buffer_to_string(wchar_buffer_t* buffer)
{
    wchar_t* wchar_array;
    size_t size;

    size = wchar_buffer_get_length(buffer);
    if (!size)
        return NULL;

    wchar_array = (wchar_t*)calloc(size + 1, sizeof(wchar_t));
    if (!wchar_array)
        return NULL;

    wchar_array[size] = L'\0';

    for (wchar_t* ptr = wchar_array; buffer != NULL; buffer = buffer->next) {
        size = buffer->used_size;
        (void)memcpy(ptr, buffer->buf, size * sizeof(wchar_t));
        ptr += size;
    }
    return wchar_array;
}

int wchar_buffer_putchar(wchar_buffer_t* buffer, wchar_t c)
{
    buffer = wchar_buffer_get_avaliable_buffer(buffer);

    if (!buffer)
        return -1;

    buffer->buf[buffer->used_size] = c;
    (buffer->used_size)++;
    return 0;
}

///////////////////////////////////
//////// TEXT WORD SPILTER ////////
///////////////////////////////////

typedef struct word_s {
    wchar_t* word;
    int line_number;
    struct word_s* next;
} word_t;

int line_counter = 1;
static inline int is_space(wchar_t c)
{
    switch (c) {
    case L'\n':
        line_counter++;
    case L'\t':
    case L' ':
        return 1;
    }
    return 0;
}

int parse_word(word_t* word, FILE* stream)
{
    wchar_t* tmp;
    word_t* prev = NULL;
    int word_counter = 0;
    wchar_buffer_t cb = {};

    wchar_t c;
    c = fgetwc(stream);
    // Unicode BOM
    if (c != 0xfeff)
        ungetwc(c, stream);

    while (!feof(stream)) {
        // Skip space
        while ((c = fgetwc(stream)) != EOF) {
            if (!is_space(c))
                break;
        }

        word->line_number = line_counter;

        if (c == WEOF) {
            break;
        } else if (c == L'"') {
            int parse_escape_char = 0;
            while ((c = fgetwc(stream)) != EOF) {
                if (parse_escape_char) {
                    switch (c) {
                    case L't':
                        c = L'\t';
                        break;
                    case L'n':
                        c = L'\n';
                        break;
                    default:
                        break;
                    }
                    parse_escape_char = 0;
                } else if (c == L'"')
                    break;
                else if (c == L'\\') {
                    parse_escape_char = 1;
                    continue;
                } else if (c == L'\n') {
                    line_counter++;
                }

                wchar_buffer_putchar(&cb, c);
            }
        } else {
            do {
                if (is_space(c))
                    break;
                wchar_buffer_putchar(&cb, c);
            } while ((c = fgetwc(stream)) != EOF);
        }

        tmp = wchar_buffer_to_string(&cb);
        if (!tmp) {
            word->word = NULL;
            fprintf(stderr, "Failed to alloc memory for string\n");
            return 0;
        }
        cb.used_size = 0;
        wchar_buffer_free(cb.next);

        word->word = tmp;
        word->next = (word_t*)malloc(sizeof(word_t));
        if (!(word->next)) {
            fprintf(stderr, "Failed to alloc memory\n");
            return 0;
        }

        prev = word;
        word = word->next;
        word_counter++;
    }
    if (prev) {
        free(prev->next);
        prev->next = NULL;
    }
    return word_counter;
}

void word_free(word_t* word)
{
    word_t* w;
    while (word) {
        w = word->next;
        if (word->word)
            free(word->word);
        free(word);
        word = w;
    }
    return;
}

/////////////////////////////////////
//////// BUFFERED BYTE ARRAY ////////
/////////////////////////////////////

#define BYTE_BUFFER_BLOCK_SIZE 1024

typedef struct byte_buffer_s {
    uint8_t buf[BYTE_BUFFER_BLOCK_SIZE];
    unsigned int used_size;
    struct byte_buffer_s* next;
} byte_buffer_t;

byte_buffer_t* byte_buffer_new()
{
    byte_buffer_t* res = (byte_buffer_t*)malloc(sizeof(byte_buffer_t));
    if (!res)
        return NULL;

    res->used_size = 0;
    res->next = NULL;
    return res;
}

void byte_buffer_free(byte_buffer_t* buffer)
{
    byte_buffer_t* t;
    while (buffer) {
        t = buffer->next;
        free(buffer);
        buffer = t;
    }
    return;
}

byte_buffer_t* byte_buffer_get_avaliable_buffer(byte_buffer_t* buffer)
{
    if (!buffer)
        return NULL;

    while (buffer->used_size == BYTE_BUFFER_BLOCK_SIZE) {
        if (buffer->next == NULL) {
            buffer->next = byte_buffer_new();
            return buffer->next;
        }

        buffer = buffer->next;
    }
    return buffer;
}

size_t byte_buffer_get_length(byte_buffer_t* buffer)
{
    size_t size = 0;

    while (buffer != NULL) {
        size += buffer->used_size;
        buffer = buffer->next;
    }
    return size;
}

uint8_t* byte_buffer_to_array(byte_buffer_t* buffer)
{
    uint8_t* byte_array;
    size_t size;

    size = byte_buffer_get_length(buffer);
    if (!size)
        return NULL;

    byte_array = (uint8_t*)malloc(size * sizeof(uint8_t));
    if (!byte_array)
        return NULL;

    for (uint8_t* ptr = byte_array; buffer != NULL; buffer = buffer->next) {
        size = buffer->used_size;
        (void)memcpy(ptr, buffer->buf, size * sizeof(uint8_t));
        ptr += size;
    }
    return byte_array;
}

int byte_buffer_put(byte_buffer_t* buffer, uint8_t c)
{
    buffer = byte_buffer_get_avaliable_buffer(buffer);

    if (!buffer)
        return -1;

    buffer->buf[buffer->used_size] = c;
    (buffer->used_size)++;
    return 0;
}

////////////////////////////////////
//////// COMPILER IMPLEMENT ////////
////////////////////////////////////

struct
{
    uint8_t opcode;
    const wchar_t* instr_name;
    const char* args;
    int needed_spr;
} instr_table[] = {
    { 1,  L"MOVE_CAMERA",                           "sss",      0 },
    { 2,  L"PAINT_CHAT",                            "bsz",      0 },
    { 4,  L"PAINT_SINGLE_DEMO_SPR_WITH_MOVE_ANIM",  "sssssss",  ((1 << 0) | (1 << 1) | (1 << 2)) },
    { 5,  L"SET_BLOCK_1",                           "ssb",      0 },
    { 6,  L"DELAY",                                 "i",        0 },
    { 7,  L"CONFIRM_WITHOUT_TIP",                   NULL,       0 },
    { 9,  L"SET_BLOCK_2",                           "sss",      0 },
    { 10, L"PLAYER_MOVE",                           "d",        0 },
    { 11, L"SET_DEMO_SPR",                          "ss",       ((1 << 0) | (1 << 2)) },
    { 12, L"SET_POS_AND_SHOW_DEMO_SPR",             "ss",       ((1 << 0) | (1 << 2)) },
    { 13, L"SET_SPR_MOVE_ANIM",                     "sss",      ((1 << 0) | (1 << 2)) },
    { 14, L"SHOW_DEMO_SPR",                         NULL,       ((1 << 0) | (1 << 2)) },
    { 15, L"HIDE_DEMO_SPR",                         NULL,       0 },
    { 16, L"SHOW_DECO_SYMBOL",                      "sb",       (1 << 1) },
    { 17, L"HIDE_DECO_SYMBOL",                      "sb",       0 },
    { 18, L"SCREEN_BLINK",                          "bc",       0 },
    { 25, L"SET_BLOCK_3",                           "ssbb",     0 },
    { 26, L"SET_BLOCK_4",                           "ssi",      0 },
    { 27, L"PAINT_HINT",                            "z",        0 }
};

#ifdef __WINNT__
#define wcscasecmp _wcsicmp
#endif

typedef struct instr_s {
    uint8_t opcode;
    uint8_t* param;
    int param_length;
    struct instr_s* next;
} instr_t;

int word_cmp(word_t* word, const wchar_t* s)
{
    if (!word || !s)
        return -1;
    return wcscasecmp(word->word, s);
}

static void write_intger_to_byte_buffer(byte_buffer_t* buffer, uint64_t val, int bytes_to_write, int line_number)
{
    uint64_t tmp = val;
    for (int i = 0; i < bytes_to_write; i++) {
        byte_buffer_put(buffer, (uint8_t)(tmp & 0xFF));
        tmp >>= 8;
    }

    if (tmp) {
        tmp <<= 8 * bytes_to_write;
        tmp = ~tmp;

        if (line_number > 0)
            fprintf(stderr, "Warning: integer %lu have cast to %lu at line %d\n", val, val & tmp, line_number);
        else
            fprintf(stderr, "Warning: integer %lu have cast to %lu\n", val, val & tmp);
    }
    return;
}

word_t* parse_instr_args(word_t* word, instr_t* instr, const char* args)
{
    uint8_t* params;
    int param_length;
    byte_buffer_t buffer = {};
    byte_buffer_t* string_buffer = NULL;

    unsigned long long tmp;

    // add check_args?
    if (!args)
        return word;

    for (int i = 0; args[i] != '\0'; i++) {
        int integer_bytes = 0;

        word = word->next;
        if (!word)
            return NULL;

        // reset errno
        errno = 0;

        switch (args[i]) {
        case 'b': // u8
        {
            integer_bytes = 1;
            break;
        }
        case 's': // u16
        {
            integer_bytes = 2;
            break;
        }
        case 'c': // rgb color
        {
            integer_bytes = 3;
            break;
        }
        case 'i': // u32
        {
            integer_bytes = 4;
            break;
        }
        case 'z': // string
        {
            size_t str_len;
            wchar_t* str = word->word;

            string_buffer = byte_buffer_new();
            if (!string_buffer) {
                goto fail;
            }

            str_len = wcslen(str);
            for (size_t j = 0; j < str_len; j++) {
                char mb[MB_CUR_MAX];
                mbstate_t state = {};

                size_t length = wcrtomb(mb, str[j], &state);
                if (errno != 0 || length == (size_t)-1) {
                    fprintf(stderr, "Failed to convert wchar to mutilbyte char\n");
                    goto fail;
                }

                for (size_t k = 0; k < length; k++) {
                    byte_buffer_put(string_buffer, (uint8_t)mb[k]);
                }
            }

            str_len = byte_buffer_get_length(string_buffer);
            if (str_len == 0) {
                fprintf(stderr, "Error: string length is 0 at line %d\n", word->line_number);
                goto fail;
            }
            if (str_len > __INT16_MAX__) {
                fprintf(stderr, "Error: string is too long (length > __INT16_MAX__) at line %d\n", word->line_number);
                goto fail;
            }

            byte_buffer_put(&buffer, (uint8_t)(str_len & 0xFF));
            byte_buffer_put(&buffer, (uint8_t)((str_len >> 8) & 0xFF));

            for (byte_buffer_t* b = string_buffer; b; b = b->next) {
                uint8_t* buf = b->buf;
                unsigned int used_size = b->used_size;
                for (unsigned int j = 0; j < used_size; j++) {
                    byte_buffer_put(&buffer, buf[j]);
                }
            }

            byte_buffer_free(string_buffer);
            string_buffer = NULL;
            break;
        }
        case 'd': // move direction
        {
            uint8_t direction;

            if (!word_cmp(word, L"up")) {
                direction = 1;
            } else if (!word_cmp(word, L"right")) {
                direction = 2;
            } else if (!word_cmp(word, L"down")) {
                direction = 3;
            } else if (!word_cmp(word, L"left")) {
                direction = 4;
            } else {
                fwprintf(stderr, L"Unkonwn move direction \"%ls\" at line %d\n", word->word, word->line_number);
                goto fail;
            }

            byte_buffer_put(&buffer, direction);
            break;
        }
        }

        if (integer_bytes) {
            if (integer_bytes == 3) { // rgb color
                tmp = wcstoull(word->word, NULL, 16);
            } else {
                tmp = wcstoull(word->word, NULL, 10);
            }

            if (errno == ERANGE) {
                // integer out of range
                fwprintf(stderr, L"Integer \"%ls\" at line %d is out of range\n", word->word, word->line_number);
                goto fail;
            }

            write_intger_to_byte_buffer(&buffer, tmp, integer_bytes, word->line_number);
        }
    }

    params = byte_buffer_to_array(&buffer);
    if (!params) {
        fprintf(stderr, "Failed to alloc mem for instr param\n");
        goto fail;
    }

    param_length = byte_buffer_get_length(&buffer);
    if (param_length > __INT32_MAX__) {
        fprintf(stderr, "Error: insruction is too large (length > __INT32_MAX__), please reduce the string length\n");
        goto fail;
    }

    instr->param = params;
    instr->param_length = param_length;

    byte_buffer_free(buffer.next);
    return word;

fail:
    byte_buffer_free(buffer.next);
    byte_buffer_free(string_buffer);
    return NULL;
}

void instr_free(instr_t* instr)
{
    instr_t* t;
    while (instr) {
        t = instr->next;
        if (instr->param)
            free(instr->param);
        free(instr);
        instr = t;
    }
    return;
}

int compile(FILE* out, word_t* word)
{
    int retval = -1;

    int depth = 0;
    int total_demo = 0;

    instr_t instr = {};
    instr_t* instr_block_ptr = NULL;

    byte_buffer_t demo_tmp = {};

    while (word) {
        int demo_id = 0;
        int instr_count = 0;
        int instr_size = 0;
        int needed_spr = 0;
        int total_spr_id = 0;
        int block_instr_counter = 0;

        instr_t* cur_instr = &instr;

        // parse demo header
        if (word_cmp(word, L"demo")) {
            fprintf(stderr, "Failed to parse demo header\n");
            return -1;
        }

        word = word->next;

        // parse demo id
        demo_id = (int)wcstoull(word->word, NULL, 10);
        write_intger_to_byte_buffer(&demo_tmp, (uint64_t)demo_id, 2, word->line_number);

        word = word->next;

        do {
            if (!word_cmp(word, L"{")) {
                depth++;
                // ckeck_depth
                if (depth > 2) {
                    fprintf(stderr, "Wrong depth %d\n", depth);
                    goto ret;
                }

                // instr_block
                if (depth == 2) {
                    // new_instr
                    if (instr_count) {
                        cur_instr->next = (instr_t*)malloc(sizeof(instr_t));
                        cur_instr = cur_instr->next;
                        if (cur_instr == NULL) {
                            fprintf(stderr, "Failed to alloc mem\n");
                            goto ret;
                        }
                        cur_instr->param = NULL;
                        cur_instr->param_length = 0;
                        cur_instr->next = NULL;
                    }

                    instr_count++;

                    instr_block_ptr = cur_instr;
                    instr_block_ptr->opcode = 0;
                }
            } else if (!word_cmp(word, L"}")) {
                depth--;
                // check_depth
                if (depth < 0) {
                    fprintf(stderr, "Wrong depth %d\n", depth);
                    goto ret;
                }

                // instr_block
                if (depth == 1) {
                    if (block_instr_counter > 255) {
                        fprintf(stderr, "Too many instr in blocks\n");
                        goto ret;
                    } else if (block_instr_counter == 0) {
                        fprintf(stderr, "No instruction in instr block at line %d\n", word->line_number);
                        goto ret;
                    }

                    instr_block_ptr->param = (uint8_t*)malloc(sizeof(uint8_t));
                    if (!(instr_block_ptr->param)) {
                        fprintf(stderr, "Failed to alloc mem for instr block param\n");
                        goto ret;
                    }

                    *(instr_block_ptr->param) = (uint8_t)block_instr_counter;
                    instr_block_ptr->param_length = 1;

                    instr_block_ptr = NULL;
                    block_instr_counter = 0;
                }
            } else {
                if (depth == 0) {
                    fprintf(stderr, "Instruction is not allowed in depth 0\n");
                    goto ret;
                }

                int instr_idx = -1;
                for (int i = 0; i < (sizeof(instr_table) / sizeof(instr_table[0])); i++) {
                    if (!word_cmp(word, instr_table[i].instr_name)) {
                        instr_idx = i;
                        break;
                    }
                }

                if (instr_idx == -1) {
                    fwprintf(stderr, L"Unkonwn instruction \'%ls\' at line %d\n", word->word, word->line_number);
                    goto ret;
                }

                // new_instr
                if (instr_count) {
                    cur_instr->next = (instr_t*)malloc(sizeof(instr_t));
                    cur_instr = cur_instr->next;
                    if (cur_instr == NULL) {
                        fprintf(stderr, "Failed to alloc mem\n");
                        goto ret;
                    }
                    cur_instr->param = NULL;
                    cur_instr->param_length = 0;
                    cur_instr->next = NULL;
                }

                if (instr_block_ptr)
                    block_instr_counter++;
                else
                    instr_count++;

                cur_instr->opcode = instr_table[instr_idx].opcode;
                needed_spr |= instr_table[instr_idx].needed_spr;

                if (!(word = parse_instr_args(word, cur_instr, instr_table[instr_idx].args))) {
                    fwprintf(stderr, L"Failed to parse arguments for %ls at line %d\n", word->word, word->line_number);
                    goto ret;
                }
            }

            word = word->next;
        } while (depth && word);

        if (depth != 0) {
            fprintf(stderr, "Wrong depth (expected 0, got %d)\n", depth);
            goto ret;
        }

        if (instr_count == 0) {
            fprintf(stderr, "Demo %d don't have any instructions\n", demo_id);
            goto ret;
        }

        for (instr_t* in = &instr; in; in = in->next) {
            instr_size += in->param_length + 1; // 1 byte for opcode
        }

        // instr_size is included spr_id size
        for (int j = 0; j < 3; j++) {
            if ((needed_spr >> j) & 1)
                total_spr_id++;
        }
        instr_size += (total_spr_id + 1) * 2;

        // save demo header
        write_intger_to_byte_buffer(&demo_tmp, (uint64_t)instr_count, 2, -1);
        write_intger_to_byte_buffer(&demo_tmp, (uint64_t)instr_size, 4, -1);
        {
            write_intger_to_byte_buffer(&demo_tmp, (uint64_t)total_spr_id, 2, -1);

            if (total_spr_id) {
                for (int j = 0; j < 3; j++) {
                    if ((needed_spr >> j) & 1)
                        write_intger_to_byte_buffer(&demo_tmp, (uint64_t)j, 2, -1);
                }
            }
        }

        // save instr
        for (instr_t* in = &instr; in; in = in->next) {
            uint8_t* param = in->param;
            int param_length = in->param_length;

            // opcode
            byte_buffer_put(&demo_tmp, in->opcode);
            // params
            for (int j = 0; j < param_length; j++) {
                byte_buffer_put(&demo_tmp, param[j]);
            }
        }

        if (instr.param)
            free(instr.param);
        instr_free(instr.next);
        instr.next = NULL;

        total_demo++;
    }

    // write to file
    {
        byte_buffer_t* t = &demo_tmp;

        // packed file header
        fputc(1, out);
        for (int j = 0; j < 4; j++) {
            fputc(0, out);
        }

        size_t chunk_size = byte_buffer_get_length(t) + 2; // 2 for storing total_demo
        for (int j = 0; j < 4; j++) {
            fputc(chunk_size & 0xFF, out);
            chunk_size >>= 8;
        }

        if (chunk_size) {
            fprintf(stderr, "Error: file is too large\n");
            goto ret;
        }

        // write total demo
        for (int j = 0; j < 2; j++) {
            fputc(total_demo & 0xFF, out);
            total_demo >>= 8;
        }

        if (total_demo) {
            fprintf(stderr, "Error: too many demos\n");
            goto ret;
        }

        while (t) {
            if (t->used_size) {
                fwrite(t->buf, sizeof(uint8_t), t->used_size, out);
            }
            t = t->next;
        }
        fflush(out);
    }

    retval = 0;
ret:
    instr_free(instr.next);
    byte_buffer_free(demo_tmp.next);
    return retval;
}

//////////////////////
//////// MAIN ////////
//////////////////////

FILE* safe_fopen(const char* name, const char* mode)
{
    FILE* fp = fopen(name, mode);
    if (!fp) {
        fprintf(stderr, "Failed to open \"%s\": %s\n", name, strerror(errno));
        exit(-1);
    }
    return fp;
}

int main(int argc, const char** argv)
{
    FILE* fp;
    int num_word;
    word_t word = {};

#ifdef __WINNT__
    if (!setlocale(LC_ALL, ".UTF-8"))
#else
    if (!setlocale(LC_ALL, "C.UTF-8"))
#endif // __WINNT__
    {
        fprintf(stderr, "Failed to set locale to utf8\n");
        return -2;
    }

    if (argc != 3) {
        fprintf(stderr, "Usage: demo_compiler [IN_FILE] [OUT_FILE]\n");
        return 1;
    }

    fp = safe_fopen(argv[1], "r");
    if (!(num_word = parse_word(&word, fp))) {
        fprintf(stderr, "No word parsed\n");
        if (word.word)
            free(word.word);
        word_free(word.next);
        fclose(fp);
        return -3;
    }
    fclose(fp);

    /*
        for(word_t *w = &word; w; w = w->next)
        {
            wprintf(L"%d: %ls\n", w->line_number, w->word);
        }
        word_free(word.next);
    */

    fp = safe_fopen(argv[2], "wb");
    int retval = compile(fp, &word);

    fclose(fp);
    if (word.word)
        free(word.word);
    word_free(word.next);

    return retval;
}
