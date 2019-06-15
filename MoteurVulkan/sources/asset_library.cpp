#include "asset_library.h"

#include <vector>
#include <array>
#include <unordered_map>
#include <string>

namespace AL
{
	std::array<GfxImage, 32> _images;
	size_t _imagesCount;
	std::array<GfxModel, 32> _modelAssets;
	size_t _modelAssetsCount;
	std::unordered_map<std::string, void*> _asset_map;

	static void AL_AddAsset(const char* assetName, void* assetPtr)
	{
		_asset_map[std::string(assetName)] = assetPtr;
	}

	static GfxImage* AL_GetImageSlot(const char* assetName)
	{
		assert(_imagesCount < 32);
		GfxImage* imageSlot = &_images[_imagesCount++];
		AL_AddAsset(assetName, imageSlot);
		return imageSlot;
	}

	static GfxModel* AL_GetModelSlot(const char* assetName)
	{
		assert(_modelAssetsCount < 32);
		GfxModel* modelSlot = &_modelAssets[_modelAssetsCount++];
		AL_AddAsset(assetName, modelSlot);
		return modelSlot;
	}

	GfxImage* LoadTexture(const char* assetName, const char* assetPath)
	{
		//TODO when i'll need it
		return nullptr;
	}

	GfxImage* LoadCubeTexture(const char* assetName, const char* assetPath)
	{
		GfxImage* cubeImage = AL_GetImageSlot(assetName);
		Load3DTexture(assetPath, *cubeImage);

		return cubeImage;
	}

	GfxImage* CreateSolidColorTexture(const char* assetName, glm::vec4 color)
	{
		GfxImage* colorImage = AL_GetImageSlot(assetName);
		CreateSolidColorImage(color, colorImage);

		return colorImage;
	}

	GfxModel* Load3DModel(const char* assetName, const char* assetPath, uint32_t hackIndex)
	{
		GfxModel* modelAsset = AL_GetModelSlot(assetName);
		LoadGenericModel(assetPath, *modelAsset, hackIndex);

		return modelAsset;
	}

	void* GetAsset(const char* assetName)
	{
		return _asset_map.find(std::string(assetName))->second;
	}

	void Cleanup()
	{
		for (size_t i = 0; i < _imagesCount; ++i)
			DestroyImage(_images[i]);
		_imagesCount = 0;

		for (size_t i = 0; i < _modelAssetsCount; ++i)
			DestroyModelAsset(_modelAssets[i]);
		_modelAssetsCount = 0;

		_asset_map.clear();
	}
}