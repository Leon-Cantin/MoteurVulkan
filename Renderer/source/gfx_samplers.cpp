#include "gfx_samplers.h"

#include <algorithm>
#include <array>
#include <stdexcept>

std::array<R_HW::GfxApiSampler, ( size_t )(eSamplers::Count)> samplers;

static void createPointSampler( R_HW::GfxApiSampler* o_sampler )
{
	if( !R_HW::CreateSampler( R_HW::GfxFilter::NEAREST, R_HW::GfxFilter::NEAREST, R_HW::GfxMipFilter::NEAREST, 16, R_HW::GfxSamplerAddressMode::REPEAT, R_HW::GfxCompareOp::NONE, o_sampler ) )
		throw std::runtime_error( "failed to create texture sampler!" );		

	R_HW::MarkGfxObject( *o_sampler, "point sampler" );
}

static void createTriLinearSampler( R_HW::GfxApiSampler* o_sampler )
{
	if( !R_HW::CreateSampler( R_HW::GfxFilter::LINEAR, R_HW::GfxFilter::LINEAR, R_HW::GfxMipFilter::LINEAR, 16, R_HW::GfxSamplerAddressMode::REPEAT, R_HW::GfxCompareOp::NONE, o_sampler ) )
		throw std::runtime_error( "failed to create texture sampler!" );

	R_HW::MarkGfxObject( *o_sampler, "trilinear sampler" );
}

static void createShadowSampler( R_HW::GfxApiSampler* o_sampler )
{
	if( !R_HW::CreateSampler( R_HW::GfxFilter::LINEAR, R_HW::GfxFilter::LINEAR, R_HW::GfxMipFilter::LINEAR, 16, R_HW::GfxSamplerAddressMode::CLAMP_TO_BORDER, R_HW::GfxCompareOp::LESS_OR_EQUAL, o_sampler ) )
		throw std::runtime_error( "failed to create texture sampler!" );

	R_HW::MarkGfxObject( *o_sampler, "shadow sampler" );
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
		R_HW::Destroy( &samplers[i] );
}

R_HW::GfxApiSampler GetSampler( eSamplers samplerId )
{
	return samplers[( size_t )samplerId];
}

R_HW::GfxApiSampler* GetSamplers()
{
	return samplers.data();
}