#include "z-doc.h"

#include "angband.h"
#include "c-string.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif

#define _INVALID_COLOR 255

doc_pos_t doc_pos_create(int x, int y)
{
    doc_pos_t result;
    result.x = x;
    result.y = y;
    return result;
}

doc_pos_t doc_pos_invalid(void)
{
    return doc_pos_create(-1, -1);
}

bool doc_pos_is_valid(doc_pos_t pos)
{
    if (pos.x >= 0 && pos.y >= 0)
        return TRUE;
    return FALSE;
}

int doc_pos_compare(doc_pos_t left, doc_pos_t right)
{
    if (left.y < right.y)
        return -1;
    if (left.y > right.y)
        return 1;
    if (left.x < right.x)
        return -1;
    if (left.x > right.x)
        return 1;
    return 0;
}

doc_region_t doc_region_create(int x1, int y1, int x2, int y2)
{
    doc_region_t result;
    result.start.x = x1;
    result.start.y = y1;
    result.stop.x = x2;
    result.stop.y = y2;
    return result;
}

doc_region_t doc_region_invalid(void)
{
    doc_region_t result;
    result.start = doc_pos_invalid();
    result.stop = doc_pos_invalid();
    return result;
}

bool doc_region_is_valid(doc_region_ptr region)
{
    if (region)
    {
        if ( doc_pos_is_valid(region->start)
          && doc_pos_is_valid(region->stop)
          && doc_pos_compare(region->start, region->stop) <= 0 )
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool doc_region_contains(doc_region_ptr region, doc_pos_t pos)
{
    if (doc_region_is_valid(region) && doc_pos_is_valid(pos))
    {
        if ( doc_pos_compare(region->start, pos) <= 0
          && doc_pos_compare(region->stop, pos) > 0 )
        {
            return TRUE;
        }
    }
    return FALSE;
}

int doc_region_line_count(doc_region_ptr region)
{
    int result = 0;
    if (doc_region_is_valid(region))
    {
        result = region->stop.y - region->start.y;
        if (region->stop.x > 0)
            result++;
    }
    return result;
}

static void _doc_bookmark_free(vptr pv)
{
    doc_bookmark_ptr mark = pv;
    string_free(mark->name);
    free(mark);
}

static void _doc_link_free(vptr pv)
{
    doc_link_ptr link = pv;
    string_free(link->file);
    string_free(link->topic);
    free(link);
}

#define PAGE_HEIGHT 128
#define PAGE_NUM(a) ((a)>>7)
#define PAGE_OFFSET(a) ((a)%128)

static void _normal_style(doc_style_ptr style)
{
    style->color = TERM_WHITE;
    style->right = 72;
}

static void _note_style(doc_style_ptr style)
{
    style->color = TERM_L_GREEN;
    style->left = 4;
    style->right = 60;
}

static void _title_style(doc_style_ptr style)
{
    style->color = TERM_L_BLUE;
}

static void _heading_style(doc_style_ptr style)
{
    style->color = TERM_RED;
}

static void _keyword_style(doc_style_ptr style)
{
    style->color = TERM_L_RED;
}

static void _keypress_style(doc_style_ptr style)
{
    style->color = TERM_ORANGE;
}

static void _link_style(doc_style_ptr style)
{
    style->color = TERM_L_GREEN;
}

static void _screenshot_style(doc_style_ptr style)
{
    style->right = 255;
    style->options |= DOC_STYLE_NO_WORDWRAP;
}

static void _table_style(doc_style_ptr style)
{
    style->right = 255;
    style->options |= DOC_STYLE_NO_WORDWRAP;
}

static void _selection_style(doc_style_ptr style)
{
    style->color = TERM_YELLOW;
}

static void _indent_style(doc_style_ptr style)
{
    style->indent = 2;
}

static void _wide_style(doc_style_ptr style)
{
    style->right = 255;
}

/* doc_style_f <-> void * is forbidden in ISO C ... I'm not sure
   what the correct idiom is for a table of function pointers? */
static void _add_doc_style_f(doc_ptr doc, cptr name, doc_style_f f)
{
    doc_style_f *pf = malloc(sizeof(doc_style_f));
    *pf = f;
    str_map_add(doc->styles, name, pf);
}

static doc_style_f _get_doc_style_f(doc_ptr doc, cptr name)
{
    doc_style_f *pf = str_map_find(doc->styles, name);
    if (pf)
        return *pf;
    return NULL;
}

doc_ptr doc_alloc(int width)
{
    doc_ptr     res = malloc(sizeof(doc_t));
    doc_style_t style = {0};

    res->cursor.x = 0;
    res->cursor.y = 0;
    res->selection = doc_region_invalid();
    res->width = width;
    res->pages = vec_alloc(free);
    res->styles = str_map_alloc(free);
    res->bookmarks = vec_alloc(_doc_bookmark_free);
    res->links = int_map_alloc(_doc_link_free);
    res->style_stack = vec_alloc(free);
    res->name = string_alloc();
    res->html_header = string_alloc();
    res->html_footer = string_alloc();

    /* Default Styles */
    _add_doc_style_f(res, "normal", _normal_style);
    _add_doc_style_f(res, "note", _note_style);
    _add_doc_style_f(res, "title", _title_style);
    _add_doc_style_f(res, "heading", _heading_style);
    _add_doc_style_f(res, "keyword", _keyword_style);
    _add_doc_style_f(res, "keypress", _keypress_style);
    _add_doc_style_f(res, "link", _link_style);
    _add_doc_style_f(res, "screenshot", _screenshot_style);
    _add_doc_style_f(res, "table", _table_style);
    _add_doc_style_f(res, "selection", _selection_style);
    _add_doc_style_f(res, "indent", _indent_style);
    _add_doc_style_f(res, "wide", _wide_style);

    /* bottom of the style stack is *always* "normal" and can never be popped */
    _normal_style(&style);
    doc_push_style(res, &style);

    return res;
}

void doc_free(doc_ptr doc)
{
    if (doc)
    {
        vec_free(doc->pages);
        str_map_free(doc->styles);
        vec_free(doc->bookmarks);
        int_map_free(doc->links);
        vec_free(doc->style_stack);
        string_free(doc->name);
        string_free(doc->html_header);
        string_free(doc->html_footer);

        free(doc);
    }
}

void doc_change_name(doc_ptr doc, cptr name)
{
    string_clear(doc->name);
    string_append_s(doc->name, name);
}

void doc_change_html_header(doc_ptr doc, cptr header)
{
    string_clear(doc->html_header);
    string_append_s(doc->html_header, header);
}

void doc_change_html_footer(doc_ptr doc, cptr footer)
{
    string_clear(doc->html_footer);
    string_append_s(doc->html_footer, footer);
}

doc_pos_t doc_cursor(doc_ptr doc)
{
    return doc->cursor;
}

int doc_line_count(doc_ptr doc)
{
    doc_region_t r = doc_range_all(doc);
    return doc_region_line_count(&r);
}

int doc_width(doc_ptr doc)
{
    return doc->width;
}

doc_region_t doc_range_all(doc_ptr doc)
{
    doc_region_t result;
    result.start = doc_pos_create(0, 0);
    result.stop = doc->cursor;
    return result;
}

doc_region_t doc_range_selection(doc_ptr doc)
{
    return doc->selection;
}

doc_region_t doc_range_top(doc_ptr doc, doc_pos_t stop)
{
    doc_region_t result;
    result.start.x = 0;
    result.start.y = 0;
    result.stop = stop;
    if (doc_pos_compare(doc->cursor, result.stop) < 0)
        result.stop = doc->cursor;
    return result;
}

doc_region_t doc_range_top_lines(doc_ptr doc, int count)
{
    doc_region_t result;
    result.start.x = 0;
    result.start.y = 0;
    result.stop.x = 0;
    result.stop.y = count; /* Remember: [start, stop)! */
    if (doc_pos_compare(doc->cursor, result.stop) < 0)
        result.stop = doc->cursor;
    return result;
}

doc_region_t doc_range_bottom(doc_ptr doc, doc_pos_t start)
{
    doc_region_t result;
    if (doc_pos_compare(doc->cursor, start) < 0)
        return doc_region_invalid();
    result.start = start;
    result.stop = doc->cursor;
    return result;
}

doc_region_t doc_range_bottom_lines(doc_ptr doc, int count)
{
    doc_region_t result;
    result.start.x = 0;
    if (doc->cursor.x > 0)
        result.start.y = MAX(0, doc->cursor.y - (count - 1));
    else
        result.start.y = MAX(0, doc->cursor.y - count);
    result.stop = doc->cursor;
    return result;
}

doc_region_t doc_range_middle(doc_ptr doc, doc_pos_t start, doc_pos_t stop)
{
    doc_region_t result;
    if (doc_pos_compare(doc->cursor, start) < 0)
        return doc_region_invalid();
    result.start = start;
    result.stop = stop;
    if (doc_pos_compare(doc->cursor, result.stop) < 0)
        result.stop = doc->cursor;
    return result;
}

doc_region_t doc_range_middle_lines(doc_ptr doc, int start_line, int stop_line)
{
    doc_region_t result;
    result.start.x = 0;
    result.start.y = start_line;
    result.stop.x = doc->width;
    result.stop.y = stop_line ;

    if (doc_pos_compare(doc->cursor, result.start) < 0)
        return doc_region_invalid();
    if (doc_pos_compare(doc->cursor, result.stop) < 0)
        result.stop = doc->cursor;
    return result;
}

doc_pos_t doc_next_bookmark(doc_ptr doc, doc_pos_t pos)
{
    int i;

    for (i = 0; i < vec_length(doc->bookmarks); i++)
    {
        doc_bookmark_ptr mark = vec_get(doc->bookmarks, i);

        if (doc_pos_compare(pos, mark->pos) < 0)
            return mark->pos;
    }
    return doc_pos_invalid();
}

doc_pos_t doc_next_bookmark_char(doc_ptr doc, doc_pos_t pos, int c)
{
    int i;
    for (i = 0; i < vec_length(doc->bookmarks); i++)
    {
        doc_bookmark_ptr mark = vec_get(doc->bookmarks, i);
        cptr             name;

        assert(mark->name);
        name = string_buffer(mark->name);

        if ( doc_pos_compare(pos, mark->pos) <= 0 /* Subtle: So I can wrap and start at (0,0) *and* pick up an initial topic */
          && string_length(mark->name) > 0
          && tolower(name[0]) == tolower(c) )
        {
            return mark->pos;
        }
    }
    return doc_pos_invalid();
}

doc_pos_t doc_prev_bookmark(doc_ptr doc, doc_pos_t pos)
{
    int i;
    doc_bookmark_ptr last = NULL;

    for (i = 0; i < vec_length(doc->bookmarks); i++)
    {
        doc_bookmark_ptr mark = vec_get(doc->bookmarks, i);

        if (doc_pos_compare(mark->pos, pos) < 0)
            last = mark;
        else
            break;
    }
    if (last)
        return last->pos;
    return doc_pos_invalid();
}

doc_pos_t doc_find_bookmark(doc_ptr doc, cptr name)
{
    int i;

    for (i = 0; i < vec_length(doc->bookmarks); i++)
    {
        doc_bookmark_ptr mark = vec_get(doc->bookmarks, i);

        if (strcmp(name, string_buffer(mark->name)) == 0)
            return mark->pos;
    }
    return doc_pos_invalid();
}

static bool _line_test_str(doc_char_ptr cell, int ncell, cptr what, int nwhat)
{
    int i;

    if (ncell < nwhat)
        return FALSE;

    for (i = 0; i < nwhat; i++, cell++)
    {
        char c = cell->c ? cell->c : ' ';
        assert(i < ncell);
        if (c != what[i]) break;
    }

    if (i == nwhat)
        return TRUE;

    return FALSE;
}

static int _line_find_str_aux(doc_char_ptr cell, int ncell, cptr what, int start)
{
    int i;
    int nwhat = strlen(what);

    if (!nwhat) return -1;

    for (i = start; i <= ncell - nwhat; i++)
    {
        if (_line_test_str(cell + i, ncell - i, what, nwhat))
            return i;
    }

    return -1;
}

static int _line_find_str(doc_char_ptr cell, int ncell, cptr what)
{
    return _line_find_str_aux(cell, ncell, what, 0);
}

static bool _doc_find_next_region(doc_ptr doc, cptr text, doc_pos_t start, doc_region_ptr region)
{
    int y;
    int len = strlen(text);

    if (!len) return FALSE;
    if (start.y < 0) start.y = 0;
    if (start.y > doc->cursor.y) return FALSE;
    if (start.x < 0) start.x = 0;
    if (start.x > doc->width) start.x = doc->width;

    for (y = start.y; y <= doc->cursor.y; y++)
    {
        int          x = (y == start.y) ? start.x : 0;
        int          ncell = doc->width - x;
        doc_char_ptr cell = doc_char(doc, doc_pos_create(x, y));
        int          i = _line_find_str(cell, ncell, text);

        if (i >= 0)
        {
            region->start.x = x + i;
            region->start.y = y;
            region->stop.x = x + i + len;
            region->stop.y = y;
            return TRUE;
        }
   }

    return FALSE;
}

static bool _doc_find_prev_region(doc_ptr doc, cptr text, doc_pos_t start, doc_region_ptr region)
{
    int y;
    int len = strlen(text);

    if (!len) return FALSE;
    if (start.y > doc->cursor.y) start.y = doc->cursor.y;
    if (start.y < 0) return FALSE;
    if (start.x < 0) start.x = 0;
    if (start.x > doc->width) start.x = doc->width;

    for (y = start.y; y >= 0; y--)
    {
        int          limit = (y == start.y) ? start.x : doc->width;
        int          i = -1;
        doc_char_ptr cell = doc_char(doc, doc_pos_create(0, y));

        if (limit < len) continue;

        for (;;)
        {
            int next = _line_find_str_aux(cell, limit, text, i + 1);
            if (next < 0) break;
            i = next;
        }

        if (i >= 0)
        {
            region->start.x = i;
            region->start.y = y;
            region->stop.x = i + len;
            region->stop.y = y;
            return TRUE;
        }
   }

    return FALSE;
}

doc_pos_t doc_find_next(doc_ptr doc, cptr text, doc_pos_t start)
{
    doc_region_t region = doc_region_invalid();
    if (_doc_find_next_region(doc, text, start, &region))
    {
        doc->selection = region;
        return region.start;
    }

    doc->selection = doc_region_invalid();
    return doc_pos_invalid();
}

doc_pos_t doc_find_prev(doc_ptr doc, cptr text, doc_pos_t start)
{
    doc_region_t region = doc_region_invalid();
    if (_doc_find_prev_region(doc, text, start, &region))
    {
        doc->selection = region;
        return region.start;
    }

    doc->selection = doc_region_invalid();
    return doc_pos_invalid();
}

typedef struct
{
    char text[81];
    bool active;
    int  total;
    int  current;
    int  wrap;
} _doc_search_t, *_doc_search_ptr;

#define _DOC_SEARCH_WRAP_NONE   0
#define _DOC_SEARCH_WRAP_TOP    1
#define _DOC_SEARCH_WRAP_BOTTOM -1
#define _DOC_RAW_F3             0x86

static int _doc_search_count(doc_ptr doc, cptr text)
{
    int total = 0;
    int len = strlen(text);
    int y;

    if (!len) return 0;

    for (y = 0; y <= doc->cursor.y; y++)
    {
        int          pos = 0;
        doc_char_ptr cell = doc_char(doc, doc_pos_create(0, y));

        for (;;)
        {
            int match = _line_find_str_aux(cell, doc->width, text, pos);
            if (match < 0) break;
            total++;
            pos = match + len;
        }
    }

    return total;
}

static int _doc_search_current(doc_ptr doc, cptr text, doc_pos_t target)
{
    int total = 0;
    int len = strlen(text);
    int y;

    if (!len || !doc_pos_is_valid(target)) return 0;

    for (y = 0; y <= doc->cursor.y; y++)
    {
        int          pos = 0;
        doc_char_ptr cell = doc_char(doc, doc_pos_create(0, y));

        for (;;)
        {
            int match = _line_find_str_aux(cell, doc->width, text, pos);
            if (match < 0) break;
            total++;
            if (y == target.y && match == target.x)
                return total;
            pos = match + len;
        }
    }

    return 0;
}

static bool _doc_search_open(doc_ptr doc, rect_t display, _doc_search_ptr search)
{
    _doc_search_t old = *search;
    doc_region_t  old_selection = doc->selection;

    Term_erase(display.x, display.y + display.cy - 1, display.cx);
    put_str("Find: ", display.y + display.cy - 1, display.x);

    if (!askfor(search->text, 80))
    {
        *search = old;
        doc->selection = old_selection;
        return FALSE;
    }

    search->wrap = _DOC_SEARCH_WRAP_NONE;
    search->current = 0;
    search->active = (search->text[0] != '\0');
    search->total = search->active ? _doc_search_count(doc, search->text) : 0;
    doc->selection = doc_region_invalid();
    return TRUE;
}

static bool _doc_search_next(doc_ptr doc, _doc_search_ptr search, int *top, int page_size)
{
    doc_region_t found = doc_region_invalid();
    doc_region_t old_selection = doc->selection;
    doc_pos_t    start;

    search->wrap = _DOC_SEARCH_WRAP_NONE;
    if (!search->active || !search->text[0]) return FALSE;

    search->total = _doc_search_count(doc, search->text);
    if (!search->total)
    {
        search->current = 0;
        doc->selection = old_selection;
        return FALSE;
    }

    start = doc_pos_is_valid(old_selection.stop) ? old_selection.stop : doc_pos_create(0, *top);
    if (!_doc_find_next_region(doc, search->text, start, &found))
    {
        if (!_doc_find_next_region(doc, search->text, doc_pos_create(0, 0), &found))
        {
            doc->selection = old_selection;
            return FALSE;
        }
        search->wrap = _DOC_SEARCH_WRAP_TOP;
    }

    doc->selection = found;
    search->current = _doc_search_current(doc, search->text, found.start);

    *top = found.start.y;
    if (*top > doc->cursor.y - page_size)
        *top = MAX(0, doc->cursor.y - page_size);
    return TRUE;
}

static bool _doc_search_prev(doc_ptr doc, _doc_search_ptr search, int *top, int page_size)
{
    doc_region_t found = doc_region_invalid();
    doc_region_t old_selection = doc->selection;
    doc_pos_t    start;

    search->wrap = _DOC_SEARCH_WRAP_NONE;
    if (!search->active || !search->text[0]) return FALSE;

    search->total = _doc_search_count(doc, search->text);
    if (!search->total)
    {
        search->current = 0;
        doc->selection = old_selection;
        return FALSE;
    }

    start = doc_pos_is_valid(old_selection.start)
        ? old_selection.start
        : doc_pos_create(doc->width, MIN(doc->cursor.y, *top + page_size));

    if (!_doc_find_prev_region(doc, search->text, start, &found))
    {
        if (!_doc_find_prev_region(doc, search->text, doc_pos_create(doc->width, doc->cursor.y), &found))
        {
            doc->selection = old_selection;
            return FALSE;
        }
        search->wrap = _DOC_SEARCH_WRAP_BOTTOM;
    }

    doc->selection = found;
    search->current = _doc_search_current(doc, search->text, found.start);

    *top = found.start.y;
    if (*top > doc->cursor.y - page_size)
        *top = MAX(0, doc->cursor.y - page_size);
    return TRUE;
}

static bool _doc_cmd_is_f3(int cmd)
{
    return cmd == SKEY_F3 || cmd == _DOC_RAW_F3;
}

static bool _doc_cmd_is_shift_f3(int cmd)
{
    return cmd == (SKEY_F3 | SKEY_MOD_SHIFT);
}

static bool _doc_search_prompt_cmd(int cmd, _doc_search_ptr search)
{
    if (cmd == '/') return TRUE;
    if (cmd == '\\') return !search->active;
    if (cmd == KTRL('F')) return !search->active;
    if (_doc_cmd_is_f3(cmd) || _doc_cmd_is_shift_f3(cmd))
        return !search->active;
    return FALSE;
}

static void _doc_search_status(char *buf, int max, _doc_search_ptr search)
{
    if (!search->active || !search->text[0])
    {
        buf[0] = '\0';
        return;
    }

    if (!search->total)
        snprintf(buf, max, "[Find: %s (0/0 matches; no matches)]", search->text);
    else if (search->wrap == _DOC_SEARCH_WRAP_TOP)
        snprintf(buf, max, "[Find: %s (%d/%d matches; wrapped to top)]", search->text, search->current, search->total);
    else if (search->wrap == _DOC_SEARCH_WRAP_BOTTOM)
        snprintf(buf, max, "[Find: %s (%d/%d matches; wrapped to bottom)]", search->text, search->current, search->total);
    else
        snprintf(buf, max, "[Find: %s (%d/%d matches)]", search->text, search->current, search->total);
}

void doc_push_style(doc_ptr doc, doc_style_ptr style)
{
    doc_style_ptr copy = malloc(sizeof(doc_style_t));

    *copy = *style;
    if (copy->right > doc->width)
        copy->right = doc->width;

    vec_push(doc->style_stack, copy);
    if (doc->cursor.x < copy->left)
    {
        doc_insert_space(doc, copy->left - doc->cursor.x);
        assert(doc->cursor.x == copy->left);
    }
}

void doc_push_named_style(doc_ptr doc, cptr name)
{
    doc_style_t copy = *doc_current_style(doc);
    doc_style_f f = _get_doc_style_f(doc, name);

    if (f)
        f(&copy);

    doc_push_style(doc, &copy);
}

void doc_pop_style(doc_ptr doc)
{
    assert(vec_length(doc->style_stack) > 0);

    /* the style stack can never empty, and the bottom is always "normal" */
    if (vec_length(doc->style_stack) > 1)
    {
        doc_style_ptr style = vec_pop(doc->style_stack);
        free(style);
    }
}

doc_style_ptr doc_current_style(doc_ptr doc)
{
    int           ct = vec_length(doc->style_stack);
    doc_style_ptr style = NULL;

    if (ct > 0)
        style = vec_get(doc->style_stack, ct - 1);

    assert(style);
    return style;
}

doc_pos_t doc_newline(doc_ptr doc)
{
    doc_style_ptr style = doc_current_style(doc);

    doc->cursor.y++;
    doc->cursor.x = 0;
    if (doc->cursor.x < style->left)
    {
        doc_insert_space(doc, style->left);
        assert(doc->cursor.x == style->left);
    }
    return doc->cursor;
}

cptr doc_parse_tag(cptr pos, doc_tag_ptr tag)
{
    /* prepare to fail! */
    tag->type = DOC_TAG_NONE;
    tag->arg = NULL;
    tag->arg_size = 0;

    /* <name:arg> where name in {"color", "style", "topic", "link"} */
    if (*pos == '<')
    {
        doc_tag_t result = {0};
        cptr seek = pos + 1;
        char name[MAX_NLEN];
        int  ct = 0;
        for (;;)
        {
            if (!*seek || strchr(" <\r\n\t", *seek)) return pos;
            if (*seek == ':' || *seek == '>') break;
            name[ct++] = *seek;
            if (ct >= MAX_NLEN) return pos;
            seek++;
        }
        name[ct] = '\0';

        /* [pos,seek) is the name of the tag */
        if (strcmp(name, "color") == 0)
            result.type = DOC_TAG_COLOR;
        else if (strcmp(name, "/color") == 0)
            result.type = DOC_TAG_CLOSE_COLOR;
        else if (strcmp(name, "style") == 0)
            result.type = DOC_TAG_STYLE;
        else if (strcmp(name, "/style") == 0)
            result.type = DOC_TAG_CLOSE_STYLE;
        else if (strcmp(name, "topic") == 0)
            result.type = DOC_TAG_TOPIC;
        else if (strcmp(name, "link") == 0)
            result.type = DOC_TAG_LINK;
        else if (strcmp(name, "$") == 0 || strcmp(name, "var") == 0)
            result.type = DOC_TAG_VAR;
        else if (strcmp(name, "indent") == 0)
            result.type = DOC_TAG_INDENT;
        else if (strcmp(name, "/indent") == 0)
            result.type = DOC_TAG_CLOSE_INDENT;
        else if (strcmp(name, "tab") == 0)
            result.type = DOC_TAG_TAB;
        else
            return pos;

        if (*seek == '>')
        {
            switch(result.type)
            {
            case DOC_TAG_CLOSE_COLOR:
            case DOC_TAG_CLOSE_STYLE:
            case DOC_TAG_INDENT:
            case DOC_TAG_CLOSE_INDENT:
                seek++;
                *tag = result;
                return seek;
            default:
                return pos;
            }
        }

        assert(*seek == ':');
        seek++;
        result.arg = seek;

        ct = 0;
        for (;;)
        {
            if (!*seek || strchr("<\r\n\t", *seek)) return pos;
            if (*seek == '>') break;
            ct++;
            seek++;
        }
        result.arg_size = ct;

        assert(*seek == '>');
        seek++;

        if (!result.arg_size)
            return pos;
        else if (result.type == DOC_TAG_COLOR)
        {
            if ( result.arg_size == 1 && result.arg[0] != '*'
              && !strchr(color_char, result.arg[0]) )
            {
                return pos;
            }
        }

        *tag = result;
        return seek;
    }

    return pos;
}

cptr doc_lex(cptr pos, doc_token_ptr token)
{
    if (!*pos)
    {
        token->type = DOC_TOKEN_EOF;
        token->pos = pos;
        token->size = 0;
        return pos;
    }
    if (*pos == '\r')
        pos++;

    if (*pos == '\n')
    {
        token->type = DOC_TOKEN_NEWLINE;
        token->pos = pos++;
        token->size = 1;
        return pos;
    }
    if (*pos == ' ' || *pos == '\t')
    {
        token->type = DOC_TOKEN_WHITESPACE;
        token->pos = pos++;
        token->size = 1;
        while (*pos && (*pos == ' ' || *pos == '\t'))
        {
            token->size++;
            pos++;
        }
        return pos;
    }
    if (*pos == '<')
    {
        token->type = DOC_TOKEN_TAG;
        token->pos = pos;
        token->size = 0;
        pos = doc_parse_tag(pos, &token->tag);
        if (token->tag.type != DOC_TAG_NONE)
        {
            token->size = pos - token->pos;
            return pos;
        }
    }
    token->type = DOC_TOKEN_WORD;
    token->pos = pos++;
    token->size = 1;

    while (*pos && !strchr(" <\n", *pos))
    {
        token->size++;
        pos++;
    }
    return pos;
}

static void _doc_process_var(doc_ptr doc, cptr name)
{
    if (strcmp(name, "version") == 0)
    {
        string_ptr s = string_alloc_format("%d.%d.%d", VER_MAJOR, VER_MINOR, VER_EXTRA);
        if (coffee_break == SPEED_COFFEE) string_append_s(s, "<color:U> (Coffee)</color>");
        if (coffee_break == SPEED_INSTA_COFFEE) string_append_s(s, "<color:U> (Instant Coffee)</color>");
        if (thrall_mode) string_append_s(s, "<color:R> (Thrall)</color>");
        if (wacky_rooms) string_append_s(s, "<color:v> (Wacky)</color>");
        if (VERSION_IS_DEVELOPMENT)
        {
            if ((coffee_break) && (wacky_rooms) && (thrall_mode)) string_append_s(s, "<color:B> (Dev)</color>");
            else string_append_s(s, "<color:B> (Development)</color>");
        }
        if ((VER_MINOR == 0) && (VER_MAJOR != 7)) string_append_s(s, "<color:r> (Beta)</color>");
        if (arg_webclient) string_append_s(s, " for angband.live");
        doc_insert(doc, string_buffer(s));
        string_free(s);
    }
    else if (strlen(name) > 3 && strncmp(name, "FF_", 3) == 0)
    {
        char buf[100];
        int  f_idx;

        sprintf(buf, "%s", name + 3);
        f_idx = f_tag_to_index(buf);

        if (0 <= f_idx && f_idx < max_f_idx)
        {
            string_ptr s = string_alloc();
            feature_type *feat = &f_info[f_idx];

            string_printf(s, "<color:%c>%c</color>",
                attr_to_attr_char(feat->d_attr[F_LIT_STANDARD]),
                feat->d_char[F_LIT_STANDARD]);

            doc_insert(doc, string_buffer(s));
            string_free(s);
        }
    }
}

static void _doc_process_tag(doc_ptr doc, doc_tag_ptr tag)
{
    if ( tag->type == DOC_TAG_CLOSE_COLOR
      || tag->type == DOC_TAG_CLOSE_STYLE
      || tag->type == DOC_TAG_CLOSE_INDENT )
    {
        doc_pop_style(doc);
    }
    else if (tag->type == DOC_TAG_COLOR)
    {
        assert(tag->arg);
        if (tag->arg_size == 1)
        {
            if (tag->arg[0] == '*')
                doc_pop_style(doc);
            else
            {
                doc_style_t style = *doc_current_style(doc); /* copy */
                byte new_attr = color_char_to_attr(tag->arg[0]);
                if (new_attr != _INVALID_COLOR) style.color = new_attr;
                doc_push_style(doc, &style);
            }
        }
        else
        {
            string_ptr  arg = string_copy_sn(tag->arg, tag->arg_size);
            doc_style_t style = *doc_current_style(doc); /* copy */
            doc_style_f f = _get_doc_style_f(doc, string_buffer(arg));

            if (f)
                f(&style);

            {/* We don't copy the named style, just its color. */
             /* Also, we damn well better push a style or we'll be upset
                when the good little user pops! */
                doc_style_t copy = *doc_current_style(doc);
                copy.color = style.color;
                doc_push_style(doc, &copy);
            }
            string_free(arg);
        }
    }
    else if (tag->type == DOC_TAG_INDENT)
    {
        doc_style_t style = *doc_current_style(doc);
        style.left = doc->cursor.x;
        doc_push_style(doc, &style);
    }
    else
    {
        string_ptr arg = string_copy_sn(tag->arg, tag->arg_size);

        switch (tag->type)
        {
        case DOC_TAG_STYLE:
            if (tag->arg_size == 1 && tag->arg[0] == '*')
                doc_pop_style(doc);
            else
            {/* Better silently add one if name doesn't exist ... */
                doc_style_t copy = *doc_current_style(doc);
                doc_style_f f = _get_doc_style_f(doc, string_buffer(arg));

                if (f)
                    f(&copy);
                doc_push_style(doc, &copy);
            }
            break;
        case DOC_TAG_VAR:
            _doc_process_var(doc, string_buffer(arg));
            break;
        case DOC_TAG_TAB:
        {
            int pos = atoi(string_buffer(arg)) + doc_current_style(doc)->left;
            if (pos > doc->cursor.x)
                doc_insert_space(doc, pos - doc->cursor.x);
            else
                doc_rollback(doc, doc_pos_create(pos, doc->cursor.y));
            break;
        }
        case DOC_TAG_TOPIC:
        {
            doc_bookmark_ptr mark = malloc(sizeof(doc_bookmark_t));
            mark->name = arg; /* steal ownership */
            arg = NULL;
            mark->pos = doc->cursor;
            vec_add(doc->bookmarks, mark);
            break;
        }
        case DOC_TAG_LINK:
        {
            doc_link_ptr link = malloc(sizeof(doc_link_t));
            int          split = string_chr(arg, 0, '#');
            int          ch = 'a' + int_map_count(doc->links);

            if (split >= 0)
            {
                substring_t left = string_left(arg, split);
                substring_t right = string_right(arg, string_length(arg) - split - 1);

                link->file = substring_copy(&left);
                link->topic = substring_copy(&right);
            }
            else
            {
                link->file = arg; /* steal ownership */
                arg = NULL;
                link->topic = NULL;
            }
            link->location.start = doc->cursor;
            int_map_add(doc->links, ch, link);
            {
                /* TODO: This is flawed. Here's a real world example:
                   "(see below <link:birth.txt#PrimaryStats>)."
                   Can you see the problem? We might line break after "[a]" right
                   before ").". Instead, "[a])." should be treated as the current
                   word. To fix this, we'll need a parser with a token queue
                   that we can push onto, but this raises storage issues.
                */
                string_ptr s = string_alloc_format("<style:link>[%c]</style>", ch);
                doc_insert(doc, string_buffer(s));
                string_free(s);

                link->location.stop = doc->cursor;
            }
            break;
        }
        }

        string_free(arg);
    }
}

doc_pos_t doc_insert(doc_ptr doc, cptr text)
{
    doc_token_t token;
    doc_token_t queue[10];
    int         qidx = 0;
    cptr        pos = text;
    int         i, j, cb;
    bool        nowrap = FALSE;
    doc_style_ptr style = NULL;

    for (;;)
    {
        pos = doc_lex(pos, &token);

        if (token.type == DOC_TOKEN_EOF)
            break;

        assert(pos != token.pos);

        if (token.type == DOC_TOKEN_TAG)
        {
            _doc_process_tag(doc, &token.tag);
            continue;
        }

        if (token.type == DOC_TOKEN_NEWLINE)
        {
            doc_newline(doc);
            nowrap = FALSE;
            continue;
        }

        if (token.type == DOC_TOKEN_WHITESPACE)
        {
            doc_insert_space(doc, token.size);
            continue;
        }

        assert(token.type == DOC_TOKEN_WORD);
        assert(token.size > 0);

        /* Queue Complexity is for "<color:R>difficult</color>!" which is actually a bit common! */
        qidx = 0;
        cb = token.size;
        queue[qidx++] = token;
        while (qidx < 10)
        {
            cptr peek = doc_lex(pos, &token);
            if (token.type == DOC_TOKEN_WORD)
            {
                cb += token.size;
            }
            else if ( token.type == DOC_TOKEN_TAG
                   && (token.tag.type == DOC_TAG_COLOR || token.tag.type == DOC_TAG_CLOSE_COLOR) )
            {
            }
            else /* whitespace or newline */
            {
                break;
            }
            queue[qidx++] = token;
            pos = peek;
        }

        style = doc_current_style(doc); /* be careful ... this changes on <color:_> tags! */
        if ( doc->cursor.x + cb >= style->right
          && !(style->options & DOC_STYLE_NO_WORDWRAP) )
        {
            doc_newline(doc);
            if (style->indent)
                doc_insert_space(doc, style->indent);
        }

        for (i = 0; i < qidx; i++)
        {
            doc_token_ptr current = &queue[i];

            if (current->type == DOC_TOKEN_TAG)
            {
                assert(current->tag.type == DOC_TAG_COLOR || current->tag.type == DOC_TAG_CLOSE_COLOR);
                _doc_process_tag(doc, &current->tag);
                style = doc_current_style(doc);
            }
            else if (doc->cursor.x < style->right)
            {
                doc_char_ptr cell = doc_char(doc, doc->cursor);

                assert(current->type == DOC_TOKEN_WORD);
                assert(cell);

                for (j = 0; j < current->size && !nowrap; j++)
                {
                    cell->a = style->color;
                    cell->c = current->pos[j];
                    if (cell->c == '\t')
                        cell->c = ' ';
                    if (doc->cursor.x == style->right - 1)
                    {
                        if (style->options & DOC_STYLE_NO_WORDWRAP)
                        {
                            /* nowrap is tricky ... we still need to process remaining tokens
                             * since these may change the current style. however, we need to stop
                             * trying to print, and we'll guard this for word tokens with the
                             * following flag: */
                            nowrap = TRUE;
                            break;
                        }
                        doc_newline(doc);
                        if (style->indent)
                            doc_insert_space(doc, style->indent);
                        cell = doc_char(doc, doc->cursor);
                    }
                    else
                    {
                        doc->cursor.x++;
                        cell++;
                    }
                }
            }
        }
    }
    assert(0 <= doc->cursor.x && doc->cursor.x < doc->width);
    return doc->cursor;
}

doc_pos_t doc_insert_char(doc_ptr doc, byte a, char c)
{
    doc_char_ptr cell = doc_char(doc, doc->cursor);
    doc_style_ptr style = doc_current_style(doc);

    cell->a = a;
    cell->c = c;

    if (doc->cursor.x >= style->right - 1)
    {
        if (!(style->options & DOC_STYLE_NO_WORDWRAP))
            doc_newline(doc);
    }
    else
        doc->cursor.x++;

    assert(0 <= doc->cursor.x && doc->cursor.x < doc->width);
    return doc->cursor;
}

doc_pos_t doc_insert_space(doc_ptr doc, int count)
{
    int          i;
    doc_char_ptr cell = doc_char(doc, doc->cursor);
    doc_style_ptr style = doc_current_style(doc);

    for (i = 0; i < count; i++)
    {
        cell->a = style->color;
        cell->c = ' ';
        if (doc->cursor.x >= style->right - 1)
            break;
        doc->cursor.x++;
        cell++;
    }
    assert(0 <= doc->cursor.x && doc->cursor.x < doc->width);
    return doc->cursor;
}

doc_pos_t doc_insert_text(doc_ptr doc, byte a, cptr text)
{
    doc_style_ptr style = doc_current_style(doc);
    if (style->color != a)
    {
        doc_style_t copy = *style;
        copy.color = a;
        doc_push_style(doc, &copy);
        doc_insert(doc, text);
        doc_pop_style(doc);
    }
    else
        doc_insert(doc, text);

    assert(0 <= doc->cursor.x && doc->cursor.x < doc->width);
    return doc->cursor;
}

doc_pos_t doc_printf(doc_ptr doc, const char *fmt, ...)
{
    string_ptr s = string_alloc();
    va_list vp;

    va_start(vp, fmt);
    string_vprintf(s, fmt, vp);
    va_end(vp);

    doc_insert(doc, string_buffer(s));
    string_free(s);
    return doc->cursor;
}

doc_pos_t doc_cprintf(doc_ptr doc, byte a, const char *fmt, ...)
{
    string_ptr s = string_alloc();
    va_list vp;

    va_start(vp, fmt);
    string_vprintf(s, fmt, vp);
    va_end(vp);

    doc_insert_text(doc, a, string_buffer(s));
    string_free(s);
    return doc->cursor;
}

doc_pos_t doc_insert_doc(doc_ptr dest_doc, doc_ptr src_doc, int indent)
{
    doc_pos_t src_pos = doc_pos_create(0, 0);
    doc_pos_t dest_pos;

    if (dest_doc->cursor.x > 0)
        doc_newline(dest_doc);

    dest_pos = dest_doc->cursor;

    while (src_pos.y <= src_doc->cursor.y)
    {
        doc_char_ptr src = doc_char(src_doc, src_pos);
        doc_char_ptr dest;
        int          count = src_doc->width;
        int          i;

        dest_pos.x += indent;
        dest = doc_char(dest_doc, dest_pos);

        if (count > dest_doc->width - dest_pos.x)
            count = dest_doc->width - dest_pos.x;
        if (src_pos.y == src_doc->cursor.y && count > src_doc->cursor.x)
            count = src_doc->cursor.x;

        for (i = 0; i < count; i++)
        {
            dest->a = src->a;
            dest->c = src->c;

            dest++;
            src++;
        }

        dest_pos.x = 0;
        dest_pos.y++;

        src_pos.x = 0;
        src_pos.y++;
    }

    dest_doc->cursor = dest_pos;
    return dest_doc->cursor;
}

doc_pos_t doc_insert_cols(doc_ptr dest_doc, doc_ptr src_cols[], int col_count, int spacing)
{
    int       src_y = 0;
    int       max_src_y = 0;
    doc_pos_t dest_pos;
    int       i;

    if (dest_doc->cursor.x > 0)
        doc_newline(dest_doc);

    dest_pos = dest_doc->cursor;

    for (i = 0; i < col_count; i++)
    {
        doc_ptr src_col = src_cols[i];
        max_src_y = MAX(src_col->cursor.y, max_src_y);
    }

    while (src_y <= max_src_y)
    {
        for (i = 0; i < col_count; i++)
        {
            doc_ptr src_col = src_cols[i];
            int     count = src_col->width;

            if (count > dest_doc->width - dest_pos.x)
                count = dest_doc->width - dest_pos.x;

            if (src_y <= src_col->cursor.y && count > 0)
            {
                doc_char_ptr dest = doc_char(dest_doc, dest_pos);
                doc_char_ptr src = doc_char(src_col, doc_pos_create(0, src_y));
                int          j;

                for (j = 0; j < count; j++)
                {
                    /* Hack: Attempt to prevent trailing spaces. For some reason,
                       this is causing oook to not display character dumps properly
                       from Windows builds. Interestingly, oook displays files correctly
                       from Linux even without this hack, which is puzzling since
                       the files seem identical on both platforms.*/
                    if (i == col_count - 1 && !src->c)
                    {
                        count = j;
                        break;
                    }

                    dest->a = src->a;
                    dest->c = src->c;

                    if (!dest->c)
                        dest->c = ' ';

                    dest++;
                    src++;
                }
            }
            dest_pos.x += count;

            if (i == col_count - 1)
                break;

            /* Spacing between columns */
            count = spacing;
            if (count > dest_doc->width - dest_pos.x)
                count = dest_doc->width - dest_pos.x;

            if (count > 0)
            {
                doc_char_ptr dest = doc_char(dest_doc, dest_pos);
                int          j;
                for (j = 0; j < count; j++)
                {
                    dest->a = TERM_WHITE;
                    dest->c = ' ';
                    dest++;
                }
            }
            dest_pos.x += count;
        }

        dest_pos.x = 0;
        dest_pos.y++;

        src_y++;
    }

    dest_doc->cursor = dest_pos;
    return dest_doc->cursor;
}

doc_char_ptr doc_char(doc_ptr doc, doc_pos_t pos)
{
    int            cb = doc->width * PAGE_HEIGHT * sizeof(doc_char_t);
    int            page_num = PAGE_NUM(pos.y);
    int            offset = PAGE_OFFSET(pos.y);
    doc_char_ptr   page = NULL;

    while (page_num >= vec_length(doc->pages))
    {
        page = malloc(cb);
        memset(page, 0, cb);
        vec_add(doc->pages, page);
    }

    page = vec_get(doc->pages, page_num);

    assert(0 <= doc->cursor.x && doc->cursor.x < doc->width);
    assert(offset * doc->width + pos.x < cb);

    return page + offset * doc->width + pos.x;
}

doc_pos_t doc_read_file(doc_ptr doc, FILE *fp)
{
    string_ptr s = string_read_file(fp);
    doc_insert(doc, string_buffer(s));
    string_free(s);
    return doc->cursor;
}

static void _doc_write_text_file(doc_ptr doc, FILE *fp)
{
    doc_pos_t    pos;
    doc_char_ptr cell;

    for (pos.y = 0; pos.y <= doc->cursor.y; pos.y++)
    {
        int cx = doc->width;
        pos.x = 0;
        if (pos.y == doc->cursor.y)
            cx = doc->cursor.x;
        cell = doc_char(doc, pos);
        for (; pos.x < cx; pos.x++)
        {
            if (!cell->c) break;
            fputc(cell->c, fp);
            cell++;
        }
        fprintf(fp, "\n");
        /*fputc('\n', fp);*/
   }
}

static int _compare_links(doc_link_ptr left, doc_link_ptr right)
{
    return doc_pos_compare(left->location.start, right->location.start);
}

static string_ptr _doc_bookmark_id(doc_bookmark_ptr mark)
{
    string_ptr id = string_alloc();
    cptr name = string_buffer(mark->name);
    bool pending_dash = FALSE;
    int i;

    for (i = 0; name[i]; i++)
    {
        unsigned char c = (unsigned char)name[i];

        if (isalnum(c))
        {
            if (pending_dash && string_length(id))
                string_append_c(id, '-');
            string_append_c(id, (char)tolower(c));
            pending_dash = FALSE;
        }
        else if (string_length(id))
            pending_dash = TRUE;
    }

    if (!string_length(id))
        string_append_s(id, "section");

    return id;
}

vec_ptr doc_get_links(doc_ptr doc)
{
    vec_ptr          links = vec_alloc(NULL);
    int_map_iter_ptr iter;

    for (iter = int_map_iter_alloc(doc->links);
            int_map_iter_is_valid(iter);
            int_map_iter_next(iter) )
    {
        doc_link_ptr link = int_map_iter_current(iter);
        vec_add(links, link);
    }
    int_map_iter_free(iter);
    vec_sort(links, (vec_cmp_f)_compare_links);
    return links;
}

static void _doc_write_html_file(doc_ptr doc, FILE *fp)
{
    doc_pos_t        pos;
    doc_char_ptr     cell;
    byte             old_a = _INVALID_COLOR;
    int              bookmark_idx = 0;
    doc_bookmark_ptr next_bookmark = NULL;
    vec_ptr          links = doc_get_links(doc);
    int              link_idx = 0;
    doc_link_ptr     next_link = NULL;

    if (bookmark_idx < vec_length(doc->bookmarks))
        next_bookmark = vec_get(doc->bookmarks, bookmark_idx);

    if (link_idx < vec_length(links))
        next_link = vec_get(links, link_idx);

    fprintf(fp, "<!DOCTYPE html>\n<html>\n");
    if (string_length(doc->html_header))
        fprintf(fp, "%s\n", string_buffer(doc->html_header));
    fprintf(fp, "<body text=\"#ffffff\" bgcolor=\"#000000\"><pre>\n");

    for (pos.y = 0; pos.y <= doc->cursor.y; pos.y++)
    {
        int cx = doc->width;
        pos.x = 0;
        if (pos.y == doc->cursor.y)
            cx = doc->cursor.x;
        cell = doc_char(doc, pos);

        if (next_bookmark && pos.y == next_bookmark->pos.y)
        {
            string_ptr id = _doc_bookmark_id(next_bookmark);
            fprintf(fp, "<a name=\"%s\" id=\"%s\"></a>", string_buffer(next_bookmark->name), string_buffer(id));
            string_free(id);
            bookmark_idx++;
            if (bookmark_idx < vec_length(doc->bookmarks))
                next_bookmark = vec_get(doc->bookmarks, bookmark_idx);
            else
                next_bookmark = NULL;
        }

        for (; pos.x < cx; pos.x++)
        {
            char c = cell->c;
            byte a = cell->a % MAX_COLOR;

            if (next_link)
            {
                if (doc_pos_compare(next_link->location.start, pos) == 0)
                {
                    string_ptr s;
                    int        pos = string_last_chr(next_link->file, '.');

                    if (pos >= 0)
                    {
                        s = string_copy_sn(string_buffer(next_link->file), pos + 1);
                        string_append_s(s, "html");
                    }
                    else
                        s = string_copy(next_link->file);

                    fprintf(fp, "<a href=\"%s", string_buffer(s));
                    if (next_link->topic)
                        fprintf(fp, "#%s", string_buffer(next_link->topic));
                    fprintf(fp, "\">");

                    string_free(s);
                }
                if (doc_pos_compare(next_link->location.stop, pos) == 0)
                {
                    fprintf(fp, "</a>");
                    link_idx++;
                    if (link_idx < vec_length(links))
                        next_link = vec_get(links, link_idx);
                    else
                        next_link = NULL;
                }
            }

            if (!c) break;

            if (a != old_a && c != ' ')
            {
                if (old_a != _INVALID_COLOR)
                    fprintf(fp, "</font>");
                fprintf(fp,
                    "<font color=\"#%02x%02x%02x\">",
                    angband_color_table[a][1],
                    angband_color_table[a][2],
                    angband_color_table[a][3]
                );
                old_a = a;
            }
            switch (c)
            {
            case '&': fprintf(fp, "&amp;"); break;
            case '<': fprintf(fp, "&lt;"); break;
            case '>': fprintf(fp, "&gt;"); break;
            default:  fprintf(fp, "%c", c); break;
            }
            cell++;
        }
        fputc('\n', fp);
   }
   fprintf(fp, "</font>");
   fprintf(fp, "</pre>");
   if (string_length(doc->html_footer))
       fprintf(fp, "%s", string_buffer(doc->html_footer));
   fprintf(fp, "</body></html>\n");

   vec_free(links);
}

static void _doc_write_doc_file(doc_ptr doc, FILE *fp)
{
    doc_pos_t        pos;
    doc_char_ptr     cell;
    byte             old_a = _INVALID_COLOR;
    int              bookmark_idx = 0;
    int              cx;

    fputs("<style:wide>", fp);
    for (pos.y = 0; pos.y <= doc->cursor.y; pos.y++)
    {
        while (bookmark_idx < vec_length(doc->bookmarks))
        {
            doc_bookmark_ptr mark = vec_get(doc->bookmarks, bookmark_idx);
            if (mark->pos.y != pos.y) break;
            fprintf(fp, "<topic:%s>", string_buffer(mark->name));
            bookmark_idx++;
        }

        cx = doc->width;
        pos.x = 0;
        if (pos.y == doc->cursor.y)
            cx = doc->cursor.x;
        cell = doc_char(doc, pos);

        for (; pos.x < cx; pos.x++)
        {
            char c = cell->c;
            byte a = cell->a;

            if (!c) break;

            if (a != old_a && c != ' ')
            {
                if (old_a != _INVALID_COLOR)
                    fputs("</color>", fp);
                fprintf(fp, "<color:%c>", attr_to_attr_char(a));
                old_a = a;
            }
            fputc(c, fp);
            cell++;
        }
        fputc('\n', fp);
   }
    fputs("</style>", fp);
}

void doc_write_file(doc_ptr doc, FILE *fp, int format)
{
    switch (format)
    {
    case DOC_FORMAT_HTML:
        _doc_write_html_file(doc, fp);
        break;
    case DOC_FORMAT_DOC:
        _doc_write_doc_file(doc, fp);
        break;
    default:
        _doc_write_text_file(doc, fp);
    }
}

typedef void (*_doc_char_fn)(doc_pos_t pos, doc_char_ptr cell);
static void _doc_for_each(doc_ptr doc, doc_region_t range, _doc_char_fn f)
{
    doc_pos_t pos;

    assert(doc_region_is_valid(&range));

    for (pos.y = range.start.y; pos.y <= range.stop.y; pos.y++)
    {
        int max_x = doc->width;

        pos.x = 0;
        if (pos.y == range.start.y)
            pos.x = range.start.x;
        if (pos.y == range.stop.y)
            max_x = range.stop.x;

        if (pos.y <= doc->cursor.y)
        {
            doc_char_ptr cell = doc_char(doc, pos);
            for (; pos.x < max_x; pos.x++, cell++)
            {
                f(pos, cell);
            }
        }
    }
}
static void _doc_clear_char(doc_pos_t pos, doc_char_ptr cell)
{
    cell->c = '\0';
    cell->a = TERM_DARK;
}
void doc_rollback(doc_ptr doc, doc_pos_t pos)
{
    doc_region_t r;
    r.start = pos;
    r.stop = doc->cursor;
    _doc_for_each(doc, r, _doc_clear_char);
    doc->cursor = pos;
}

void doc_clear(doc_ptr doc)
{
    doc_rollback(doc, doc_pos_create(0, 0));
}

void doc_sync_menu(doc_ptr doc)
{
    rect_t mr = ui_doc_menu_rect();
    rect_t dr = mr;

    dr.cx = doc_width(doc);
    dr.cy = doc_cursor(doc).y + 1;
    /* Try to draw a shadow */
    if (dr.cy >= mr.cy - 1)
        doc_sync_term(doc, doc_range_top_lines(doc, mr.cy), doc_pos_create(mr.x, mr.y));
    else
    {
        rect_t sr = dr;
        if (sr.cx < mr.cx) sr.cx++;
        sr.cy = dr.cy + 1;
        Term_clear_rect(sr);
        doc_sync_term(doc, doc_range_all(doc), doc_pos_create(dr.x, dr.y));
    }

}

void doc_sync_term(doc_ptr doc, doc_region_t range, doc_pos_t term_pos)
{
    doc_pos_t pos;
    byte      selection_color = _INVALID_COLOR;

    assert(doc_region_is_valid(&range));

    if (doc_region_is_valid(&doc->selection))
    {
        doc_style_t style = *doc_current_style(doc);
        doc_style_f f = _get_doc_style_f(doc, "selection");
        if (f)
        {
            f(&style);
            selection_color = style.color;
        }
    }

    for (pos.y = range.start.y; pos.y <= range.stop.y; pos.y++)
    {
        int term_y = term_pos.y + (pos.y - range.start.y);
        int max_x = doc->width;

        pos.x = 0;
        if (pos.y == range.start.y)
            pos.x = range.start.x;
        if (pos.y == range.stop.y)
            max_x = range.stop.x;

        if (max_x > 0)
            Term_erase(term_pos.x + pos.x, term_y, doc->width - pos.x);
        if (pos.y <= doc->cursor.y)
        {
            doc_char_ptr cell = doc_char(doc, pos);
            for (; pos.x < max_x; pos.x++, cell++)
            {
                if (cell->c)
                {
                    int term_x = term_pos.x + pos.x;
                    byte         a = cell->a;

                    if ( selection_color != _INVALID_COLOR
                      && doc_region_contains(&doc->selection, pos) )
                    {
                        a = selection_color;
                    }
                    Term_putch(term_x, term_y, a, cell->c);
                }
            }
        }
    }
}

#define _UNWIND 1
#define _OK 0
int doc_display(doc_ptr doc, cptr caption, int top)
{
    rect_t display = {0};
    Term_get_size(&display.cx, &display.cy);
    return doc_display_aux(doc, caption, top, display);
}

typedef struct
{
    int     slot;
    time_t  saved_at;
    s32b    saved_game_turn;
    doc_ptr doc;
} _cs_slot_t;

static string_ptr _doc_write_doc_string(doc_ptr doc)
{
    doc_pos_t    pos;
    doc_char_ptr cell;
    byte         old_a = _INVALID_COLOR;
    int          bookmark_idx = 0;
    string_ptr   result = string_alloc();

    string_append_s(result, "<style:wide>");
    for (pos.y = 0; pos.y <= doc->cursor.y; pos.y++)
    {
        while (bookmark_idx < vec_length(doc->bookmarks))
        {
            doc_bookmark_ptr mark = vec_get(doc->bookmarks, bookmark_idx);
            if (mark->pos.y != pos.y) break;
            string_printf(result, "<topic:%s>", string_buffer(mark->name));
            bookmark_idx++;
        }

        {
            int cx = doc->width;
            pos.x = 0;
            if (pos.y == doc->cursor.y)
                cx = doc->cursor.x;
            cell = doc_char(doc, pos);

            for (; pos.x < cx; pos.x++)
            {
                char c = cell->c;
                byte a = cell->a;

                if (!c) break;

                if (a != old_a && c != ' ')
                {
                    if (old_a != _INVALID_COLOR)
                        string_append_s(result, "</color>");
                    string_printf(result, "<color:%c>", attr_to_attr_char(a));
                    old_a = a;
                }
                string_append_c(result, c);
                cell++;
            }
        }
        string_append_c(result, '\n');
    }
    if (old_a != _INVALID_COLOR)
        string_append_s(result, "</color>");
    string_append_s(result, "</style>");
    return result;
}

static doc_ptr _doc_dup(doc_ptr doc)
{
    doc_ptr     copy = doc_alloc(doc->width);
    string_ptr  serialized = _doc_write_doc_string(doc);

    doc_insert(copy, string_buffer(serialized));
    doc_change_name(copy, string_buffer(doc->name));
    doc_change_html_header(copy, string_buffer(doc->html_header));
    doc_change_html_footer(copy, string_buffer(doc->html_footer));

    string_free(serialized);
    return copy;
}

static void _doc_clear_selection(doc_ptr doc)
{
    doc->selection = doc_region_invalid();
}

static cptr _cs_signature(void)
{
    return "FROX-CSNAP 1";
}

static int _cs_slot_keys[] = {0, 7, 8, 9};

static int _cs_slot_index(int slot)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (_cs_slot_keys[i] == slot)
            return i;
    }
    return -1;
}

