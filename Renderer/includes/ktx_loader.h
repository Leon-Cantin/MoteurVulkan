#pragma once

#include <stdio.h>
#include <memory>
#include <vector>

struct KTX_Header
{
	unsigned char       identifier[12];
	unsigned int        endianness;
	unsigned int        gltype;
	unsigned int        gltypesize;
	unsigned int        glformat;
	unsigned int        glinternalformat;
	unsigned int        glbaseinternalformat;
	unsigned int        pixelwidth;
	unsigned int        pixelheight;
	unsigned int        pixeldepth;
	unsigned int        arrayelements;
	unsigned int        faces;
	unsigned int        miplevels;
	unsigned int        keypairbytes;
};

enum class TextureType {
	None,
	T_1D,
	T_2D,
	T_1D_A,
	T_2D_A,
	T_3D,
	Cube,
	Cube_A
};

static const unsigned char identifier[] =
{
	0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

static const unsigned int swap32(const unsigned int u32)
{
	union
	{
		unsigned int u32;
		unsigned char u8[4];
	} a, b;

	a.u32 = u32;
	b.u8[0] = a.u8[3];
	b.u8[1] = a.u8[2];
	b.u8[2] = a.u8[1];
	b.u8[3] = a.u8[0];

	return b.u32;
}

#define _CRT_SECURE_NO_WARNINGS

bool load_ktx(const char * filename, std::vector<char>& o_data, KTX_Header& o_header, TextureType& o_texture_type)
{
	FILE * fp;
	size_t data_start, data_end;
	o_texture_type = TextureType::None;

	fopen_s(&fp, filename, "rb");

	if (!fp)
		return 0;

	if (fread(&o_header, sizeof(o_header), 1, fp) != 1)
		goto fail_read;

	if (memcmp(o_header.identifier, identifier, sizeof(identifier)) != 0)
		goto fail_header;

	if (o_header.endianness == 0x04030201)
	{
		// No swap needed
	}
	else if (o_header.endianness == 0x01020304)
	{
		// Swap needed
		o_header.endianness = swap32(o_header.endianness);
		o_header.gltype = swap32(o_header.gltype);
		o_header.gltypesize = swap32(o_header.gltypesize);
		o_header.glformat = swap32(o_header.glformat);
		o_header.glinternalformat = swap32(o_header.glinternalformat);
		o_header.glbaseinternalformat = swap32(o_header.glbaseinternalformat);
		o_header.pixelwidth = swap32(o_header.pixelwidth);
		o_header.pixelheight = swap32(o_header.pixelheight);
		o_header.pixeldepth = swap32(o_header.pixeldepth);
		o_header.arrayelements = swap32(o_header.arrayelements);
		o_header.faces = swap32(o_header.faces);
		o_header.miplevels = swap32(o_header.miplevels);
		o_header.keypairbytes = swap32(o_header.keypairbytes);
	}
	else
	{
		goto fail_header;
	}

	// Guess target (texture type)
	if (o_header.pixelheight == 0)
	{
		if (o_header.arrayelements == 0)
			o_texture_type = TextureType::T_1D;
		else
			o_texture_type = TextureType::T_1D_A;
	}
	else if (o_header.pixeldepth == 0)
	{
		if (o_header.arrayelements == 0)
		{
			if (o_header.faces == 0)
				o_texture_type = TextureType::T_2D;
			else
				o_texture_type = TextureType::Cube;
		}
		else
		{
			if (o_header.faces == 0)
				o_texture_type = TextureType::T_2D_A;
			else
				o_texture_type = TextureType::Cube_A;
		}
	}
	else
	{
		o_texture_type = TextureType::T_3D;
	}

	// Check for insanity...
	if (o_texture_type == TextureType::None ||                  // Couldn't figure out target
		(o_header.pixelwidth == 0) ||                                  // Texture has no width???
		(o_header.pixelheight == 0 && o_header.pixeldepth != 0))              // Texture has depth but no height???
	{
		goto fail_header;
	}

	data_start = ftell(fp) + o_header.keypairbytes;
	fseek(fp, 0, SEEK_END);
	data_end = ftell(fp);
	fseek(fp, data_start, SEEK_SET);

	o_data.resize(data_end - data_start);
	memset(o_data.data(), 0, data_end - data_start);

	fread(o_data.data(), 1, data_end - data_start, fp);

	if (o_header.miplevels == 0)
	{
		o_header.miplevels = 1;
	}

fail_header:;
fail_read:;
	fclose(fp);

	return o_texture_type != TextureType::None;
}