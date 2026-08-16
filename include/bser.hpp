// Copyright (c) 2026 Erik-Neo Östlund-Zetterberg
// See the license in the accompanying LICENSE.md file at the github repository:
// https://github.com/NeoZett/BSER

#ifndef BSER_HPP
#define BSER_HPP

#ifndef __cplusplus
#error "bser.hpp is a C++ wrapper and requires a C++ compiler. Use bser.h for C."
#endif

#if defined(_MSVC_LANG)
#if _MSVC_LANG < 201103L
#error "bser.hpp requires C++11 or higher."
#endif
#elif __cplusplus < 201103L
#error "bser.hpp requires C++11 or higher."
#endif

#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>
#include <type_traits>
#include <array>
#include <memory>
#include <bser.h>

#ifndef BSER_NOEXCEPT
#define BSER_NOEXCEPT noexcept
#endif

#ifndef BSER_NODISCARD
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard) >= 201603L
#define BSER_NODISCARD [[nodiscard]]
#else
#define BSER_NODISCARD
#endif
#endif

namespace bser
{
    class Record
    {
    public:
        using IndexType = size_t;
        using IdType = bser_id_t;
        using ByteType = bser_byte_t;

        Record() BSER_NOEXCEPT : m_handle{} {}

        Record(bser_id_t id, std::initializer_list<const char*> fields) BSER_NOEXCEPT : m_handle{}
        {
            bser_record_init(&m_handle, id, fields.begin(), fields.size());
        }

        Record(bser_id_t id, const char* const* fields, size_t field_count) BSER_NOEXCEPT : m_handle{}
        {
            bser_record_init(&m_handle, id, fields, field_count);
        }

        explicit Record(const bser_record_t& native_rec) BSER_NOEXCEPT : m_handle(native_rec) {}

        Record& set(IndexType index, const bser_byte_t* value, size_t len)
        {
            if (!bser_record_set_field_at(&m_handle, index, value, len))
            {
                throw std::runtime_error("Failed to set field at index " + std::to_string(index));
            }
            return *this;
        }

        Record& set(const char* name, const bser_byte_t* value, size_t len)
        {
            if (!bser_record_set_field(&m_handle, name, value, len))
            {
                throw std::runtime_error("Failed to set field with name: " + std::string(name));
            }
            return *this;
        }

        template <typename T>
        Record& set(IndexType index, const T& value)
        {
            static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
            return set(index, reinterpret_cast<const bser_byte_t*>(&value), sizeof(T));
        }

        template <typename T>
        Record& set(const char* name, const T& value)
        {
            static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
            return set(name, reinterpret_cast<const bser_byte_t*>(&value), sizeof(T));
        }

        BSER_NODISCARD std::array<bser_byte_t, BSER_MAX_FIELD_BYTES> get(IndexType index) const
        {
            std::array<bser_byte_t, BSER_MAX_FIELD_BYTES> buffer{};
            if (!bser_record_get_field_at(&m_handle, index, buffer.data(), buffer.size()))
            {
                throw std::runtime_error("Failed to get field at index " + std::to_string(index));
            }
            return buffer;
        }

        BSER_NODISCARD std::array<bser_byte_t, BSER_MAX_FIELD_BYTES> get(const char* name) const
        {
            std::array<bser_byte_t, BSER_MAX_FIELD_BYTES> buffer{};
            if (!bser_record_get_field(&m_handle, name, buffer.data(), buffer.size()))
            {
                throw std::runtime_error("Failed to get field with name: " + std::string(name));
            }
            return buffer;
        }

        template <typename T>
        BSER_NODISCARD T get(IndexType index) const
        {
            static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
            if (sizeof(T) > BSER_MAX_FIELD_BYTES)
            {
                throw std::invalid_argument("Type T exceeds field capacity");
            }

            T value{};
            if (!bser_record_get_field_at(&m_handle, index, reinterpret_cast<bser_byte_t*>(&value), sizeof(T)))
            {
                throw std::runtime_error("Failed to extract field value at index " + std::to_string(index));
            }
            return value;
        }

        template <typename T>
        BSER_NODISCARD T get(const char* name) const
        {
            static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
            if (sizeof(T) > BSER_MAX_FIELD_BYTES)
            {
                throw std::invalid_argument("Type T exceeds field capacity");
            }

            T value{};
            if (!bser_record_get_field(&m_handle, name, reinterpret_cast<bser_byte_t*>(&value), sizeof(T)))
            {
                throw std::runtime_error("Failed to extract field value for name: " + std::string(name));
            }
            return value;
        }