static cptr _cs_slot_name(int slot)
{
    switch (slot)
    {
    case 0: return "slot0";
    case 7: return "slot7";
    case 8: return "slot8";
    case 9: return "slot9";
    default: return "slot?";
    }
}

static cptr _cs_slot_display_name(int slot)
{
    switch (slot)
    {
    case 0: return "default";
    case 7: return "slot 7";
    case 8: return "slot 8";
    case 9: return "slot 9";
    default: return "slot?";
    }
}

static int _cs_slot_by_name(cptr name)
{
    if (streq(name, "slot0")) return 0;
    if (streq(name, "slot7")) return 7;
    if (streq(name, "slot8")) return 8;
    if (streq(name, "slot9")) return 9;
    return -1;
}

static void _cs_build_path(char *buf, int size)
{
    char name[64];
    strnfmt(name, sizeof(name), "%s.snapshots", player_base);
    path_build(buf, size, ANGBAND_DIR_USER, name);
}

static bool _replace_file(cptr temp, cptr path)
{
    char from[1024];
    char to[1024];

    if (path_parse(from, sizeof(from), temp)) return FALSE;
    if (path_parse(to, sizeof(to), path)) return FALSE;

#ifdef WINDOWS
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(from, to) == 0;
#endif
}

static void _cs_format_saved_stamp(char *buf, int size, time_t when)
{
    time_t now = when;
    struct tm *tm = localtime(&now);

    if (tm && strftime(buf, size, "@ %H:%M %d/%m/%y", tm)) return;
    my_strcpy(buf, "@ unknown time", size);
}

