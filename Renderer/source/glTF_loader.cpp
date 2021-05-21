#include "glTF_loader.h"
#include "nlohmann/json.h"

#include <fstream>
#include <cstdint>
#include <vector>

#include "gfx_model.h"


namespace glTF_L
{
	enum ComponentType
	{
		BYTE = 0,
		UNSIGNED_BYTE,
		SHORT,
		UNSIGNED_SHORT,
		UNSIGNED_INT,
		FLOAT,
		COMPONENT_TYPE_COUNT
	};

	const char COMPONENT_TYPE_SIZES[] = { 1, 1, 2, 2, 4, 4 };
	const int COMPONENT_TYPE_ID[] = { 5120, 5121, 5122, 5123, 5125, 5126 };

	ComponentType ComponentTypeIdToEnum( const int id )
	{
		for( char type = 0; type != COMPONENT_TYPE_COUNT; type++ )
		{
			if( id == COMPONENT_TYPE_ID[type] )
				return static_cast< ComponentType >(type);
		}
		return COMPONENT_TYPE_COUNT;
	}

	enum Type : char
	{
		SCALAR = 0,
		VEC2,
		VEC3,
		VEC4,
		MAT2,
		MAT3,
		MAT4,
		TYPE_COUNT
	};

	const std::string TYPES_S[] = { "SCALAR", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3", "MAT4" };
	const char TYPE_ELEMENT_COUNTS[] = { 1, 2, 3, 4, 4, 9, 16 };

	Type strToTypeEnum( const std::string& str )
	{
		for( char type = 0; type != TYPE_COUNT; type++ )
		{
			if( str == TYPES_S[type] )
				return static_cast< Type >(type);
		}
		return TYPE_COUNT;
	}

	struct Attributes
	{
		int position;
		int normal;
		int tangent;
		int texcoord_0;
	};

	void from_json( const nlohmann::json& j, Attributes& a )
	{
		a = {
			j["POSITION"].get<int>(),
			j["NORMAL"].get<int>(),
			j["TANGENT"].get<int>(),
			j["TEXCOORD_0"].get<int>()
		};
	}

	struct Primitive
	{
		Attributes attributes;
		int indices;
	};

	void from_json( const nlohmann::json& j, Primitive& p )
	{
		p = {
			j["attributes"].get<Attributes>(),
			j["indices"].get<int>()
		};
	}

	struct Mesh
	{
		std::string name;
		std::vector<Primitive> primitives;
	};

	void from_json( const nlohmann::json& j, Mesh& m )
	{
		m = {
			j["name"].get<std::string>(),
			j["primitives"].get<std::vector<Primitive>>()
		};
	}

	struct Accessor
	{
		int bufferView;
		//int byteOffset;
		ComponentType componentType;
		int count;
		Type type;
	};

	void from_json( const nlohmann::json& j, Accessor& a )
	{
		a = {
			j["bufferView"].get<int>(),
			static_cast< ComponentType >(ComponentTypeIdToEnum( j["componentType"].get<int>() )),
			j["count"].get<int>(),
			strToTypeEnum( j["type"].get<std::string>() )
		};
	}

	struct BufferView
	{
		int buffer;
		int byteLength;
		int byteOffset;
	};

	void from_json( const nlohmann::json& j, BufferView& b )
	{
		b = {
			j["buffer"].get<int>(),
			j["byteLength"].get<int>(),
			j["byteOffset"].get<int>()
		};
	}

	struct Buffer
	{
		int byteLength;
	};

	void from_json( const nlohmann::json& j, Buffer& b )
	{
		b = { j["byteLength"].get<int>() };
	}

	template< class T >
	static T GetType( const char* ptr, size_t index, size_t elementIndex, Type type, ComponentType componentType )
	{
		return *reinterpret_cast< const T* >(&ptr[(index * TYPE_ELEMENT_COUNTS[type] + elementIndex) * COMPONENT_TYPE_SIZES[componentType]]);
	};

	void LoadCollisionData( const char* fileName, std::vector<glm::vec3>* vertices, std::vector<uint32_t>* indices )
	{
		std::fstream fs( fileName, std::fstream::in | std::fstream::binary );

		//Parse header
		uint32_t header[3];
		fs.read( reinterpret_cast< char* >(header), sizeof( header ) );

		//Parse JSON
		uint32_t jsonHeader[2];
		fs.read( reinterpret_cast< char* >(jsonHeader), sizeof( jsonHeader ) );

		const uint32_t jsonChunkSize = jsonHeader[0];
		std::vector<char> jsonChunk;
		jsonChunk.resize( jsonChunkSize );
		fs.read( jsonChunk.data(), jsonChunkSize );

		nlohmann::json j = nlohmann::json::parse( jsonChunk.begin(), jsonChunk.end() );

		std::vector<Accessor> accessors = j["accessors"].get<std::vector<Accessor>>();
		std::vector<BufferView> bufferViews = j["bufferViews"].get<std::vector<BufferView>>();
		std::vector<Buffer> buffers = j["buffers"].get<std::vector<Buffer>>();

		int meshIndex = j["nodes"][0]["mesh"].get<int>();
		Mesh mesh = j["meshes"][meshIndex].get<Mesh>();

		//read the buffer
		uint32_t bufferHeader[2];
		fs.read( reinterpret_cast< char* >(bufferHeader), sizeof( bufferHeader ) );
		size_t bufferFileOffset = fs.tellg();

		std::vector<char> bufferChunk;
		const uint32_t bufferChunkSize = bufferHeader[0];
		bufferChunk.resize( bufferChunkSize );
		fs.read( bufferChunk.data(), bufferChunkSize );

		assert( buffers.size() == 1 );

		int positionsIndex = mesh.primitives[0].attributes.position;
		assert( accessors[positionsIndex].componentType == FLOAT );
		assert( accessors[positionsIndex].type == VEC3 );
		const char * positions = &bufferChunk[bufferViews[positionsIndex].byteOffset];

		int indexesIndex = mesh.primitives[0].indices;
		assert( accessors[indexesIndex].componentType == UNSIGNED_SHORT );
		assert( accessors[indexesIndex].type == SCALAR );
		const char* indexes = &bufferChunk[bufferViews[indexesIndex].byteOffset];

		size_t vertexCount = accessors[positionsIndex].count;

		vertices->resize( vertexCount );

		for( size_t i = 0; i < vertexCount; ++i )
		{
			(*vertices)[i].x = GetType<float>( positions, i, 0, VEC3, FLOAT );
			(*vertices)[i].y = GetType<float>( positions, i, 1, VEC3, FLOAT );
			(*vertices)[i].z = GetType<float>( positions, i, 2, VEC3, FLOAT ) *-1.0f;//TODO: glTF forced to right handed with Z backward
		}

		size_t indexCount = accessors[indexesIndex].count;
		indices->resize( indexCount );
		for( size_t i = 0; i < indexCount; ++i )
		{
			(*indices)[i] = GetType<unsigned short>( indexes, i, 0, SCALAR, UNSIGNED_SHORT );
		}
	}

	void LoadMesh( const char* fileName, GfxModel* model, I_BufferAllocator* allocator )
	{
		std::fstream fs( fileName, std::fstream::in | std::fstream::binary );

		//Parse header
		uint32_t header[3];
		fs.read( reinterpret_cast< char* >(header), sizeof( header ) );

		//Parse JSON
		uint32_t jsonHeader[2];
		fs.read( reinterpret_cast< char* >(jsonHeader), sizeof( jsonHeader ) );

		const uint32_t jsonChunkSize = jsonHeader[0];
		std::vector<char> jsonChunk;
		jsonChunk.resize( jsonChunkSize );
		fs.read( jsonChunk.data(), jsonChunkSize );

		nlohmann::json j = nlohmann::json::parse( jsonChunk.begin(), jsonChunk.end() );

		std::vector<Accessor> accessors = j["accessors"].get<std::vector<Accessor>>();
		std::vector<BufferView> bufferViews = j["bufferViews"].get<std::vector<BufferView>>();
		std::vector<Buffer> buffers = j["buffers"].get<std::vector<Buffer>>();

		int meshIndex = j["nodes"][0]["mesh"].get<int>();
		Mesh mesh = j["meshes"][meshIndex].get<Mesh>();

		//read the buffer
		uint32_t bufferHeader[2];
		fs.read( reinterpret_cast< char* >(bufferHeader), sizeof( bufferHeader ) );
		size_t bufferFileOffset = fs.tellg();

		std::vector<char> bufferChunk;
		const uint32_t bufferChunkSize = bufferHeader[0];
		bufferChunk.resize( bufferChunkSize );
		fs.read( bufferChunk.data(), bufferChunkSize );

		assert( buffers.size() == 1 );

		int positionsIndex = mesh.primitives[0].attributes.position;
		assert( accessors[positionsIndex].componentType == FLOAT );
		assert( accessors[positionsIndex].type == VEC3 );
		const char * positions = &bufferChunk[bufferViews[positionsIndex].byteOffset];

		int normalsIndex = mesh.primitives[0].attributes.normal;
		assert( accessors[normalsIndex].componentType == FLOAT );
		assert( accessors[normalsIndex].type == VEC3 );
		const char * normals = &bufferChunk[bufferViews[normalsIndex].byteOffset];

		int tangentsIndex = mesh.primitives[0].attributes.tangent;
		assert( accessors[tangentsIndex].componentType == FLOAT );
		assert( accessors[tangentsIndex].type == VEC4 );
		const char * tangents = &bufferChunk[bufferViews[tangentsIndex].byteOffset];

		int texcoordsIndex = mesh.primitives[0].attributes.texcoord_0;
		assert( accessors[texcoordsIndex].componentType == FLOAT );
		assert( accessors[texcoordsIndex].type == VEC2 );
		const char * texcoords = &bufferChunk[bufferViews[texcoordsIndex].byteOffset];

		int indexesIndex = mesh.primitives[0].indices;
		assert( accessors[indexesIndex].componentType == UNSIGNED_SHORT );
		assert( accessors[indexesIndex].type == SCALAR );
		const char* indexes = &bufferChunk[bufferViews[indexesIndex].byteOffset];

		size_t vertexCount = accessors[positionsIndex].count;

		std::vector<glm::vec3> vertPos, vertNormals, vertTangents, vertColor;
		std::vector<glm::vec2> vertTexCoord;
		vertPos.resize( vertexCount );
		vertNormals.resize( vertexCount );
		vertTangents.resize( vertexCount );
		vertColor.resize( vertexCount );
		vertTexCoord.resize( vertexCount );

		for( size_t i = 0; i < vertexCount; ++i )
		{
			vertPos[i].x = GetType<float>( positions, i, 0, VEC3, FLOAT );
			vertPos[i].y = GetType<float>( positions, i, 1, VEC3, FLOAT );
			vertPos[i].z = GetType<float>( positions, i, 2, VEC3, FLOAT ) *-1.0f;//TODO: glTF forced to right handed with Z backward

			vertNormals[i].x = GetType<float>( normals, i, 0, VEC3, FLOAT );
			vertNormals[i].y = GetType<float>( normals, i, 1, VEC3, FLOAT );
			vertNormals[i].z = GetType<float>( normals, i, 2, VEC3, FLOAT );

			vertTangents[i].x = GetType<float>( tangents, i, 0, VEC4, FLOAT );
			vertTangents[i].y = GetType<float>( tangents, i, 1, VEC4, FLOAT );
			vertTangents[i].z = GetType<float>( tangents, i, 2, VEC4, FLOAT );

			vertTexCoord[i].x = GetType<float>( texcoords, i, 0, VEC2, FLOAT );
			vertTexCoord[i].y = GetType<float>( texcoords, i, 1, VEC2, FLOAT );
		}

		size_t indexCount = accessors[indexesIndex].count;
		std::vector<uint32_t> indexes_32;
		indexes_32.resize( indexCount );
		for( size_t i = 0; i < indexCount; ++i )
		{
			indexes_32[i] = GetType<unsigned short>( indexes, i, 0, SCALAR, UNSIGNED_SHORT );
		}

		std::vector<VIDesc> modelVIDescs = {
			{ ( VIDataType )eVIDataType::POSITION, eVIDataElementType::FLOAT, 3 },
			{ ( VIDataType )eVIDataType::NORMAL, eVIDataElementType::FLOAT, 3 },
			{ ( VIDataType )eVIDataType::TANGENT, eVIDataElementType::FLOAT, 3 },
			{ ( VIDataType )eVIDataType::COLOR, eVIDataElementType::FLOAT, 3 },
			{ ( VIDataType )eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2 },
		};
		std::vector<void*> modelData = {
			vertPos.data(),
			vertNormals.data(),
			vertTangents.data(),
			vertColor.data(),
			vertTexCoord.data(),
		};

		*model = CreateGfxModel( modelVIDescs, modelData, vertexCount, indexes_32.data(), indexCount, sizeof(uint32_t), allocator );
	}
}