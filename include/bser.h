#ifndef BSER_H
#define BSER_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/* Export / Import visibility macros */
#if defined(_MSC_VER) && defined(BSER_EXPORTS)
#define BSER_EXPORT __declspec(dllexport)
#elif defined(_MSC_VER) && defined(BSER_DLL)
#define BSER_EXPORT __declspec(dllimport)
#elif defined(BSER_EXPORTS)
#define BSER_EXPORT __attribute__((visibility("default")))
#else
#define BSER_EXPORT
#endif

/* C / C++ Interoperability & Inline semantics */
#ifdef __cplusplus
#define BSER_API extern "C"
#define BSER_INLINE inline
#else
#define BSER_API
#define BSER_INLINE static inline
#endif

/* Debug breakpoints */
#if defined(_MSC_VER)
#define BSER_BREAKPOINT __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define BSER_BREAKPOINT __builtin_trap()
#else
#define BSER_BREAKPOINT assert(0)
#endif

/* No-throw annotations across compilers */
#if defined(__cplusplus)
#define BSER_NOEXCEPT noexcept
#elif defined(__GNUC__) || defined(__clang__)
#define BSER_NOEXCEPT __attribute__((nothrow))
#elif defined(_MSC_VER)
#define BSER_NOEXCEPT __declspec(nothrow)
#else
#define BSER_NOEXCEPT
#endif

/* Limits & Constants */

/* Standard Configuration Defaults (User-Overrideable) */

#ifndef BSER_MAX_SCHEMAS
#define BSER_MAX_SCHEMAS 32
#endif

#ifndef BSER_MAX_FIELD_NAME_LENGTH
#define BSER_MAX_FIELD_NAME_LENGTH 32
#endif

#ifndef BSER_MAX_FIELDS
#define BSER_MAX_FIELDS 32
#endif

#ifndef BSER_MAX_READ_BATCH
#define BSER_MAX_READ_BATCH 64
#endif

#ifndef BSER_MAX_WRITE_BATCH
#define BSER_MAX_WRITE_BATCH 64
#endif

#ifndef BSER_MAX_FIELD_BYTES
#define BSER_MAX_FIELD_BYTES 128
#endif

/* Primitive Type Aliases */
typedef uint8_t  bser_byte_t;
typedef int32_t  bser_id_t;
typedef uint32_t bser_size_t;

/* Memory Packing Macros */
#define BSER_PACK(src_val, dest_buf)   memcpy((dest_buf), &(src_val), sizeof(src_val))
#define BSER_UNPACK(src_buf, dest_val) memcpy(&(dest_val), (src_buf), sizeof(dest_val))

/* --- Low-Level Stream I/O Helpers --- */

BSER_API BSER_INLINE bool bser_write_bytes(FILE* file, const void* data, size_t size) BSER_NOEXCEPT
{
    if (!file || !data || size == 0) return false;
    return fwrite(data, 1, size, file) == size;
}

BSER_API BSER_INLINE bool bser_read_bytes(FILE* file, void* data, size_t size) BSER_NOEXCEPT
{
    if (!file || !data || size == 0) return false;
    return fread(data, 1, size, file) == size;
}

BSER_API BSER_INLINE bool bser_write_byte(FILE* file, bser_byte_t value) BSER_NOEXCEPT
{
    return bser_write_bytes(file, &value, sizeof(value));
}

BSER_API BSER_INLINE bool bser_read_byte(FILE* file, bser_byte_t* out_value) BSER_NOEXCEPT
{
    return bser_read_bytes(file, out_value, sizeof(*out_value));
}

BSER_API BSER_INLINE bool bser_write_id(FILE* file, bser_id_t value) BSER_NOEXCEPT
{
    return bser_write_bytes(file, &value, sizeof(value));
}

BSER_API BSER_INLINE bool bser_read_id(FILE* file, bser_id_t* out_value) BSER_NOEXCEPT
{
    return bser_read_bytes(file, out_value, sizeof(*out_value));
}

BSER_API BSER_INLINE bool bser_write_size(FILE* file, bser_size_t size) BSER_NOEXCEPT
{
    return bser_write_bytes(file, &size, sizeof(size));
}

BSER_API BSER_INLINE bool bser_read_size(FILE* file, bser_size_t* out_size) BSER_NOEXCEPT
{
    return bser_read_bytes(file, out_size, sizeof(*out_size));
}

/* --- Record & Schema Structures --- */