static bool _cs_format_turn_delta(char *buf, int size, s32b saved_turn)
{
    long delta;

    if (saved_turn < 0 || game_turn < saved_turn)
        return FALSE;

    delta = MAX(0, game_turn - saved_turn);

    if (delta < 10000)
        strnfmt(buf, size, "%ld turns", delta);
    else
        strnfmt(buf, size, "%ldk turns", delta / 1000);
    return TRUE;
}

static void _cs_format_compact_minutes(char *buf, int size, long minutes)
{
    long days;
    long hours;

    minutes = MAX(1, minutes);
    if (minutes < 60)
    {
        strnfmt(buf, size, "%ldm", minutes);
        return;
    }

    if (minutes < 24L * 60L)
    {
        hours = minutes / 60L;
        minutes %= 60L;
        strnfmt(buf, size, "%ldh%ldm", hours, minutes);
        return;
    }

    days = minutes / (24L * 60L);
    hours = (minutes % (24L * 60L)) / 60L;
    if (days < 100L)
        strnfmt(buf, size, "%ldd%ldh", days, hours);
    else
        strnfmt(buf, size, "%ldd", days);
}

static bool _cs_format_saved_delta(char *buf, int size, time_t when)
{
    time_t now = time(NULL);

    if (!when || now < when)
    {
        _cs_format_saved_stamp(buf, size, when);
        return FALSE;
    }

    _cs_format_compact_minutes(buf, size, (long)(difftime(now, when) / 60.0));
    return TRUE;
}