        BSER_NODISCARD size_t size() const BSER_NOEXCEPT
        {
            return m_handle.field_count;
        }

        BSER_NODISCARD IdType id() const BSER_NOEXCEPT
        {
            return m_handle.id;
        }

        BSER_NODISCARD const bser_record_t* native_handle() const BSER_NOEXCEPT
        {
            return &m_handle;
        }

    private:
        bser_record_t m_handle;
    };

    using Schema = Record;

    class SchemaCatalog
    {
    public:
        SchemaCatalog() BSER_NOEXCEPT : m_handle{} {}

        SchemaCatalog(std::initializer_list<Schema> schemas) : m_handle{}
        {
            for (const auto& sch : schemas)
            {
                add(sch);
            }
        }

        explicit SchemaCatalog(const std::vector<Schema>& schemas) : m_handle{}
        {
            for (const auto& sch : schemas)
            {
                add(sch);
            }
        }

        void add(const Schema& schema)
        {
            if (m_handle.schema_count >= BSER_MAX_SCHEMAS)
            {
                throw std::runtime_error("Catalog capacity exceeded maximum schemas limit");
            }
            m_handle.schemas[m_handle.schema_count++] = *(schema.native_handle());
        }

        BSER_NODISCARD size_t size() const BSER_NOEXCEPT
        {
            return static_cast<size_t>(m_handle.schema_count);
        }

        BSER_NODISCARD bool empty() const BSER_NOEXCEPT
        {
            return m_handle.schema_count == 0;
        }

        BSER_NODISCARD const bser_catalog_t* native_handle() const BSER_NOEXCEPT
        {
            return &m_handle;
        }

    private:
        bser_catalog_t m_handle;
    };

    class FileStream
    {
    public:
        FileStream(const wchar_t* path, const wchar_t* mode) : m_handle{}
        {
            if (!bser_stream_open(&m_handle, path, mode))
            {
                throw std::runtime_error("Failed to open file stream at specified path");
            }
        }

        FileStream(const FileStream&) = delete;
        FileStream& operator=(const FileStream&) = delete;

        FileStream(FileStream&& other) BSER_NOEXCEPT : m_handle(other.m_handle)
        {
            other.m_handle.file = nullptr;
            other.m_handle.path = nullptr;
        }

        FileStream& operator=(FileStream&& other) BSER_NOEXCEPT
        {
            if (this != &other)
            {
                close();
                m_handle = other.m_handle;
                other.m_handle.file = nullptr;
                other.m_handle.path = nullptr;
            }
            return *this;
        }

        ~FileStream() BSER_NOEXCEPT
        {
            close();
        }

        bool close() BSER_NOEXCEPT
        {
            return bser_stream_close(&m_handle);
        }

        BSER_NODISCARD const bser_stream_t* native_handle() const BSER_NOEXCEPT
        {
            return &m_handle;
        }

    private:
        bser_stream_t m_handle;
    };

    class BinaryReader
    {
    public:
        BinaryReader(const wchar_t* path, const SchemaCatalog& catalog) : m_handle{}
        {
            if (!path || !bser_reader_init(&m_handle, path, catalog.native_handle()))
            {
                throw std::runtime_error("Failed to initialize binary reader");
            }
        }

        BinaryReader(const std::wstring& path, const SchemaCatalog& catalog)
            : BinaryReader(path.c_str(), catalog)
        {
        }

        BinaryReader(const BinaryReader&) = delete;
        BinaryReader& operator=(const BinaryReader&) = delete;

        BinaryReader(BinaryReader&& other) BSER_NOEXCEPT : m_handle(other.m_handle)
        {
            std::memset(&other.m_handle, 0, sizeof(bser_reader_t));
        }

        BinaryReader& operator=(BinaryReader&& other) BSER_NOEXCEPT
        {
            if (this != &other)
            {
                bser_reader_deinit(&m_handle);
                m_handle = other.m_handle;
                std::memset(&other.m_handle, 0, sizeof(bser_reader_t));
            }
            return *this;
        }

        ~BinaryReader() BSER_NOEXCEPT
        {
            bser_reader_deinit(&m_handle);
        }

        bool close() BSER_NOEXCEPT
        {
            return bser_reader_close(&m_handle);
        }