typedef struct bser_record
{
    bser_id_t     id;
    char          field_names[BSER_MAX_FIELDS][BSER_MAX_FIELD_NAME_LENGTH];
    size_t        field_count;
    bser_byte_t   field_values[BSER_MAX_FIELDS][BSER_MAX_FIELD_BYTES];
} bser_record_t, bser_schema_t;

BSER_API BSER_INLINE void bser_record_init(bser_record_t* self, bser_id_t id, const char* const fields[], size_t count)
{
    if (!self) return;
    self->id = id;
    self->field_count = (count > BSER_MAX_FIELDS) ? BSER_MAX_FIELDS : count;
    for (size_t i = 0; i < self->field_count; ++i) {
        strncpy_s(self->field_names[i], fields[i], BSER_MAX_FIELD_NAME_LENGTH - 1);
        self->field_names[i][BSER_MAX_FIELD_NAME_LENGTH - 1] = '\0';
    }
}

BSER_API BSER_INLINE bool bser_record_set_field_at(bser_record_t* self, size_t index, const bser_byte_t* value, size_t len)
{
    if (!self || !value || index >= self->field_count) return false;
    size_t copy_size = (len > BSER_MAX_FIELD_BYTES) ? BSER_MAX_FIELD_BYTES : len;
    memset(self->field_values[index], 0, BSER_MAX_FIELD_BYTES);
    memcpy(self->field_values[index], value, copy_size);
    return true;
}

BSER_API BSER_INLINE bool bser_record_set_field(bser_record_t* self, const char* name, const bser_byte_t* value, size_t len)
{
    if (!self || !name) return false;
    for (size_t i = 0; i < self->field_count; ++i) {
        if (self->field_names[i] && strcmp(self->field_names[i], name) == 0) {
            return bser_record_set_field_at(self, i, value, len);
        }
    }
    return false;
}

BSER_API BSER_INLINE bool bser_record_get_field_at(const bser_record_t* self, size_t index, bser_byte_t* out_buffer, size_t max_len)
{
    if (!self || !out_buffer || index >= self->field_count) return false;
    size_t copy_size = (max_len > BSER_MAX_FIELD_BYTES) ? BSER_MAX_FIELD_BYTES : max_len;
    memcpy(out_buffer, self->field_values[index], copy_size);
    return true;
}

BSER_API BSER_INLINE bool bser_record_get_field(const bser_record_t* self, const char* name, bser_byte_t* out_buffer, size_t max_len)
{
    if (!self || !name) return false;
    for (size_t i = 0; i < self->field_count; ++i) {
        if (self->field_names[i] && strcmp(self->field_names[i], name) == 0) {
            return bser_record_get_field_at(self, i, out_buffer, max_len);
        }
    }
    return false;
}

/* --- Record Serialization --- */

BSER_API BSER_INLINE bool bser_record_write(FILE* file, const bser_record_t* record) BSER_NOEXCEPT
{
    if (!file || !record) return false;
    if (!bser_write_id(file, record->id)) return false;

    for (size_t i = 0; i < record->field_count; ++i) {
        bser_size_t size = (bser_size_t)sizeof(record->field_values[i]);
        if (!bser_write_size(file, size)) return false;
        if (!bser_write_bytes(file, record->field_values[i], size)) return false;
    }
    return true;
}

BSER_API BSER_INLINE bool bser_record_read(FILE* file, bser_record_t* record) BSER_NOEXCEPT
{
    if (!file || !record) return false;

    for (size_t i = 0; i < record->field_count; ++i) {
        bser_size_t size;
        if (!bser_read_size(file, &size)) return false;

        /* Prevent buffer overflow if the binary payload contains bad size */
        if (size > BSER_MAX_FIELD_BYTES) return false;
        if (size != 0 && !bser_read_bytes(file, record->field_values[i], size)) return false;
    }
    return true;
}

/* --- Stream Abstraction, Reader & Writer --- */

typedef struct bser_stream
{
    const wchar_t* path;
    FILE* file;
} bser_stream_t;

BSER_API BSER_INLINE bool bser_stream_open(bser_stream_t* self, const wchar_t* path, const wchar_t* mode) BSER_NOEXCEPT
{
    if (!self || !path || !mode) return false;
    self->path = path;

#if defined(_WIN32) && defined(_MSC_VER)
    return (_wfopen_s(&self->file, path, mode) == 0);
#elif defined(_WIN32)
    self->file = _wfopen(path, mode);
    return (self->file != NULL);
#else
    char char_path[1024];
    char char_mode[16];
    wcstombs(char_path, path, sizeof(char_path));
    wcstombs(char_mode, mode, sizeof(char_mode));
    self->file = fopen(char_path, char_mode);
    return (self->file != NULL);
#endif
}