static bool _cs_format_game_delta(char *buf, int size, s32b saved_turn)
{
    long long minutes;
    long long turns_per_day = (long long)TURNS_PER_TICK * (long long)TOWN_DAWN;

    if (saved_turn < 0 || game_turn < saved_turn)
        return FALSE;

    minutes = ((long long)(game_turn - saved_turn) * 1440LL) / turns_per_day;
    _cs_format_compact_minutes(buf, size, (long)minutes);
    return TRUE;
}

static void _cs_slots_init(_cs_slot_t slots[4])
{
    int i;

    for (i = 0; i < 4; i++)
    {
        slots[i].slot = _cs_slot_keys[i];
        slots[i].saved_at = 0;
        slots[i].saved_game_turn = -1;
        slots[i].doc = NULL;
    }
}

static void _cs_slots_free(_cs_slot_t slots[4])
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (slots[i].doc)
            doc_free(slots[i].doc);
        slots[i].doc = NULL;
        slots[i].saved_at = 0;
        slots[i].saved_game_turn = -1;
    }
}

static void _cs_slots_apply_doc_meta(_cs_slot_t slots[4], doc_ptr doc)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (!slots[i].doc) continue;
        doc_change_name(slots[i].doc, string_buffer(doc->name));
        doc_change_html_header(slots[i].doc, string_buffer(doc->html_header));
        doc_change_html_footer(slots[i].doc, string_buffer(doc->html_footer));
    }
}

