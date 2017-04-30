#include "string_utilities.h"
#include "unicode_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else

typedef struct StringUtf8Source {
    Utf8Source utf8_source;
    int position;
    int length;
    const char* string;
} StringUtf8Source;

static Utf8Source* StringUtf8Source_new(const char* string);

static void StringUtf8Source_delete(Utf8Source* utf8_source);

static unsigned char StringUtf8Source_read(Utf8Source* utf8_source);

#endif /* ifdef _WIN32 ... else ... */

wchar_t* StringUtilities_copy_string(const wchar_t* string) {
    return StringUtilities_copy_string_part(string, wcslen(string));
}

wchar_t* StringUtilities_copy_string_part(const wchar_t* string, const int length) {
    wchar_t* copy = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
    wmemcpy(copy, string, length);
    copy[length] = L'\0';
    return copy;
}

#ifndef _WIN32

wchar_t* StringUtilities_copy_to_wide_string(const char* string) {
    int length = strlen(string);
    wchar_t* copy = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
    Utf8Source* utf8_source = StringUtf8Source_new(string);
    int to_index = 0;
    int i;
    for (i = 0; i < length; ++i) {
        long code_point = UnicodeUtilities_read_code_point_from_utf8_source(utf8_source);
        if (code_point == WEOF) {
            break;
        }
        if (code_point <= 0xFFFF || sizeof(wchar_t) > 2) {
            copy[to_index++] = (wchar_t)code_point;
        } else {
            Utf16Surrogates surrogates = UnicodeUtilities_get_utf16_surrogates(code_point);
            copy[to_index++] = surrogates.leading;
            copy[to_index++] = surrogates.trailing;
        }
    }
    copy[to_index] = L'\0';
    Utf8Source_delete(utf8_source);
    return copy;
}

Utf8Source* StringUtf8Source_new(const char* string) {
    StringUtf8Source* string_utf8_source = (StringUtf8Source*)malloc(sizeof(StringUtf8Source));
    string_utf8_source->utf8_source.read = &StringUtf8Source_read;
    string_utf8_source->utf8_source.delete = &StringUtf8Source_delete;
    string_utf8_source->position = 0;
    string_utf8_source->length = strlen(string);
    string_utf8_source->string = string;
    return (Utf8Source*)string_utf8_source;
}

void StringUtf8Source_delete(Utf8Source* utf8_source) {
    if (!utf8_source) {
        return;
    }
    free((void*)utf8_source);
}

unsigned char StringUtf8Source_read(Utf8Source* utf8_source) {
    StringUtf8Source* string_utf8_source = (StringUtf8Source*)utf8_source;
    if (string_utf8_source->position < string_utf8_source->length) {
        return string_utf8_source->string[string_utf8_source->position++];
    }
    return EOF;
}

#else /* _WIN32 defined */

LPWSTR StringUtilities_copy_to_wide_string(const char* string) {
    /* int MultiByteToWideChar(
            _In_      UINT   CodePage,
            _In_      DWORD  dwFlags,
            _In_      LPCSTR lpMultiByteStr,
            _In_      int    cbMultiByte,
            _Out_opt_ LPWSTR lpWideCharStr,
            _In_      int    cchWideChar
       ); */
    /* Let  MultiByteToWideChar calculate the required buffer size */
    int buffer_size = MultiByteToWideChar(CP_THREAD_ACP, 0, string, -1, 0, 0);
    if (buffer_size == 0 || buffer_size == 0xFFFD) { /* error, return empty string (0xFFFD -> UTF byte sequence invalid) */
        LPWSTR copy = (wchar_t*)malloc(1 * sizeof(wchar_t));
        copy[0] = L'\0';
        return copy;
    }
    /* The + 1 should not be necessary since the calculated buffer size
       includes the terminating null character, but MinGW compilers
       produce an unstable executable when the + 1 is not there. */
    LPWSTR copy = (wchar_t*)malloc((buffer_size + 1) * sizeof(wchar_t));
    int result = MultiByteToWideChar(CP_THREAD_ACP, 0, string, -1, copy, buffer_size);
    if (result == 0 || result != buffer_size) { /* error, return empty string */
        copy[0] = L'\0';
    } else {
        copy[buffer_size] = L'\0';
    }
    return copy;
}

#endif /* ifndef _WIN32 ... else ... */

size_t StringUtilities_code_point_length(const wchar_t* string) {
    if (sizeof(wchar_t) > 2) {
        return wcslen(string);
    } else {
        return StringUtilities_code_point_length_for_part(string, wcslen(string));
    }
}

size_t StringUtilities_code_point_length_for_part(const wchar_t* string, const int length) {
    int code_points = 0;
    int i;
    for (i = 0; i < length; ++i) {
        ++code_points;
        if (UnicodeUtilities_is_utf16_surrogate(string[i])) {
            ++i;
        }

    }
    return code_points;
}