BSER_API BSER_INLINE bool bser_stream_close(bser_stream_t* self) BSER_NOEXCEPT
{
    if (self && self->file != NULL) {
        fclose(self->file);
        self->file = NULL;
        return true;
    }
    return false;
}

typedef struct bser_catalog
{
    bser_schema_t schemas[BSER_MAX_SCHEMAS];
    size_t        schema_count;
} bser_catalog_t;

typedef struct bser_reader
{
    bser_stream_t  stream;
    bser_catalog_t catalog;
    bser_record_t  records[BSER_MAX_READ_BATCH];
    size_t         record_count;
    bool           has_completed;
} bser_reader_t;

BSER_API BSER_INLINE bool bser_reader_init(bser_reader_t* self, const wchar_t* path, const bser_catalog_t* catalog) BSER_NOEXCEPT
{
    if (!self || !path || !catalog) return false;

    memset(self, 0, sizeof(bser_reader_t));
    self->catalog = *catalog;

    if (!bser_stream_open(&self->stream, path, L"rb")) {
        return false;
    }

    self->has_completed = false;
    return true;
}

BSER_API BSER_INLINE bool bser_reader_close(bser_reader_t* self) BSER_NOEXCEPT
{
    if (!self) return false;
    return bser_stream_close(&self->stream);
}

BSER_API BSER_INLINE bool bser_reader_execute(bser_reader_t* self) BSER_NOEXCEPT
{
    if (!self || !self->stream.file) return false;
    if (self->has_completed) return true;

    bser_id_t id;

    while (bser_read_id(self->stream.file, &id)) {
        bool matched = false;

        for (size_t i = 0; i < self->catalog.schema_count; ++i) {
            if (self->catalog.schemas[i].id == id) {
                bser_record_t record = self->catalog.schemas[i];

                if (!bser_record_read(self->stream.file, &record)) {
                    bser_reader_close(self);
                    return false;
                }

                if (self->record_count < BSER_MAX_READ_BATCH) {
                    self->records[self->record_count++] = record;
                }
                matched = true;
                break;
            }
        }

        if (!matched) {
            bser_reader_close(self);
            return false;
        }
    }

    self->has_completed = true;
    bser_reader_close(self);
    return true;
}

BSER_API BSER_INLINE void bser_reader_deinit(bser_reader_t* self) BSER_NOEXCEPT
{
    if (self) {
        bser_stream_close(&self->stream);
        self->record_count = 0;
        self->has_completed = false;
    }
}

typedef struct bser_writer
{
    bser_stream_t stream;
    bser_record_t records[BSER_MAX_WRITE_BATCH];
    size_t        record_count;
    bool          has_completed;
} bser_writer_t;

BSER_API BSER_INLINE bool bser_writer_init(bser_writer_t* self, const wchar_t* path, const bser_record_t* records, size_t record_count) BSER_NOEXCEPT
{
    if (!self || !path || !records) return false;

    memset(self, 0, sizeof(bser_writer_t));

    self->record_count = (record_count > BSER_MAX_WRITE_BATCH) ? BSER_MAX_WRITE_BATCH : record_count;
    memcpy(self->records, records, sizeof(bser_record_t) * self->record_count);

    if (!bser_stream_open(&self->stream, path, L"wb")) {
        return false;
    }

    self->has_completed = false;
    return true;
}

BSER_API BSER_INLINE bool bser_writer_close(bser_writer_t* self) BSER_NOEXCEPT
{
    if (!self) return false;
    return bser_stream_close(&self->stream);
}

BSER_API BSER_INLINE bool bser_writer_execute(bser_writer_t* self) BSER_NOEXCEPT
{
    if (!self || !self->stream.file) return false;
    if (self->has_completed) return true;

    for (size_t i = 0; i < self->record_count; ++i) {
        if (!bser_record_write(self->stream.file, &self->records[i])) {
            bser_writer_close(self);
            return false;
        }
    }

    self->has_completed = true;
    bser_writer_close(self);
    return true;
}

BSER_API BSER_INLINE void bser_writer_deinit(bser_writer_t* self) BSER_NOEXCEPT
{
    if (self) {
        bser_stream_close(&self->stream);
        self->has_completed = false;
    }
}

#endif /* BSER_H */