#define _CS_SNAPSHOT_DOC_MAX (256 * 1024)

static void _cs_slots_load(_cs_slot_t slots[4])
{
    char       path[1024];
    FILE      *fp;
    string_ptr line = string_alloc();

    _cs_build_path(path, sizeof(path));
    fp = my_fopen(path, "r");
    if (!fp)
    {
        string_free(line);
        return;
    }

    string_read_line(line, fp);
    if (!streq(string_buffer(line), _cs_signature()))
        goto cleanup;

    while (!feof(fp))
    {
        char          slot_name[16];
        unsigned long saved_at;
        long          saved_game_turn;
        int           doc_size;
        int           slot;
        int           idx;
        char         *serialized;
        doc_ptr       doc;
        int           sep;

        string_read_line(line, fp);
        if (!string_length(line))
            continue;
        if (4 != sscanf(string_buffer(line), "slot %15s %lu %ld %d", slot_name, &saved_at, &saved_game_turn, &doc_size))
            break;
        if (doc_size < 0 || doc_size > _CS_SNAPSHOT_DOC_MAX)
        {
            char size_desc[32];

            if (doc_size < 0)
                strnfmt(size_desc, sizeof(size_desc), "negative");
            else
                strnfmt(size_desc, sizeof(size_desc), "%dK", (doc_size + 1023) / 1024);
            msg_format("Failed to load from snapshot file: %s - slot size (%s) out of range!", path, size_desc);
            break;
        }

        slot = _cs_slot_by_name(slot_name);
        idx = _cs_slot_index(slot);
        if (idx < 0)
            break;

        serialized = malloc(doc_size + 1);
        if (!serialized)
            break;
        if ((int)fread(serialized, 1, doc_size, fp) != doc_size)
        {
            free(serialized);
            break;
        }
        serialized[doc_size] = '\0';

        sep = fgetc(fp);
        if (sep == '\r')
            sep = fgetc(fp);

        doc = doc_alloc(80);
        doc_insert(doc, serialized);
        free(serialized);

        if (slots[idx].doc)
            doc_free(slots[idx].doc);
        slots[idx].doc = doc;
        slots[idx].saved_at = (time_t)saved_at;
        slots[idx].saved_game_turn = (s32b)saved_game_turn;
    }

cleanup:
    my_fclose(fp);
    string_free(line);
}

