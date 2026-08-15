# BSER project

*Save C or C++ objects in binary files, in which languages you can also load the same objects using the same binary files.*

## Abstract

This project consists of a C standard and C++ standard, supporting both C++11 and C. Using records and schemas, you can build objects and catalogs. Through a catalog, you can specify different integral record identifiers with different fields. This lets different object types with different fields coincide through the same binary file. Each field can be of any size, with a macro defining the maximum size. To change the maximum binary field size you can explicitly define the macro called `BSER_MAX_FIELD_BYTES` either before you include the header or after you include the header at the top of the file.

There are also other configurable macros that you can define:

| Macro                        | Description                                                    | Default |
|------------------------------|----------------------------------------------------------------|---------|
| `BSER_MAX_SCHEMAS`           | The maximum amount of schemas that there can be in one catalog | 32      |
| `BSER_MAX_FIELD_NAME_LENGTH` | The maximum string length of the name of one field             | 32      |
| `BSER_MAX_FIELDS`            | The maximum number of fields in one record                     | 32      |
| `BSER_MAX_READ_BATCH`        | The maximum number of records read through one reader          | 64      |
| `BSER_MAX_WRITE_BATCH`       | The maximum number of records written through one writer       | 64      |
| `BSER_MAX_FIELD_BYTES`       | The maximum number of bytes the value of a field can have      | 128     |

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

To access the results of the reader, you can access the `records` and `record_count` field for all of the records and an integral number for how many records that was read.

The writer must be initialized using the `bser_writer_init` function. You can also deinitialize it using the `bser_writer_deinit`, close the file using `bser_writer_close`, and execute the writer using the `bser_writer_execute` function.

## The C++ version

The C++ version is a wrapper over the C version primitives, reducing code boilerplate and exposure. Every C struct has a C++ correspondence in the C++ `bser` namespace. Initialization and deinitialization are handled automatically through the constructor and deconstructor of each object. For specialized behaviour, you can also use the `BSER_STRUCT` macro function to facilitate the struct serialization and deserialization.

After calling the `BSER_STRUCT` with a struct and specifying every field you want to facilitate, there will be a static `to_record` and `from_record` function on the `bser::StructTraits<Type>` template class. The first parameter of the `to_record` function will be the numerical identifier of the record, and the second parameter will be an instance of the struct itself. In the `from_record` function, you can supply the record object itself for the struct instance.

## License

Copyright 2026 Erik-Neo Östlund-Zetterberg

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.