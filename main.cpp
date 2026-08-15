// Example program

#include <iostream>
#include <bser>

struct Coordinate
{
	int x;
	int y;
};

BSER_STRUCT(Coordinate, obj.x, obj.y)

enum SchemaID : bser_id_t {
	ID_COORDINATE = 101
};

int main()
{
	const wchar_t* output_file = L"test.bser";

	bser::Record recordA = bser::StructTraits<Coordinate>::to_record(ID_COORDINATE, { 10, 57 });

	bser::BinaryWriter writer(output_file, { recordA });

	if (!writer.execute())
	{
		throw std::runtime_error("Couldn't write to file");
	}
	
	bser::SchemaCatalog catalog(
		{
			bser::StructTraits<Coordinate>::to_record(ID_COORDINATE, { })
		});
	bser::BinaryReader reader(output_file, catalog);

	if (!reader.execute())
	{
		throw std::runtime_error("Couldn't read from file");
	}

	auto records = reader.records();

	for (bser::Record& record : records)
	{
		std::cout << "X: " << record.get<int>("x") << ", Y: " << record.get<int>("y");
	}

	return 0;
}