static bool _cs_slots_save(_cs_slot_t slots[4])
{
    char path[1024];
    char temp[1024];
    FILE *fp;
    int   i;
    bool  has_slots = FALSE;

    _cs_build_path(path, sizeof(path));
    strnfmt(temp, sizeof(temp), "%s.tmp", path);

    for (i = 0; i < 4; i++)
    {
        if (slots[i].doc)
        {
            has_slots = TRUE;
            break;
        }
    }

    if (!has_slots)
    {
        fd_kill(path);
        return TRUE;
    }

    fp = my_fopen(temp, "w");
    if (!fp)
        return FALSE;

    fprintf(fp, "%s\n", _cs_signature());
    for (i = 0; i < 4; i++)
    {
        string_ptr serialized;

        if (!slots[i].doc) continue;

        serialized = _doc_write_doc_string(slots[i].doc);
        fprintf(fp, "slot %s %lu %ld %d\n",
            _cs_slot_name(slots[i].slot),
            (unsigned long)slots[i].saved_at,
            (long)slots[i].saved_game_turn,
            string_length(serialized));
        string_write_file(serialized, fp);
        fputc('\n', fp);
        string_free(serialized);
    }

    if (my_fclose(fp))
    {
        fd_kill(temp);
        return FALSE;
    }
    if (!_replace_file(temp, path))
    {
        fd_kill(temp);
        return FALSE;
    }
    return TRUE;
}

static int _cs_save_slot_key(int cmd)
{
    switch (cmd)
    {
    case 'c':
    case '0':
    case '\r':
    case '\n':
        return 0;
    case '6':
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    default:
        return -1;
    }
}

static int _cs_load_slot_key(int cmd)
{
    switch (cmd)
    {
    case '&':
    case ':':
        return 7;
    case '*':
        return 8;
    case '(':
        return 9;
    case ')':
    case '=':
        return 0;
    default:
        return -1;
    }
}

static int _cs_clamp_top(doc_ptr doc, int top, int page_size)
{
    if (top < 0)
        top = 0;
    if (top > doc->cursor.y - page_size)
        top = MAX(0, doc->cursor.y - page_size);
    return top;
}

static int _cs_anchor_top(doc_ptr from_doc, int from_top, doc_ptr to_doc, int page_size)
{
    int top_limit = MIN(from_doc->cursor.y, from_top + page_size);
    int i;

    for (i = 0; i < vec_length(from_doc->bookmarks); i++)
    {
        doc_bookmark_ptr from_mark = vec_get(from_doc->bookmarks, i);
        int              offset;
        doc_pos_t        to_pos;

        if (from_mark->pos.y < from_top) continue;
        if (from_mark->pos.y >= top_limit) break;

        offset = from_mark->pos.y - from_top;
        to_pos = doc_find_bookmark(to_doc, string_buffer(from_mark->name));
        if (doc_pos_is_valid(to_pos))
            return _cs_clamp_top(to_doc, to_pos.y - offset, page_size);
    }

    return _cs_clamp_top(to_doc, from_top, page_size);
}

static void _cs_status_line(char *buf, int size, bool save_mode, bool compare_armed, _doc_search_ptr search)
{
    if (save_mode)
    {
        my_strcpy(buf, "Save snapshot: c/0/Enter default, 7/8/9 extra, ? help, ESC cancel", size);
        return;
    }
    if (compare_armed)
    {
        my_strcpy(buf, "Press C again to compare with the default snapshot. Any other key cancels.", size);
        return;
    }
    _doc_search_status(buf, size, search);
}

static void _cs_notice(char *buf, int size, byte *color, byte notice_color, cptr fmt, ...)
{
    va_list vp;

    va_start(vp, fmt);
    (void)vstrnfmt(buf, size, fmt, vp);
    va_end(vp);
    *color = notice_color;
}

static void _cs_show_slot_message(char *buf, int size, byte *color, int slot, time_t saved_at, s32b saved_game_turn)
{
    char turns[32];
    char game[32];
    char saved[32];
    bool have_turns = _cs_format_turn_delta(turns, sizeof(turns), saved_game_turn);
    bool have_game = _cs_format_game_delta(game, sizeof(game), saved_game_turn);
    bool saved_relative = _cs_format_saved_delta(saved, sizeof(saved), saved_at);

    if (have_turns && have_game)
    {
        if (saved_relative)
            _cs_notice(buf, size, color, TERM_L_GREEN, "Loaded %s snapshot: %s, %s ago (game), saved %s ago.", _cs_slot_display_name(slot), turns, game, saved);
        else
            _cs_notice(buf, size, color, TERM_L_GREEN, "Loaded %s snapshot: %s, %s ago (game), saved %s.", _cs_slot_display_name(slot), turns, game, saved);
    }
    else
    {
        if (saved_relative)
            _cs_notice(buf, size, color, TERM_L_GREEN, "Loaded %s snapshot: saved %s ago.", _cs_slot_display_name(slot), saved);
        else
            _cs_notice(buf, size, color, TERM_L_GREEN, "Loaded %s snapshot: saved %s.", _cs_slot_display_name(slot), saved);
    }
}

static bool _cs_switch_doc(doc_ptr *active_doc, int *active_slot, int slot, _cs_slot_t slots[4], int *top, int page_size, _doc_search_ptr search, char *notice, int notice_size, byte *notice_color)
{
    int idx = _cs_slot_index(slot);
    doc_ptr next;

    if (idx < 0 || !slots[idx].doc)
    {
        _cs_notice(notice, notice_size, notice_color, TERM_YELLOW, "No snapshot is saved in %s.", _cs_slot_display_name(slot));
        return FALSE;
    }

    next = slots[idx].doc;
    _doc_clear_selection(*active_doc);
    _doc_clear_selection(next);
    *top = _cs_anchor_top(*active_doc, *top, next, page_size);
    *active_doc = next;
    *active_slot = slot;
    memset(search, 0, sizeof(*search));
    _cs_show_slot_message(notice, notice_size, notice_color, slot, slots[idx].saved_at, slots[idx].saved_game_turn);
    return TRUE;
}

static void _cs_switch_live(doc_ptr live_doc, doc_ptr *active_doc, int *active_slot, int *top, int page_size, _doc_search_ptr search, char *notice, int notice_size, byte *notice_color)
{
    _doc_clear_selection(*active_doc);
    _doc_clear_selection(live_doc);
    *top = _cs_anchor_top(*active_doc, *top, live_doc, page_size);
    *active_doc = live_doc;
    *active_slot = -1;
    memset(search, 0, sizeof(*search));
    _cs_notice(notice, notice_size, notice_color, TERM_L_GREEN, "Viewing current character sheet.");
}

static bool _cs_save_current_slot(_cs_slot_t slots[4], int slot, doc_ptr source_doc, doc_ptr *active_doc, char *notice, int notice_size, byte *notice_color)
{
    int      idx = _cs_slot_index(slot);
    doc_ptr  prior = NULL;
    doc_ptr  copy;
    time_t   prior_saved_at;
    s32b     prior_saved_game_turn;
    bool     active_was_prior = FALSE;

    if (idx < 0)
        return FALSE;

    copy = _doc_dup(source_doc);
    prior = slots[idx].doc;
    prior_saved_at = slots[idx].saved_at;
    prior_saved_game_turn = slots[idx].saved_game_turn;
    active_was_prior = (*active_doc == prior);

    slots[idx].doc = copy;
    slots[idx].saved_at = time(NULL);
    slots[idx].saved_game_turn = game_turn;

    if (active_was_prior)
        *active_doc = copy;

    if (!_cs_slots_save(slots))
    {
        slots[idx].doc = prior;
        slots[idx].saved_at = prior_saved_at;
        slots[idx].saved_game_turn = prior_saved_game_turn;
        if (active_was_prior)
            *active_doc = prior;
        doc_free(copy);
        _cs_notice(notice, notice_size, notice_color, TERM_RED, "Failed to save character-sheet snapshots.");
        return FALSE;
    }

    if (prior)
        doc_free(prior);

    _cs_notice(notice, notice_size, notice_color, TERM_L_GREEN, "Saved the displayed sheet to %s.", _cs_slot_display_name(slot));
    return TRUE;
}