        bool execute() BSER_NOEXCEPT
        {
            return bser_reader_execute(&m_handle);
        }

        BSER_NODISCARD std::vector<Record> records() const
        {
            std::vector<Record> record_list;
            record_list.reserve(m_handle.record_count);

            for (size_t i = 0; i < m_handle.record_count; ++i)
            {
                record_list.emplace_back(m_handle.records[i]);
            }

            return record_list;
        }

        BSER_NODISCARD bool has_completed() const BSER_NOEXCEPT
        {
            return m_handle.has_completed;
        }

        BSER_NODISCARD const bser_reader_t* native_handle() const BSER_NOEXCEPT
        {
            return &m_handle;
        }

    private:
        bser_reader_t m_handle;
    };

    class BinaryWriter
    {
    public:
        BinaryWriter(const wchar_t* path, const std::vector<Record>& records) : m_handle{}
        {
            init(path, records);
        }

        BinaryWriter(const std::wstring& path, const std::vector<Record>& records) : m_handle{}
        {
            init(path.c_str(), records);
        }

        BinaryWriter(const wchar_t* path, std::initializer_list<Record> records)
            : BinaryWriter(path, std::vector<Record>(records))
        {
        }

        BinaryWriter(const std::wstring& path, std::initializer_list<Record> records)
            : BinaryWriter(path.c_str(), std::vector<Record>(records))
        {
        }

        BinaryWriter(const BinaryWriter&) = delete;
        BinaryWriter& operator=(const BinaryWriter&) = delete;

        BinaryWriter(BinaryWriter&& other) BSER_NOEXCEPT : m_handle(other.m_handle)
        {
            std::memset(&other.m_handle, 0, sizeof(bser_writer_t));
        }

        BinaryWriter& operator=(BinaryWriter&& other) BSER_NOEXCEPT
        {
            if (this != &other)
            {
                bser_writer_deinit(&m_handle);
                m_handle = other.m_handle;
                std::memset(&other.m_handle, 0, sizeof(bser_writer_t));
            }
            return *this;
        }

        ~BinaryWriter() BSER_NOEXCEPT
        {
            bser_writer_deinit(&m_handle);
        }

        bool close() BSER_NOEXCEPT
        {
            return bser_writer_close(&m_handle);
        }

        bool execute() BSER_NOEXCEPT
        {
            return bser_writer_execute(&m_handle);
        }

        BSER_NODISCARD bool has_completed() const BSER_NOEXCEPT
        {
            return m_handle.has_completed;
        }

        BSER_NODISCARD const bser_writer_t* native_handle() const BSER_NOEXCEPT
        {
            return &m_handle;
        }

    private:
        void init(const wchar_t* path, const std::vector<Record>& records)
        {
            if (!path)
            {
                throw std::invalid_argument("File path cannot be null");
            }

            std::vector<bser_record_t> native_records;
            native_records.reserve(records.size());

            for (const auto& rec : records)
            {
                native_records.push_back(*rec.native_handle());
            }

            if (!bser_writer_init(&m_handle, path, native_records.data(), native_records.size()))
            {
                throw std::runtime_error("Failed to initialize binary writer");
            }
        }

        bser_writer_t m_handle;
    };

    class BinaryStream
    {
    public:
        explicit BinaryStream(std::wstring path)
            : m_path(std::move(path))
        {
        }

        explicit BinaryStream(const wchar_t* path)
            : m_path(path ? path : L"")
        {
        }

        void push_back(const Record& record)
        {
            m_records.push_back(record);
        }

        void push_back(Record&& record)
        {
            m_records.push_back(std::move(record));
        }

        void clear() BSER_NOEXCEPT
        {
            m_records.clear();
        }

        bool read(const SchemaCatalog& catalog)
        {
            BinaryReader reader(m_path.c_str(), catalog);

            if (!reader.execute())
            {
                throw std::runtime_error("An error occurred when reading binary stream");
            }

            m_records = reader.records();
            return reader.has_completed();
        }

        bool write()
        {
            BinaryWriter writer(m_path.c_str(), m_records);

            if (!writer.execute())
            {
                throw std::runtime_error("An error occurred when writing binary stream");
            }

            return writer.has_completed();
        }

        BSER_NODISCARD const std::vector<Record>& records() const BSER_NOEXCEPT
        {
            return m_records;
        }

        BSER_NODISCARD std::vector<Record>& records() BSER_NOEXCEPT
        {
            return m_records;
        }

