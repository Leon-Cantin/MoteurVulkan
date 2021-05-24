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
	constexpr int INVALID_INT = INT_MIN;

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

	template< typename T >
	T GetDefaultIfNull( const nlohmann::json& j, const char* object_name, const T& default_value )
	{
		auto it = j.find( object_name );
		return it == j.end() ? default_value : *it;
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
		std::string uri;
	};

	void from_json( const nlohmann::json& j, Buffer& b )
	{
		b = { j["byteLength"].get<int>(),
			GetDefaultIfNull<std::string>( j, "uri", "" )
		};
	}

	struct Vec3
	{
		float x;
		float y;
		float z;
	};

	void from_json( const nlohmann::json& j, Vec3& v )
	{
		v = {
			j[0].get<float>(),
			j[1].get<float>(),
			j[2].get<float>()
		};
	}

	struct Vec4
	{
		float w;
		float x;
		float y;
		float z;
	};

	void from_json( const nlohmann::json& j, Vec4& v )
	{
		v = {
			j[0].get<float>(),
			j[1].get<float>(),
			j[2].get<float>(),
			j[3].get<float>()
		};
	}

	struct Node
	{
		int meshIndex;
		std::string name;
		//extras
		Vec3 translation;
		Vec3 scale;
		Vec4 rotation;
	};

	void from_json( const nlohmann::json& j, Node& n )
	{
		n = { GetDefaultIfNull<int>( j, "mesh", INVALID_INT ),
			GetDefaultIfNull<std::string>( j, "name", "" ),
			GetDefaultIfNull<Vec3>( j, "translation", { 0.0f, 0.0f, 0.0f } ),
			GetDefaultIfNull<Vec3>( j, "scale", { 1.0f, 1.0f, 1.0f } ),
			GetDefaultIfNull<Vec4>( j, "rotation", { 1.0f, 0.0f, 0.0f, 0.0f } )
		};
	}

	template< class T >
	static T GetType( const byte* ptr, size_t index, size_t elementIndex, Type type, ComponentType componentType )
	{
		return *reinterpret_cast< const T* >(&ptr[(index * TYPE_ELEMENT_COUNTS[type] + elementIndex) * COMPONENT_TYPE_SIZES[componentType]]);
	};

	struct glTF_Json
	{
		std::vector<Accessor> accessors;
		std::vector<BufferView> bufferViews;
		std::vector<Buffer> buffers;
		std::vector<Node> nodes;
		std::vector<Mesh> meshes;
		std::vector<byte> data;
	};

	static glTF_Json ReadJson( const char* fileName )
	{
		const bool is_gltf = strstr( fileName, ".gltf" ) != nullptr;
		const bool is_glb = strstr( fileName, ".glb" ) != nullptr;

		assert( is_glb || is_gltf );

		std::fstream fs( fileName, std::fstream::in | std::fstream::binary );

		nlohmann::json j;
		if( is_glb )
		{
			//Parse header
			uint32_t header[3];
			fs.read( reinterpret_cast< char* >(header), sizeof( header ) );

			//Parse JSON
			uint32_t jsonHeader[2];
			fs.read( reinterpret_cast< char* >(jsonHeader), sizeof( jsonHeader ) );

			const uint32_t jsonChunkSize = jsonHeader[0];
			std::vector<byte> jsonChunk;
			jsonChunk.resize( jsonChunkSize );
			fs.read( reinterpret_cast< char* >(jsonChunk.data()), jsonChunkSize );

			j = nlohmann::json::parse( jsonChunk.begin(), jsonChunk.end() );
		}
		else if( is_gltf )
		{
			constexpr size_t max_byte_count = 1 * 1024 * 1024;
			std::vector<byte> jsonChunk;
			jsonChunk.resize( max_byte_count );
			fs.read( reinterpret_cast< char* >(jsonChunk.data()), jsonChunk.size() );
			assert( fs.gcount() < max_byte_count );

			j = nlohmann::json::parse( jsonChunk.begin(), jsonChunk.begin() + jsonChunk.size() );
		}

		const std::vector<Accessor> accessors = j["accessors"].get<std::vector<Accessor>>();
		const std::vector<BufferView> bufferViews = j["bufferViews"].get<std::vector<BufferView>>();
		const std::vector<Buffer> buffers = j["buffers"].get<std::vector<Buffer>>();
		const std::vector<Node> nodes = j["nodes"].get<std::vector<Node>>();
		const std::vector<Mesh> meshes = j["meshes"].get<std::vector<Mesh>>();

		assert( buffers.size() == 1 );

		std::vector<byte> bufferChunk;
		if( is_glb )
		{
			//read the buffer
			uint32_t bufferHeader[2];
			fs.read( reinterpret_cast< char* >(bufferHeader), sizeof( bufferHeader ) );
			size_t bufferFileOffset = fs.tellg();

			const uint32_t bufferChunkSize = bufferHeader[0];
			bufferChunk.resize( bufferChunkSize );
			fs.read( reinterpret_cast< char* >(bufferChunk.data()), bufferChunkSize );
		}
		else if( is_gltf )
		{
			const std::string& bufferFileName = buffers[0].uri;
			const char* ptrFilePathEnd = strrchr( fileName, '/' );
			std::string bufferFilePath = std::string( fileName, ptrFilePathEnd - fileName );
			bufferFilePath += "/" + bufferFileName;

			constexpr size_t max_byte_count = 1 * 1024 * 1024;
			bufferChunk.resize( max_byte_count );

			std::fstream buffer_fs( bufferFilePath, std::fstream::in | std::fstream::binary );
			buffer_fs.read( reinterpret_cast< char* >(bufferChunk.data()), bufferChunk.size() );
			assert( buffer_fs.gcount() < max_byte_count );
		}

		return { accessors,
			bufferViews,
			buffers,
			nodes,
			meshes,
			bufferChunk };
	}

	static GfxModel LoadMesh( const Mesh& mesh, const std::vector<Accessor>& accessors, const std::vector<BufferView>& buffer_views, const byte* data, I_BufferAllocator* allocator )
	{
		assert( mesh.primitives.size() == 1 );

		int positionsIndex = mesh.primitives[0].attributes.position;
		assert( accessors[positionsIndex].componentType == FLOAT );
		assert( accessors[positionsIndex].type == VEC3 );
		const byte * positions = &data[buffer_views[positionsIndex].byteOffset];

		int normalsIndex = mesh.primitives[0].attributes.normal;
		assert( accessors[normalsIndex].componentType == FLOAT );
		assert( accessors[normalsIndex].type == VEC3 );
		const byte * normals = &data[buffer_views[normalsIndex].byteOffset];

		int tangentsIndex = mesh.primitives[0].attributes.tangent;
		assert( accessors[tangentsIndex].componentType == FLOAT );
		assert( accessors[tangentsIndex].type == VEC4 );
		const byte * tangents = &data[buffer_views[tangentsIndex].byteOffset];

		int texcoordsIndex = mesh.primitives[0].attributes.texcoord_0;
		assert( accessors[texcoordsIndex].componentType == FLOAT );
		assert( accessors[texcoordsIndex].type == VEC2 );
		const byte * texcoords = &data[buffer_views[texcoordsIndex].byteOffset];

		int indexesIndex = mesh.primitives[0].indices;
		assert( accessors[indexesIndex].componentType == UNSIGNED_SHORT );
		assert( accessors[indexesIndex].type == SCALAR );
		const byte* indexes = &data[buffer_views[indexesIndex].byteOffset];

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

		return CreateGfxModel( modelVIDescs, modelData, vertexCount, indexes_32.data(), indexCount, sizeof( uint32_t ), allocator );
	}

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

		std::vector<byte> bufferChunk;
		const uint32_t bufferChunkSize = bufferHeader[0];
		bufferChunk.resize( bufferChunkSize );
		fs.read( reinterpret_cast<char*>( bufferChunk.data() ), bufferChunkSize );

		assert( buffers.size() == 1 );

		int positionsIndex = mesh.primitives[0].attributes.position;
		assert( accessors[positionsIndex].componentType == FLOAT );
		assert( accessors[positionsIndex].type == VEC3 );
		const byte * positions = &bufferChunk[bufferViews[positionsIndex].byteOffset];

		int indexesIndex = mesh.primitives[0].indices;
		assert( accessors[indexesIndex].componentType == UNSIGNED_SHORT );
		assert( accessors[indexesIndex].type == SCALAR );
		const byte* indexes = &bufferChunk[bufferViews[indexesIndex].byteOffset];

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
		const glTF_Json gltf_json = ReadJson( fileName );

		assert( gltf_json.meshes.size() == 1 );

		*model = LoadMesh( gltf_json.meshes[0], gltf_json.accessors, gltf_json.bufferViews, gltf_json.data.data(), allocator );
	}

	void LoadScene( const char* fileName, RegisterGfxModelCallback_t registerGfxModelCallback, RegisterGfxAssetCallback_t registerGfxAssetCallback, RegisterSceneInstanceCallback_t registerSceneInstanceCallback, I_BufferAllocator* allocator )
	{
		const glTF_Json gltf_json = ReadJson( fileName );

		//TODO: currently 1 model == 1 asset
		std::vector<GfxAsset*> gfxAssets;
		gfxAssets.reserve( gltf_json.meshes.size() );

		for( const Mesh& mesh : gltf_json.meshes )
		{
			GfxModel* gfxModel = registerGfxModelCallback( mesh.name.c_str() );
			*gfxModel = LoadMesh( mesh, gltf_json.accessors, gltf_json.bufferViews, gltf_json.data.data(), allocator );
			GfxAsset* gfxAsset = registerGfxAssetCallback( mesh.name.c_str() );
			gfxAsset->modelAsset = gfxModel;
			gfxAsset->textureIndices.push_back( 0 ); // TODO
			gfxAssets.push_back( gfxAsset );
		}

		for( auto i = 0; i < gltf_json.nodes.size(); ++i )
		{
			const Node& node = gltf_json.nodes[i];
			if( node.meshIndex == INVALID_INT )
				continue;

			assert( node.meshIndex < gfxAssets.size() );
			SceneInstance* sceneInstance = registerSceneInstanceCallback( node.name.c_str(), gfxAssets[node.meshIndex] );

			sceneInstance->location.x = node.translation.x;
			sceneInstance->location.y = node.translation.y;
			sceneInstance->location.z = node.translation.z * -1.0f;//TODO: glTF forced to right handed with Z backward

			sceneInstance->orientation.w = node.rotation.w;
			sceneInstance->orientation.x = node.rotation.x;
			sceneInstance->orientation.y = node.rotation.y;
			sceneInstance->orientation.z = node.rotation.z;

			sceneInstance->scale = node.scale.x;

			sceneInstance->parent = nullptr; // TODO?
		}
	}
}