int doc_display_character_sheet(doc_ptr doc)
{
    int              rc = _OK;
    int              i;
    char             status[255];
    rect_t           display = {0};
    int              page_size;
    int              top = 0;
    bool             done = FALSE;
    bool             compare_armed = FALSE;
    bool             compare_active = FALSE;
    bool             save_mode = FALSE;
    int              active_slot = -1;
    doc_ptr          active_doc = doc;
    _doc_search_t    search = {{0}};
    _cs_slot_t       slots[4];
    char             notice[255] = "";
    byte             notice_color = TERM_WHITE;

    Term_get_size(&display.cx, &display.cy);
    page_size = display.cy - 4;
    top = _cs_clamp_top(doc, top, page_size);
    _cs_slots_init(slots);
    _cs_slots_load(slots);
    _cs_slots_apply_doc_meta(slots, doc);

    for (i = 0; i < display.cy; i++)
        Term_erase(display.x, display.y + i, display.cx);

    while (!done)
    {
        int cmd;
        int slot_cmd;
        bool handled = FALSE;

        Term_erase(display.x, display.y, display.cx);
        c_put_str(TERM_L_GREEN, format("[Character Sheet, Line %d/%d]", top, active_doc->cursor.y), display.y, display.x);
        Term_erase(display.x, display.y + 1, display.cx);
        if (notice[0])
            c_put_str(notice_color, notice, display.y + 1, display.x);
        doc_sync_term(active_doc, doc_region_create(0, top, active_doc->width, top + page_size - 1), doc_pos_create(display.x, display.y + 2));
        Term_erase(display.x, display.y + display.cy - 2, display.cx);
        _cs_status_line(status, sizeof(status), save_mode, compare_armed, &search);
        if (status[0])
            c_put_str(TERM_YELLOW, status, display.y + display.cy - 2, display.x);
        Term_erase(display.x, display.y + display.cy - 1, display.cx);
        if (save_mode)
        {
            c_put_str(TERM_L_GREEN, "Save to: c/0/Enter = default slot, 7/8/9 = extra slots, ? = help, ESC to cancel.",
                display.y + display.cy - 1, display.x);
        }
        else
        {
            c_put_str(TERM_L_GREEN, "Press c to save a snapshot, C to compare, / to search, ? for help, Esc to exit.",
                display.y + display.cy - 1, display.x);
        }

        cmd = inkey_special(TRUE);

        if (save_mode)
        {
            int save_slot = _cs_save_slot_key(cmd);

            if (cmd == ESCAPE)
            {
                save_mode = FALSE;
                continue;
            }
            if (cmd == '?')
            {
                rc = doc_display_help_aux("charsheet.txt", "CharacterCompare", display);
                if (rc == _UNWIND)
                    done = TRUE;
                continue;
            }
            if (save_slot >= 0)
            {
                save_mode = FALSE;
                (void)_cs_save_current_slot(slots, save_slot, active_doc, &active_doc, notice, sizeof(notice), &notice_color);
                continue;
            }

            save_mode = FALSE;
        }

        if (compare_armed)
        {
            if (cmd == ESCAPE)
            {
                compare_armed = FALSE;
                continue;
            }
            if (cmd == '?')
            {
                rc = doc_display_help_aux("charsheet.txt", "CharacterCompare", display);
                if (rc == _UNWIND)
                    done = TRUE;
                continue;
            }
            if (cmd == 'C')
            {
                compare_armed = FALSE;
                compare_active = TRUE;
                (void)_cs_switch_doc(&active_doc, &active_slot, 0, slots, &top, page_size, &search, notice, sizeof(notice), &notice_color);
                continue;
            }
            compare_armed = FALSE;
        }

        slot_cmd = _cs_load_slot_key(cmd);
        if (slot_cmd >= 0)
        {
            if (active_slot == slot_cmd)
            {
                _cs_switch_live(doc, &active_doc, &active_slot, &top, page_size, &search, notice, sizeof(notice), &notice_color);
                compare_active = TRUE;
            }
            else if (_cs_switch_doc(&active_doc, &active_slot, slot_cmd, slots, &top, page_size, &search, notice, sizeof(notice), &notice_color))
                compare_active = TRUE;
            continue;
        }

        if (cmd == 'c')
        {
            save_mode = TRUE;
            continue;
        }
        if (cmd == 'C')
        {
            int slot0_idx = _cs_slot_index(0);

            if (slot0_idx < 0 || !slots[slot0_idx].doc)
            {
                if (compare_active && active_slot != -1)
                    _cs_switch_live(doc, &active_doc, &active_slot, &top, page_size, &search, notice, sizeof(notice), &notice_color);
                continue;
            }

            if (compare_active)
            {
                if (active_slot == 0)
                    _cs_switch_live(doc, &active_doc, &active_slot, &top, page_size, &search, notice, sizeof(notice), &notice_color);
                else
                    (void)_cs_switch_doc(&active_doc, &active_slot, 0, slots, &top, page_size, &search, notice, sizeof(notice), &notice_color);
            }
            else
                compare_armed = TRUE;
            continue;
        }

        if (_doc_search_prompt_cmd(cmd, &search))
        {
            _doc_search_open(active_doc, display, &search);
            continue;
        }
        if (search.active)
        {
            if (cmd == '\r' || cmd == '\n' || cmd == KTRL('F') || _doc_cmd_is_f3(cmd))
            {
                _doc_search_next(active_doc, &search, &top, page_size);
                continue;
            }
            if (cmd == '\\' || _doc_cmd_is_shift_f3(cmd))
            {
                _doc_search_prev(active_doc, &search, &top, page_size);
                continue;
            }
        }

        if (rogue_like_commands)
        {
            if (cmd == 'j')
            {
                top++;
                top = _cs_clamp_top(active_doc, top, page_size);
                continue;
            }
            else if (cmd == 'k')
            {
                top--;
                top = _cs_clamp_top(active_doc, top, page_size);
                continue;
            }
            else if (cmd == KTRL('F'))
            {
                top += page_size;
                top = _cs_clamp_top(active_doc, top, page_size);
                continue;
            }
            else if (cmd == KTRL('B'))
            {
                top -= page_size;
                top = _cs_clamp_top(active_doc, top, page_size);
                continue;
            }
        }

        switch (cmd)
        {
        case '?':
            rc = doc_display_help_aux("charsheet.txt", NULL, display);
            if (rc == _UNWIND)
                done = TRUE;
            handled = TRUE;
            break;
        case ESCAPE:
            done = TRUE;
            handled = TRUE;
            break;
        case 'q':
        case 'Q':
            done = TRUE;
            rc = _UNWIND;
            handled = TRUE;
            break;
        case SKEY_TOP:
        case '7':
            top = 0;
            handled = TRUE;
            break;
        case SKEY_BOTTOM:
        case '1':
            top = MAX(0, active_doc->cursor.y - page_size);
            handled = TRUE;
            break;
        case SKEY_PGUP:
        case '9':
            top -= page_size;
            top = _cs_clamp_top(active_doc, top, page_size);
            handled = TRUE;
            break;
        case SKEY_PGDOWN:
        case '3':
            top += page_size;
            top = _cs_clamp_top(active_doc, top, page_size);
            handled = TRUE;
            break;
        case SKEY_UP:
        case '8':
            top--;
            top = _cs_clamp_top(active_doc, top, page_size);
            handled = TRUE;
            break;
        case SKEY_DOWN:
        case '2':
            top++;
            top = _cs_clamp_top(active_doc, top, page_size);
            handled = TRUE;
            break;
        case '>':
        {
            doc_pos_t pos = doc_next_bookmark(active_doc, doc_pos_create(active_doc->width - 1, top));
            if (doc_pos_is_valid(pos))
                top = _cs_clamp_top(active_doc, pos.y, page_size);
            handled = TRUE;
            break;
        }
        case '<':
        {
            doc_pos_t pos = doc_prev_bookmark(active_doc, doc_pos_create(0, top));
            if (doc_pos_is_valid(pos))
                top = pos.y;
            else
                top = 0;
            handled = TRUE;
            break;
        }
        case '|':
        {
            FILE *fp2;
            char buf[1024];
            char name[82];
            int  cb;
            int  format = DOC_FORMAT_TEXT;

            strcpy(name, string_buffer(active_doc->name));

            if (!get_string("File name: ", name, 80)) break;
            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

            cb = strlen(buf);
            if (cb > 5 && strcmp(buf + cb - 5, ".html") == 0)
                format = DOC_FORMAT_HTML;
            else if (cb > 4 && strcmp(buf + cb - 4, ".htm") == 0)
                format = DOC_FORMAT_HTML;

            if ((format != DOC_FORMAT_HTML) && strlen(buf))
            {
                char nuname[1024], prompt[256];
                int j, paikka = 0;
                strcpy(nuname, buf);
                for (j = strlen(buf) - 1; ((j > 0) && (j > (int)strlen(buf) - 7)); j--)
                {
                    unsigned char testi = buf[j];
                    if (testi == '/') break;
                    if (testi == '.')
                    {
                        paikka = j + 1;
                        break;
                    }
                }
                if (paikka)
                {
                    for (j = strlen(buf) - 1; j > paikka - 2; j--)
                        nuname[j] = '\0';
                }
                strcat(nuname, ".html");
                snprintf(prompt, sizeof(prompt), "Please note that the angband.live ladder only accepts HTML dumps.\n<color:y>Save dump as</color> <color:R>%.50s</color><color:y>? [y/n]</color>", nuname);
                if (msg_prompt(prompt, "ny", PROMPT_DEFAULT) == 'y')
                {
                    strcpy(buf, nuname);
                    format = DOC_FORMAT_HTML;
                }
            }

            fp2 = my_fopen(buf, "w");
            if (!fp2)
            {
                msg_format("Failed to open file: %s", buf);
                handled = TRUE;
                break;
            }

            doc_write_file(active_doc, fp2, format);
            my_fclose(fp2);
            msg_format("Created file: %s", buf);
            msg_print(NULL);
            handled = TRUE;
            break;
        }
        default:
            break;
        }
        if (handled)
            continue;

        {
            doc_pos_t pos = doc_next_bookmark_char(active_doc, doc_pos_create(1, top), cmd);
            if (!doc_pos_is_valid(pos))
                pos = doc_next_bookmark_char(active_doc, doc_pos_create(0, 0), cmd);
            if (doc_pos_is_valid(pos))
                top = _cs_clamp_top(active_doc, pos.y, page_size);
        }
    }

    _cs_slots_free(slots);
    return rc;
}

