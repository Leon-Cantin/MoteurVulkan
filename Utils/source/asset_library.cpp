#include "asset_library.h"

#include "glTF_loader.h"
#include "assimp_loader.h"
#include "stb_image.h"
#include "generate_geometry.h"

#include <vector>
#include <array>
#include <unordered_map>
#include <string>

namespace AL
{
	constexpr size_t MAX_ASSETS = 32;
	std::array<R_HW::GfxImage, MAX_ASSETS> _images;
	size_t _imagesCount;
	std::array<GfxModel, MAX_ASSETS> _modelAssets;
	size_t _modelAssetsCount;
	std::array<GfxAsset, MAX_ASSETS> _gfxAssets;
	size_t _gfxAssetsCount;
	std::unordered_map<std::string, void*> _asset_map;

	static void AL_AddAsset(const char* assetName, void* assetPtr)
	{
		_asset_map[std::string(assetName)] = assetPtr;
	}

	R_HW::GfxImage* AL_GetImageSlot(const char* assetName)
	{
		assert(_imagesCount < MAX_ASSETS );
		R_HW::GfxImage* imageSlot = &_images[_imagesCount++];
		AL_AddAsset(assetName, imageSlot);
		return imageSlot;
	}

	GfxModel* AL_GetModelSlot(const char* assetName)
	{
		assert(_modelAssetsCount < MAX_ASSETS );
		GfxModel* modelSlot = &_modelAssets[_modelAssetsCount++];
		AL_AddAsset(assetName, modelSlot);
		return modelSlot;
	}

	GfxAsset* AL_GetAssetSlot( const char* assetName )
	{
		assert( _gfxAssetsCount < MAX_ASSETS );
		GfxAsset* assetSlot = &_gfxAssets[_gfxAssetsCount++];
		//TODO sine we have a single map, assets may clash with models: AL_AddAsset( assetName, modelSlot );
		return assetSlot;
	}

	R_HW::GfxImage* LoadTexture(const char* assetName, const char* assetPath, I_ImageAlloctor* allocator )
	{
		R_HW::GfxImage* image = AL_GetImageSlot( assetName );
		Load2DTextureFromFile( assetPath, image, allocator );
		return image;
	}

	R_HW::GfxImage* LoadCubeTexture(const char* assetName, const char* assetPath, I_ImageAlloctor* allocator )
	{
		R_HW::GfxImage* cubeImage = AL_GetImageSlot( assetName );
		Load3DTexture( assetPath, cubeImage, allocator );

		return cubeImage;
	}

	R_HW::GfxImage* CreateSolidColorTexture(const char* assetName, glm::vec4 color, I_ImageAlloctor* allocator )
	{
		R_HW::GfxImage* colorImage = AL_GetImageSlot( assetName );
		CreateSolidColorImage( color, colorImage, allocator );

		return colorImage;
	}

	GfxModel* Load3DModel (const char* assetName, const char* assetPath, uint32_t hackIndex, R_HW::I_BufferAllocator* allocator )
	{
		GfxModel* modelAsset = AL_GetModelSlot( assetName );
		LoadModel_AssImp( assetPath, *modelAsset, hackIndex, allocator );

		return modelAsset;
	}

	GfxModel* LoadglTf3DModel( const char* assetName, const char* assetPath, R_HW::I_BufferAllocator* allocator )
	{
		GfxModel* modelAsset = AL_GetModelSlot( assetName );
		glTF_L::LoadMesh( assetPath, modelAsset, allocator );

		return modelAsset;
	}

	GfxModel* CreateQuad( const char* assetName, float size, R_HW::I_BufferAllocator* allocator )
	{
		GfxModel* modelAsset = AL_GetModelSlot( assetName );
		*modelAsset = GenerateQuad( size, allocator );

		return modelAsset;
	}

	GfxModel* CreateQuad( const char* assetName, float width, float height, R_HW::I_BufferAllocator* allocator )
	{
		GfxModel* modelAsset = AL_GetModelSlot( assetName );
		*modelAsset = GenerateQuad( width, height, allocator );

		return modelAsset;
	}

	GfxModel* RegisterGfxModel( const char* asset_name, GfxModel&& model )
	{
		GfxModel* modelAsset = AL_GetModelSlot( asset_name );
		*modelAsset = model;
		return modelAsset;
	}

	void* GetAsset(const char* assetName)
	{
		return _asset_map.find(std::string(assetName))->second;
	}

	void Cleanup()
	{
		for( size_t i = 0; i < _imagesCount; ++i )
			DestroyImage( &_images[i] );
		_imagesCount = 0;

		for (size_t i = 0; i < _modelAssetsCount; ++i)
			DestroyGfxModel(_modelAssets[i]);
		_modelAssetsCount = 0;

		_asset_map.clear();
	}
}