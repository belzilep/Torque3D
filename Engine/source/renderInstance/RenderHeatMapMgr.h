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

#ifndef _RENDERHEATMAPMGR_H_
#define _RENDERHEATMAPMGR_H_

#ifndef _TEXTARGETBIN_MGR_H_
#include "renderInstance/renderTexTargetBinManager.h"
#endif


class PostEffect;


///
class RenderHeatMapMgr : public RenderTexTargetBinManager
{  
	typedef RenderTexTargetBinManager Parent;

public:

	RenderHeatMapMgr();
	virtual ~RenderHeatMapMgr();

	/// Returns the heat vision post effect.
	PostEffect* getHeatVisionEffect();

	/// Returns true if the heat vision post effect is
	/// enabled and the heat map buffer should be updated.
	bool isHeatVisionEnabled();

	// RenderBinManager
	virtual void addElement( RenderInst *inst );
	virtual void render( SceneRenderState *state );

	// ConsoleObject
	DECLARE_CONOBJECT( RenderHeatMapMgr );

protected:

	class HeatMaterialHook : public MatInstanceHook
	{
	public:

		HeatMaterialHook( BaseMatInstance *matInst );
		virtual ~HeatMaterialHook();

		virtual BaseMatInstance *getMatInstance() { return mHeatMatInst; }

		virtual const MatInstanceHookType& getType() const { return Type; }

		/// Our material hook type.
		static const MatInstanceHookType Type;

	protected:

		static void _overrideFeatures(   ProcessedMaterial *mat,
			U32 stageNum,
			MaterialFeatureData &fd, 
			const FeatureSet &features );

		BaseMatInstance *mHeatMatInst; 
	};

	SimObjectPtr<PostEffect> mHeatVisionEffect;

};


#endif // _RENDERHEATMAPMGR_H_