int doc_display_aux(doc_ptr doc, cptr caption, int top, rect_t display)
{
    int     rc = _OK;
    int     i;
    char    search_status[255];
    int     page_size;
    bool    done = FALSE;
    bool    verify_format_hack = (strpos("Character Sheet", caption) == 1);
    _doc_search_t search = {{0}};

    page_size = display.cy - 4;

    if (top < 0)
        top = 0;
    if (top > doc->cursor.y - page_size)
        top = MAX(0, doc->cursor.y - page_size);

    for (i = 0; i < display.cy; i++)
        Term_erase(display.x, display.y + i, display.cx);

    while (!done)
    {
        int cmd;

        Term_erase(display.x, display.y, display.cx);
        c_put_str(TERM_L_GREEN, format("[%s, Line %d/%d]", caption, top, doc->cursor.y), display.y, display.x);
        doc_sync_term(doc, doc_region_create(0, top, doc->width, top + page_size - 1), doc_pos_create(display.x, display.y + 2));
        Term_erase(display.x, display.y + display.cy - 2, display.cx);
        _doc_search_status(search_status, sizeof(search_status), &search);
        if (search_status[0])
            c_put_str(TERM_YELLOW, search_status, display.y + display.cy - 2, display.x);
        Term_erase(display.x, display.y + display.cy - 1, display.cx);
        c_put_str(TERM_L_GREEN, "[ESC exit. / or Ctrl+F find. Enter/F3 next. Shift+F3 prev. ? help]",
            display.y + display.cy - 1, display.x);

        cmd = inkey_special(TRUE);

        if (_doc_search_prompt_cmd(cmd, &search))
        {
            _doc_search_open(doc, display, &search);
            continue;
        }
        if (search.active)
        {
            if (cmd == '\r' || cmd == '\n' || cmd == KTRL('F') || _doc_cmd_is_f3(cmd))
            {
                _doc_search_next(doc, &search, &top, page_size);
                continue;
            }
            if (cmd == '\\' || _doc_cmd_is_shift_f3(cmd))
            {
                _doc_search_prev(doc, &search, &top, page_size);
                continue;
            }
        }

        /* links */
        if ('a' <= cmd && cmd <= 'z')
        {
            doc_link_ptr link = int_map_find(doc->links, cmd);
            if (link)
            {
                rc = doc_display_help_aux(string_buffer(link->file), string_buffer(link->topic), display);
                if (rc == _UNWIND)
                    done = TRUE;
                continue;
            }
        }

        /* vi like movement for roguelike keyset */
        if (rogue_like_commands)
        {
            if (cmd == 'j')
            {
                top++;
                if (top > doc->cursor.y - page_size)
                    top = MAX(0, doc->cursor.y - page_size);
                continue;
            }
            else if (cmd == 'k')
            {
                top--;
                if (top < 0) top = 0;
                continue;
            }
            else if (cmd == KTRL('F'))
            {
                top += page_size;
                if (top > doc->cursor.y - page_size)
                    top = MAX(0, doc->cursor.y - page_size);
                continue;
            }
            else if (cmd == KTRL('B'))
            {
                top -= page_size;
                if (top < 0) top = 0;
                continue;
            }
        }

        switch (cmd)
        {
        case '?':
            if (!strstr(caption, "helpinfo.txt"))
            {
                rc = doc_display_help_aux("helpinfo.txt", NULL, display);
                if (rc == _UNWIND)
                    done = TRUE;
            }
            break;
        case '!':
            if (!strstr(caption, "start.txt"))
            {
                rc = doc_display_help_aux("start.txt", NULL, display);
                if (rc == _UNWIND)
                    done = TRUE;
            }
            break;
        case ESCAPE:
            done = TRUE;
            break;
        case 'q':
        case 'Q':
            done = TRUE;
            rc = _UNWIND;
            break;
        case SKEY_TOP:
        case '7':
            top = 0;
            break;
        case SKEY_BOTTOM:
        case '1':
            top = MAX(0, doc->cursor.y - page_size);
            break;
        case SKEY_PGUP:
        case '9':
            top -= page_size;
            if (top < 0) top = 0;
            break;
        case SKEY_PGDOWN:
        case '3':
            top += page_size;
            if (top > doc->cursor.y - page_size)
                top = MAX(0, doc->cursor.y - page_size);
            break;
        case SKEY_UP:
        case '8':
            top--;
            if (top < 0) top = 0;
            break;
        case SKEY_DOWN:
        case '2':
            top++;
            if (top > doc->cursor.y - page_size)
                top = MAX(0, doc->cursor.y - page_size);
            break;
        case '>':
        {
            doc_pos_t pos = doc_next_bookmark(doc, doc_pos_create(doc->width - 1, top));
            if (doc_pos_is_valid(pos))
            {
                top = pos.y;
                if (top > doc->cursor.y - page_size)
                    top = MAX(0, doc->cursor.y - page_size);
            }
            break;
        }
        case '<':
        {
            doc_pos_t pos = doc_prev_bookmark(doc, doc_pos_create(0, top));
            if (doc_pos_is_valid(pos))
                top = pos.y;
            else
                top = 0;
            break;
        }
        case '|':
        {
            FILE *fp2;
            char buf[1024];
            char name[82];
            int  cb;
            int  format = DOC_FORMAT_TEXT;

            strcpy(name, string_buffer(doc->name));

            if (!get_string("File name: ", name, 80)) break;
            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

            cb = strlen(buf);
            if (cb > 5 && strcmp(buf + cb - 5, ".html") == 0)
                format = DOC_FORMAT_HTML;
            else if (cb > 4 && strcmp(buf + cb - 4, ".htm") == 0)
                format = DOC_FORMAT_HTML;

            if ((format != DOC_FORMAT_HTML) && (verify_format_hack) && (strlen(buf)))
            {
                char nuname[1024], prompt[256];
                int i, paikka = 0;
                strcpy(nuname, buf);
                for (i = strlen(buf) - 1; ((i > 0) && (i > (int)strlen(buf) - 7)); i--)
                {
                    unsigned char testi = buf[i];
                    if (testi == '/') break;
                    if (testi == '.')
                    {
                        paikka = i + 1;
                        break;
                    }
                }
                if (paikka)
                {
                    for (i = strlen(buf) - 1; i > paikka - 2; i--)
                    {
                        nuname[i] = '\0';
                    }
                }
                strcat(nuname, ".html");
                snprintf(prompt, sizeof(prompt), "Please note that the angband.live ladder only accepts HTML dumps.\n<color:y>Save dump as</color> <color:R>%.50s</color><color:y>? [y/n]</color>", nuname);
                if (msg_prompt(prompt, "ny", PROMPT_DEFAULT) == 'y')
                {
                    strcpy(buf, nuname);
                    format = DOC_FORMAT_HTML;
                }
            }

            fp2 = my_fopen(buf, "w");
            if (!fp2)
            {
                msg_format("Failed to open file: %s", buf);
                break;
            }

            doc_write_file(doc, fp2, format);
            my_fclose(fp2);
            msg_format("Created file: %s", buf);
            msg_print(NULL);
            break;
        }
        default:
        {   /* BETA: Any unhandled keystroke will navigate to the next topic based
                     upon a comparison of the first letter. This is nice, say, for
                     viewing the Character Sheet and navigating to the various sections */
            doc_pos_t pos = doc_next_bookmark_char(doc, doc_pos_create(1, top), cmd);
            if (!doc_pos_is_valid(pos)) /* wrap */
                pos = doc_next_bookmark_char(doc, doc_pos_create(0, 0), cmd);
            if (doc_pos_is_valid(pos))
            {
                top = pos.y;
                if (top > doc->cursor.y - page_size)
                    top = MAX(0, doc->cursor.y - page_size);
            }
        }
        }
    }
    return rc;
}

/* Modified version of doc_display_aux() to fulfill the needs of the proficiency table */
int weapon_exp_display(doc_ptr doc, cptr caption, int *top)
{
    int     rc = 0;
    bool    change_mode = FALSE;
    int     i;
    char    finder_str[81];
    char    back_str[81];
    int     page_size;
    bool    done = FALSE;
    rect_t display = {0};
    Term_get_size(&display.cx, &display.cy);

    strcpy(finder_str, "");

    page_size = display.cy - 4;

    if (*top < 0)
        *top = 0;
    if (*top > doc->cursor.y - page_size)
        *top = MAX(0, doc->cursor.y - page_size);

    for (i = 0; i < display.cy; i++)
        Term_erase(display.x, display.y + i, display.cx);

    while (!done)
    {
        int cmd;

        Term_erase(display.x, display.y, display.cx);
        c_put_str(TERM_L_GREEN, format("[%s, Line %d/%d]", caption, *top, doc->cursor.y), display.y, display.x);
        doc_sync_term(doc, doc_region_create(0, *top, doc->width, *top + page_size - 1), doc_pos_create(display.x, display.y + 2));
        Term_erase(display.x, display.y + display.cy - 1, display.cx);
        c_put_str(TERM_L_GREEN, "[Press ESC to exit. Press M to toggle mode. Press ? for help]", display.y + display.cy - 1, display.x);

        cmd = inkey_special(TRUE);

        /* vi like movement for roguelike keyset */
        if (rogue_like_commands)
        {
            if (cmd == 'j')
            {
                *top += 1;
                if (*top > doc->cursor.y - page_size)
                    *top = MAX(0, doc->cursor.y - page_size);
                continue;
            }
            else if (cmd == 'k')
            {
                *top -= 1;
                if (*top < 0) *top = 0;
                continue;
            }
            else if (cmd == KTRL('F'))
            {
                *top += page_size;
                if (*top > doc->cursor.y - page_size)
                    *top = MAX(0, doc->cursor.y - page_size);
                continue;
            }
            else if (cmd == KTRL('B'))
            {
                *top -= page_size;
                if (*top < 0) *top = 0;
                continue;
            }
        }

        switch (cmd)
        {
        case '?':
            if (!strstr(caption, "helpinfo.txt"))
            {
                rc = doc_display_help_aux("helpinfo.txt", NULL, display);
                if (rc == 1)
                    done = TRUE;
            }
            break;
        case ESCAPE:
        case 'q':
            done = TRUE;
            break;
        case SKEY_TOP:
        case '7':
            *top = 0;
            break;
        case SKEY_BOTTOM:
        case '1':
            *top = MAX(0, doc->cursor.y - page_size);
            break;
        case SKEY_PGUP:
        case '9':
            *top -= page_size;
            if (*top < 0) *top = 0;
            break;
        case SKEY_PGDOWN:
        case '3':
            *top += page_size;
            if (*top > doc->cursor.y - page_size)
                *top = MAX(0, doc->cursor.y - page_size);
            break;
        case SKEY_UP:
        case '8':
            *top -= 1;
            if (*top < 0) *top = 0;
            break;
        case SKEY_DOWN:
        case '2':
            *top += 1;
            if (*top > doc->cursor.y - page_size)
                *top = MAX(0, doc->cursor.y - page_size);
            break;
        case '>':
        {
            doc_pos_t pos = doc_next_bookmark(doc, doc_pos_create(doc->width - 1, *top));
            if (doc_pos_is_valid(pos))
            {
                *top = pos.y;
                if (*top > doc->cursor.y - page_size)
                    *top = MAX(0, doc->cursor.y - page_size);
            }
            break;
        }
        case '<':
        {
            doc_pos_t pos = doc_prev_bookmark(doc, doc_pos_create(0, *top));
            if (doc_pos_is_valid(pos))
                *top = pos.y;
            else
                *top = 0;
            break;
        }
        case '|':
        {
            FILE *fp2;
            char buf[1024];
            char name[82];
            int  cb;
            int  format = DOC_FORMAT_TEXT;

            strcpy(name, string_buffer(doc->name));

            if (!get_string("File name: ", name, 80)) break;
            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
            fp2 = my_fopen(buf, "w");
            if (!fp2)
            {
                msg_format("Failed to open file: %s", buf);
                break;
            }

            cb = strlen(buf);
            if (cb > 5 && strcmp(buf + cb - 5, ".html") == 0)
                format = DOC_FORMAT_HTML;
            else if (cb > 4 && strcmp(buf + cb - 4, ".htm") == 0)
                format = DOC_FORMAT_HTML;

            doc_write_file(doc, fp2, format);
            my_fclose(fp2);
            msg_format("Created file: %s", buf);
            msg_print(NULL);
            break;
        }
        case '/':
            Term_erase(display.x, display.y + display.cy - 1, display.cx);
            put_str("Find: ", display.y + display.cy - 1, display.x);
            strcpy(back_str, finder_str);
            if (askfor(finder_str, 80))
            {
                if (finder_str[0])
                {
                    doc_pos_t pos = doc->selection.stop;
                    if (!doc_pos_is_valid(pos))
                        pos = doc_pos_create(0, *top);
                    pos = doc_find_next(doc, finder_str, pos);
                    if (doc_pos_is_valid(pos))
                    {
                        *top = pos.y;
                        if (*top > doc->cursor.y - page_size)
                            *top = MAX(0, doc->cursor.y - page_size);
                    }
                }
            }
            else strcpy(finder_str, back_str);
            break;
        case '\\':
            Term_erase(display.x, display.y + display.cy - 1, display.cx);
            put_str("Find: ", display.y + display.cy - 1, display.x);
            strcpy(back_str, finder_str);
            if (askfor(finder_str, 80))
            {
                if (finder_str[0])
                {
                    doc_pos_t pos = doc->selection.start;
                    if (!doc_pos_is_valid(pos))
                        pos = doc_pos_create(doc->width, *top + page_size);
                    pos = doc_find_prev(doc, finder_str, pos);
                    if (doc_pos_is_valid(pos))
                    {
                        *top = pos.y;
                        if (*top > doc->cursor.y - page_size)
                            *top = MAX(0, doc->cursor.y - page_size);
                    }
                }
            }
            else strcpy(finder_str, back_str);
            break;
        case 'm':
        case 'M':
            change_mode = TRUE;
            done = TRUE;
            break;
        default:
            break;
        }
    }
    return (change_mode) ? 1 : 0;
}
int doc_display_help(cptr file_name, cptr topic)
{
    rect_t display = {0};
    Term_get_size(&display.cx, &display.cy);
    return doc_display_help_aux(file_name, topic, display);
}
int doc_display_help_aux(cptr file_name, cptr topic, rect_t display)
{
    int     rc = _OK;
    FILE   *fp = NULL;
    char    path[1024];
    char    caption[1024];
    doc_ptr doc = NULL;
    int     top = 0;

    /* Check for file_name#topic from a lazy client */
    if (!topic)
    {
        cptr pos = strchr(file_name, '#');
        if (pos)
        {
            string_ptr name = string_copy_sn(file_name, pos - file_name);
            int        result = doc_display_help_aux(string_buffer(name), pos + 1, display);

            string_free(name);
            return result;
        }
    }

    sprintf(caption, "Help file '%s'", file_name);
    path_build(path, sizeof(path), ANGBAND_DIR_HELP, file_name);
    fp = my_fopen(path, "r");
    if (!fp)
    {
        cmsg_format(TERM_VIOLET, "Cannot open '%s'.", file_name);
        msg_print(NULL);
        return _OK;
    }

    doc = doc_alloc(MIN(80, display.cx));
    doc_read_file(doc, fp);
    my_fclose(fp);

    if (topic)
    {
        doc_pos_t pos = doc_find_bookmark(doc, topic);
        if (doc_pos_is_valid(pos))
            top = pos.y;
    }

    rc = doc_display_aux(doc, caption, top, display);
    doc_free(doc);
    return rc;
}
