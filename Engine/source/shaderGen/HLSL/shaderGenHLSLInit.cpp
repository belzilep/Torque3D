//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "shaderGen/shaderGen.h"
#include "shaderGen/HLSL/shaderGenHLSL.h"
#include "shaderGen/HLSL/shaderFeatureHLSL.h"
#include "shaderGen/featureMgr.h"
#include "shaderGen/HLSL/bumpHLSL.h"
#include "shaderGen/HLSL/pixSpecularHLSL.h"
#include "shaderGen/HLSL/depthHLSL.h"
#include "shaderGen/HLSL/paraboloidHLSL.h"
#include "materials/materialFeatureTypes.h"
#include "core/module.h"


static ShaderGen::ShaderGenInitDelegate sInitDelegate;

void _initShaderGenHLSL( ShaderGen *shaderGen )
{
   shaderGen->setPrinter( new ShaderGenPrinterHLSL );
   shaderGen->setComponentFactory( new ShaderGenComponentFactoryHLSL );
   shaderGen->setFileEnding( "hlsl" );

   FEATUREMGR->registerFeature( MFT_VertTransform, new VertPositionHLSL );
   FEATUREMGR->registerFeature( MFT_RTLighting, new RTLightingFeatHLSL );
   FEATUREMGR->registerFeature( MFT_IsDXTnm, new NamedFeatureHLSL( "DXTnm" ) );
   FEATUREMGR->registerFeature( MFT_TexAnim, new TexAnimHLSL );
   FEATUREMGR->registerFeature( MFT_DiffuseMap, new DiffuseMapFeatHLSL );
   FEATUREMGR->registerFeature( MFT_OverlayMap, new OverlayTexFeatHLSL );
   FEATUREMGR->registerFeature( MFT_DiffuseColor, new DiffuseFeatureHLSL );
   FEATUREMGR->registerFeature( MFT_DiffuseVertColor, new DiffuseVertColorFeatureHLSL );
   FEATUREMGR->registerFeature( MFT_AlphaTest, new AlphaTestHLSL );
   FEATUREMGR->registerFeature( MFT_GlowMask, new GlowMaskHLSL );
   FEATUREMGR->registerFeature( MFT_LightMap, new LightmapFeatHLSL );
   FEATUREMGR->registerFeature( MFT_ToneMap, new TonemapFeatHLSL );
   FEATUREMGR->registerFeature( MFT_VertLit, new VertLitHLSL );
   FEATUREMGR->registerFeature( MFT_Parallax, new ParallaxFeatHLSL );
   FEATUREMGR->registerFeature( MFT_NormalMap, new BumpFeatHLSL );
   FEATUREMGR->registerFeature( MFT_DetailNormalMap, new NamedFeatureHLSL( "Detail Normal Map" ) );
   FEATUREMGR->registerFeature( MFT_DetailMap, new DetailFeatHLSL );
   FEATUREMGR->registerFeature( MFT_CubeMap, new ReflectCubeFeatHLSL );
   FEATUREMGR->registerFeature( MFT_PixSpecular, new PixelSpecularHLSL );
   FEATUREMGR->registerFeature( MFT_IsTranslucent, new NamedFeatureHLSL( "Translucent" ) );
   FEATUREMGR->registerFeature( MFT_IsTranslucentZWrite, new NamedFeatureHLSL( "Translucent ZWrite" ) );
   FEATUREMGR->registerFeature( MFT_Visibility, new VisibilityFeatHLSL );
   FEATUREMGR->registerFeature( MFT_Fog, new FogFeatHLSL );
   FEATUREMGR->registerFeature( MFT_SpecularMap, new SpecularMapHLSL );
   FEATUREMGR->registerFeature( MFT_GlossMap, new NamedFeatureHLSL( "Gloss Map" ) );
   FEATUREMGR->registerFeature( MFT_LightbufferMRT, new NamedFeatureHLSL( "Lightbuffer MRT" ) );
   FEATUREMGR->registerFeature( MFT_RenderTarget1_Zero, new RenderTargetZeroHLSL( ShaderFeature::RenderTarget1 ) );

   FEATUREMGR->registerFeature( MFT_DiffuseMapAtlas, new NamedFeatureHLSL( "Diffuse Map Atlas" ) );
   FEATUREMGR->registerFeature( MFT_NormalMapAtlas, new NamedFeatureHLSL( "Normal Map Atlas" ) );

   FEATUREMGR->registerFeature( MFT_NormalsOut, new NormalsOutFeatHLSL );
   
   FEATUREMGR->registerFeature( MFT_DepthOut, new DepthOutHLSL );
   FEATUREMGR->registerFeature( MFT_EyeSpaceDepthOut, new EyeSpaceDepthOutHLSL() );

   FEATUREMGR->registerFeature( MFT_HDROut, new HDROutHLSL );
   FEATUREMGR->registerFeature( MFT_HeatMap, new HeatMapFeatureHLSL );

   FEATUREMGR->registerFeature( MFT_ParaboloidVertTransform, new ParaboloidVertTransformHLSL );
   FEATUREMGR->registerFeature( MFT_IsSinglePassParaboloid, new NamedFeatureHLSL( "Single Pass Paraboloid" ) );
   FEATUREMGR->registerFeature( MFT_UseInstancing, new NamedFeatureHLSL( "Hardware Instancing" ) );

   FEATUREMGR->registerFeature( MFT_Foliage, new FoliageFeatureHLSL );

   FEATUREMGR->registerFeature( MFT_ParticleNormal, new ParticleNormalFeatureHLSL );

   FEATUREMGR->registerFeature( MFT_InterlacedPrePass, new NamedFeatureHLSL( "Interlaced Pre Pass" ) );

   FEATUREMGR->registerFeature( MFT_ForwardShading, new NamedFeatureHLSL( "Forward Shaded Material" ) );

   FEATUREMGR->registerFeature( MFT_ImposterVert, new ImposterVertFeatureHLSL );

   //  [4/16/2013 belp1710]
   FEATUREMGR->registerFeature( MFT_Fur, new FurFeatureHLSL );
   FEATUREMGR->registerFeature( MFT_Vert_Translate, new VertTranslateFeatureHLSL );

   FEATUREMGR->registerFeature( MFT_Layer0, new Layer0FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer1, new Layer1FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer2, new Layer2FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer3, new Layer3FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer4, new Layer4FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer5, new Layer5FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer6, new Layer6FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer7, new Layer7FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer8, new Layer8FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer9, new Layer9FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer10, new Layer10FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer11, new Layer11FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer12, new Layer12FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer13, new Layer13FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer14, new Layer14FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer15, new Layer15FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer16, new Layer16FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer17, new Layer17FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer18, new Layer18FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer19, new Layer19FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer20, new Layer20FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer21, new Layer21FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer22, new Layer22FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer23, new Layer23FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer24, new Layer24FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer25, new Layer25FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer26, new Layer26FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer27, new Layer27FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer28, new Layer28FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer29, new Layer29FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer30, new Layer30FeatHLSL );
   FEATUREMGR->registerFeature( MFT_Layer31, new Layer31FeatHLSL );
}

MODULE_BEGIN( ShaderGenHLSL )

   MODULE_INIT_AFTER( ShaderGen )
   MODULE_INIT_AFTER( ShaderGenFeatureMgr )
   
   MODULE_INIT
   {
      sInitDelegate.bind(_initShaderGenHLSL);
      SHADERGEN->registerInitDelegate(Direct3D9, sInitDelegate);
      SHADERGEN->registerInitDelegate(Direct3D9_360, sInitDelegate);
   }
   
MODULE_END;