        BSER_NODISCARD const std::wstring& path() const BSER_NOEXCEPT
        {
            return m_path;
        }

    private:
        std::wstring m_path;
        std::vector<Record> m_records;
    };

    template <int size>
    struct SerializableString {
        char data[size]{};

        SerializableString() = default;

        SerializableString(const char* src) {
            strncpy_s(data, src, sizeof(data));
            data[sizeof(data) - 1] = '\0';
        }

        SerializableString& operator=(const char* src) {
            strncpy_s(data, src, sizeof(data));
            data[sizeof(data) - 1] = '\0';
            return *this;
        }

        operator const char* () const { return data; }
    };

    namespace detail
    {
        inline std::string clean_field_name(const std::string& name)
        {
            size_t dot_pos = name.rfind('.');
            if (dot_pos != std::string::npos)
            {
                return name.substr(dot_pos + 1);
            }
            size_t arrow_pos = name.rfind("->");
            if (arrow_pos != std::string::npos)
            {
                return name.substr(arrow_pos + 2);
            }
            return name;
        }

        inline std::vector<std::string> parse_field_names(const char* names_str)
        {
            std::vector<std::string> fields;
            if (!names_str) return fields;

            const char* p = names_str;
            while (*p)
            {
                while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
                const char* start = p;
                while (*p && *p != ',') ++p;
                const char* end = p;
                while (end > start && std::isspace(static_cast<unsigned char>(*(end - 1)))) --end;

                if (end > start)
                {
                    std::string raw_name(start, end - start);
                    fields.push_back(clean_field_name(raw_name));
                }
                if (*p == ',') ++p;
            }
            return fields;
        }

        inline std::vector<const char*> to_char_ptrs(const std::vector<std::string>& strs)
        {
            std::vector<const char*> ptrs;
            ptrs.reserve(strs.size());
            for (const auto& s : strs)
            {
                ptrs.push_back(s.c_str());
            }
            return ptrs;
        }

        inline void set_field_at_idx(Record&, size_t&) {}

        template <typename Head, typename... Tail>
        inline void set_field_at_idx(Record& rec, size_t& idx, const Head& head, const Tail&... tail)
        {
            rec.set(idx++, head);
            set_field_at_idx(rec, idx, tail...);
        }

        inline void get_field_at_idx(const Record&, size_t&) {}

        template <typename Head, typename... Tail>
        inline void get_field_at_idx(const Record& rec, size_t& idx, Head& head, Tail&... tail)
        {
            using MemType = typename std::decay<Head>::type;
            head = rec.get<MemType>(idx++);
            get_field_at_idx(rec, idx, tail...);
        }
    }

    template <typename T>
    inline Record& set_object(Record& rec, const char* field_name, const T& obj)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type T must be trivially copyable to serialize directly.");
        return rec.set(field_name, reinterpret_cast<const bser_byte_t*>(&obj), sizeof(T));
    }

    template <typename T>
    inline T get_object(const Record& rec, const char* field_name)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type T must be trivially copyable to deserialize directly.");
        return rec.get<T>(field_name);
    }

    template <typename T>
    struct StructTraits;
}

#define BSER_STRUCT(Type, ...)                                                  \
namespace bser                                                                  \
{                                                                               \
    template <>                                                                 \
    struct StructTraits<Type> {                                                 \
        static Record to_record(bser_id_t id, const Type& obj) {                \
            auto field_strs = ::bser::detail::parse_field_names(#__VA_ARGS__);  \
            auto field_ptrs = ::bser::detail::to_char_ptrs(field_strs);         \
            Record rec(id, field_ptrs.data(), field_ptrs.size());               \
            apply_to_record(rec, obj);                                          \
            return rec;                                                         \
        }                                                                       \
        static Type from_record(const Record& rec) {                            \
            Type obj{};                                                         \
            apply_from_record(rec, obj);                                        \
            return obj;                                                         \
        }                                                                       \
    private:                                                                    \
        static void apply_to_record(Record& rec, const Type& obj) {             \
            size_t idx = 0;                                                     \
            ::bser::detail::set_field_at_idx(rec, idx, __VA_ARGS__);            \
        }                                                                       \
        static void apply_from_record(const Record& rec, Type& obj) {           \
            size_t idx = 0;                                                     \
            ::bser::detail::get_field_at_idx(rec, idx, __VA_ARGS__);            \
        }                                                                       \
    };                                                                          \
}

#endif /* BSER_HPP */