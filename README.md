# BSER: Binary Serialization for C and C++11

*Save C or C++ objects in binary files, in which languages you can also load the same objects using the same binary files.*

| Trait | Rank | Reasoning |
|-------|------|-----------|
| **speed** | 🥇 | Top 1-2% (only Cap'n Proto faster) |
| **Predictability** | 🥇 | Absolute best in class |
| **Ease of Use** | 🥈 | Only JSON simpler (universal support) |
| **File Size** | 🥉 | 80% of protobuf (but 2-3x vs JSON) |
| **Safety** | 🥈 | Good, with fixed struct guarantees |
| **Flexibility** | 🥉 | Limited (no nested structures) |
    Remember that the above scores are only assessments by myself.

## Abstract

This project consists of a C and C++ standard, supporting both C++11 and C. In this library, you can use records and schemas to build objects and catalogs. Through a catalog, you can specify different integral record identifiers with different fields, composed into schemas/records.

This lets different object types with different fields and field sizes coincide through the same binary file. There is a macro defining the maximum size a field can take. To change the maximum binary field size you can explicitly define the macro called `BSER_MAX_FIELD_BYTES` either before you include the header or after you include the header at the top of the file or through your compilation method.

There are also other configurable macros that you can define:

| Macro                        | Description                                                    | Default |
|------------------------------|----------------------------------------------------------------|---------|
| `BSER_MAX_SCHEMAS`           | The maximum amount of schemas that there can be in one catalog | 32      |
| `BSER_MAX_FIELD_NAME_LENGTH` | The maximum length of a string name representation of a field  | 32      |
| `BSER_MAX_FIELDS`            | The maximum number of fields in one record                     | 32      |
| `BSER_MAX_READ_BATCH`        | The maximum number of records read through one reader          | 64      |
| `BSER_MAX_WRITE_BATCH`       | The maximum number of records written through one writer       | 64      |
| `BSER_MAX_FIELD_BYTES`       | The maximum number of bytes the value of a field can have      | 128     |

It is important to note that the the endianness may be differently structured because the program uses `memcpy` of native integer types (no explicit byte-order conversion). That means the file stores integers in the host machine's endiannes (almost always little-endian on x86). If you move the file between different-endian machines you must handle byte-ordering.

Every field will consist of the same amount of bytes as the value of the `BSER_MAX_FIELD_BYTES` macro.

## Details

Bser stands for "Binary Serialization." It is especially useful when you need a compact, fast, low-overhead binary record format in C or C++ that maps directly to plain-old-data (POD) structs, supports streaming I/O, and has a tiny C API (no codegen).

Choose bser when throughput, predictability and minimal runtime cost matter more than human-redability, dynamic typing, or rich schema evolution features.

### Why and when to use bser

- High-throughput telemetry / logging: bser stores fixed-size binary fields and writes/reads records efficiently. Useful for logging many small records (sensor samples, traces) where serialization/deserialization overhead must be tiny.

- IPC and netword protocols with known record shapes: if both endpoints share C/C++ code and agree on struct layouts, bser lets you send/receive records with minimal packing/unpacking.

- On-disk compact storage for fixed schema records: compact binary representation and schema/catalog support make bser good for append-only stores, caches, or compact databases where you mostly read/write fixed-width records.

- Embedded systems and low-dependency environments: bser is a small C library with a thin C++ wrapper. No codegen, no heavy runtime, and easy to compile into constrained toolchains.

- Zero/low-allocation streaming: the reader/writer and stream types in the header have orientation toward I/O (open/execute/close), allowing low-allocation processing of sequences of records.

- Easy C / C++ interoperability: bser exposes a C API and the header provides RAII C++ wrappers, so mixed-language projects benefit.

### How bser compares to other formats / libraries

- JSON
  ----
  - Pros: much smaller on-disk/network size, far faster to parse/serialize, no textual formatting overhead.
  - Cons: not human readable, and not convenient for ad-hoc intspection or editing.
  - Use bser when machine throughput and size matter; use JSON when human readability and ubiquity matter.

- MessagePack / BSON / CBOR
  -------------------------
  - Similar in being binary and efficient. It is different through the following points.
  - bser is record/schema-oriented and designed for direct mapping to C structs; MessagePack/CBOR are more generic, dynamically typed, and often require more parsing logic top map into fixed C structs.
  - MessagePack/CBOR have widespread ecosystem support accross languages; bser's strength is the tiny C API and C/C++ convenience when you control both ends.

- Protocol Buffers / FlatBuffers / Cap'n Proto
  --------------------------------------------
  - Protobuf provides strong schema, versioning, compact wire format, auto code-generation across languages—excellent for long-term, cross-language APIs.
  - FlatBuffers/Cap'n Proto give zero-copy access and very fast random access.
  - bser's advantages over them: no schema language or codegen step (you can build schemas at runtime or embed schema records), simpler build/embedding, and straightforward memory copies for POD types. If you need cross-language versioning, comples feature evolution, or language-neutral RPC, Protobuf/FlatBuffers are usually better.

- hand-rolled binary formats
  --------------------------
  - bser provides a small, consistent API and helpers (schemas, catalogs, reader/writer) so you avoid ad-hoc bugs and inconsistent layouts that plague homebrew formats.

### Practical trade-offs and limitations

- Requires agreement on binary layout and POD types: the provided C++ wrapper focuses on trivially-copyable types. It is not a full object serializer (no automatic deep serialization of `std::string` or containers)

- Endianness and ABI portability: direct memory copies of POD types are fast but must be managed across heterogeneous architectures (endianness, alignment, different type sizes).

- Limited built-in versioning: unlike Protobuf, bser doesn't provide a rich schema evolution system; you must manage compatibility manually.

- Field size constraint: the header uses a fixed `BSER_MAX_FIELD_BYTES` for field storage; large or variable-length fields may require custom handling or chunking if it exceeds this field.

- Defining the `BSER_MAX_FIELD_BYTES` macro as larger than the default of 128 will effect the file size. Therefore it is worth considering chunking or custom handling of large types that exceeds this field in size.

- Platform conventions: this header uses `wchar_t` paths for streams, which is platform-spcific (Windows-oriented)—be mindful if porting to POSIX-only systems. We are still figuring out if we should enable more choices in the future.

### Concrete schenarios where bser is the right choice

- Recording millions of fixed-layout entries (e.g. telemetry, time-series) where writing and later sequential reading must be cheap.

- High-frequencey IPC between processes written in C/C++ where you can rely on matching struct layouts and want minimal runtime overhead.

- Small-footprint applications and embedded devices where avoiding heavy runtimes and codegen is critical.

- Tools that need deterministic binary snapshots of memory layouts or simple persistent cahces of structs.

### When another solution is likely better

- If you need human-friendly interchange (use JSON/YAML).

- If you need multi-language, long-lived public APIs with schema evolution and tooling (use ProtoBuf/Avro/Thrift).

- If you need automatic variable-length container/string handling with broad language libraries (use MessagePach/CBOR).

## The C version

### The C-header file

In the include directory, the [`bser.h`](include/bser.h) file implements all of the C features. There are type definitions for a byte, record identifier, and numerical size. There are also binary packing, through the macros `BSER_PACK` and `BSER_UNPACK`.

Using a C file object in different bser functions, you can read and write bytes, records, identifiers, and field sizes directly to the file. Using the `bser_record` struct, you can define a record with an identifier, field names, field count, and field values. You must initialize the record first using the `bser_record_init` function.

After you have initialized the record, you can set field values using their field index or name. This can be done through the `bser_record_set_field_at` or `bser_record_set_field` functions, accompanied with their complimenting functions—for getting the value of a field—through the `bser_record_get_field_at` and `bser_record_get_field` functions.

### Input/output structs

There is a `bser_stream` struct, assessing which file you are modifying, common file operations, and the C file object itself. Once you create a `bser_stream` struct, you must initialize it using the `bser_stream_open` function, that returns true upon success and false otherwise. Then, to close the stream, you can call the `bser_stream_close` function.

The `bser_stream` struct is essentially the bser version of a file object or input/output file-stream. It is used during both reading and writing using the following two structs: `bser_reader` and `bser_writer`.

To use the `bser_reader` struct, you have to specify which objects a file can consist of and which fields should be associated with which field name. The field names must be in the exact same order to preserve the field index that is required for loading and saving to files. You don't have to supply the records with a value since the records themselves use indexes to bind field names with their corresponding values.

To create a reader, you must first construct a catalog. In the catalog, there will be record-schemes that tell your reader what data you want and how you want to structure it. The catalog is a struct called `bser_catalog`, in which you must fill the `schemas` field with schemas and preserve the `schema_count` field with how many schemas there are in the catalog in total.

After creating an instance of the `bser_reader` struct, you must initialize it using the `bser_reader_init` function which will take a catalog for reading the records in a `path` wide character pointer parameter. When you are done with the reader, you must call the `bser_reader_close` to close the file or `bser_reader_deinit` to deinitialize it.

With a reader, you can read all of the records in the file using the `bser_reader_execute` function. This function will return a boolean value, that is true when successful and false otherwise. The `has_completed` field will become true if successful and you will not be able to read the object again using the same reader. The reader is only supposed to be used once.

To access the results of the reader, you can access the `records` and `record_count` fields for all of the records and an integral number for how many records that was read.

The writer must be initialized using the `bser_writer_init` function. You can also deinitialize it using the `bser_writer_deinit`, close the file using `bser_writer_close`, and execute the writer using the `bser_writer_execute` function.

## The C++ version

The C++ version is a wrapper over the C version primitives, reducing code boilerplate and exposure. Every C struct has a C++ correspondence in the C++ `bser` namespace. We must highlight here that the minimum version that this RAII convenience layer can support for the small C library is at C++11. It is implemented in the [`bser.hpp`](include/bser.hpp) file under the include directory.

Initialization and deinitialization are handled automatically through the constructor and deconstructor of each object. For specialized behavior, you can also use the `BSER_STRUCT` macro function to facilitate the struct serialization and deserialization.

After calling the `BSER_STRUCT` with a struct and specifying every field you want to facilitate, there will be a static `to_record` and `from_record` function on the `bser::StructTraits<Type>` template class.

The first parameter of the `to_record` function will be the numerical identifier of the record, and the second parameter will be an instance of the struct itself. In the `from_record` function, you can supply the record object itself for the struct instance.

### C++ Summary

A thin, practical, exception-based C++ wrapper that provides RAII, typed get/set for trivially-copyable fields, schema/catalog helpers, and a macro to auto-generate struct↔record mapping. Good for binary layouts and simple POD-style structs, but not for complex/variable-length fields or non-trivially-copyable members.

## License

MIT License

Copyright 2026 Erik-Neo Östlund-Zetterberg

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.