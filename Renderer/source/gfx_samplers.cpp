#include "gfx_samplers.h"

#include <algorithm>
#include <array>
#include <stdexcept>

std::array<GfxApiSampler, ( size_t )(eSamplers::Count)> samplers;

static void createPointSampler( GfxApiSampler* o_sampler )
{
	if( !CreateSampler( GfxFilter::NEAREST, GfxFilter::NEAREST, GfxMipFilter::NEAREST, 16, GfxSamplerAddressMode::REPEAT, GfxCompareOp::NONE, o_sampler ) )
		throw std::runtime_error( "failed to create texture sampler!" );		

	MarkGfxObject( *o_sampler, "point sampler" );
}

static void createTriLinearSampler( GfxApiSampler* o_sampler )
{
	if( !CreateSampler( GfxFilter::LINEAR, GfxFilter::LINEAR, GfxMipFilter::LINEAR, 16, GfxSamplerAddressMode::REPEAT, GfxCompareOp::NONE, o_sampler ) )
		throw std::runtime_error( "failed to create texture sampler!" );

	MarkGfxObject( *o_sampler, "trilinear sampler" );
}

static void createShadowSampler( GfxApiSampler* o_sampler )
{
	if( !CreateSampler( GfxFilter::LINEAR, GfxFilter::LINEAR, GfxMipFilter::LINEAR, 16, GfxSamplerAddressMode::CLAMP_TO_BORDER, GfxCompareOp::LESS_OR_EQUAL, o_sampler ) )
		throw std::runtime_error( "failed to create texture sampler!" );

	MarkGfxObject( *o_sampler, "shadow sampler" );
}

void InitSamplers()
{
	createPointSampler( &samplers[( size_t )eSamplers::Point] );
	createTriLinearSampler( &samplers[( size_t )eSamplers::Trilinear] );
	createShadowSampler( &samplers[( size_t )eSamplers::Shadow] );
}

void DestroySamplers()
{
	for( size_t i = 0; i < samplers.size(); ++i )
		Destroy( &samplers[i] );
}

GfxApiSampler GetSampler( eSamplers samplerId )
{
	return samplers[( size_t )samplerId];
}