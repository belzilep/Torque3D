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

#include "ts/tsShapeInstance.h"
#include "T3D/tsStatic.h"
#include "T3D/player.h"

#include <limits>

//----------------------------------------------------------------------------------
// some utility functions
//-------------------------------------------------------------------------------------

S32 QSORT_CALLBACK FN_CDECL compareThreads( const void* e1, const void* e2)
{
   const TSThread * th1 = *(const TSThread**)e1;
   const TSThread * th2 = *(const TSThread**)e2;
   return (*th1 < *th2);
}

void TSShapeInstance::sortThreads()
{
   PROFILE_SCOPE( TSShapeInstance_sortThreads );
   dQsort(mThreadList.address(),mThreadList.size(),sizeof(TSThread*),compareThreads);
   dQsort(mTransitionThreads.address(),mTransitionThreads.size(),sizeof(TSThread*),compareThreads);
}

void TSShapeInstance::setDirty(U32 dirty)
{
   AssertFatal((dirty & AllDirtyMask) == dirty,"TSShapeInstance::setDirty: illegal dirty flags");
   for (S32 i=0; i<mShape->subShapeFirstNode.size(); i++)
      mDirtyFlags[i] |= dirty;
}

void TSShapeInstance::clearDirty(U32 dirty)
{
   AssertFatal((dirty & AllDirtyMask) == dirty,"TSShapeInstance::clearDirty: illegal dirty flags");
   for (S32 i=0; i<mShape->subShapeFirstNode.size(); i++)
      mDirtyFlags[i] &= ~dirty;
}

//-------------------------------------------------------------------------------------
// Animate nodes
//-------------------------------------------------------------------------------------

void TSShapeInstance::animateNodes(S32 ss)
{
   PROFILE_SCOPE( TSShapeInstance_animateNodes );

   if (!mShape->nodes.size())
      return;

   // @todo: When a node is added, we need to make sure to resize the nodeTransforms array as well
   if (mNodeTransforms.size() != mShape->nodes.size())
   {
	   mNodeTransforms.setSize(mShape->nodes.size());
   }     

   // temporary storage for node transforms
   smNodeCurrentRotations.setSize(mShape->nodes.size());
   smNodeCurrentTranslations.setSize(mShape->nodes.size());
   smNodeLocalTransforms.setSize(mShape->nodes.size());
   smRotationThreads.setSize(mShape->nodes.size());
   smTranslationThreads.setSize(mShape->nodes.size());

   TSIntegerSet rotBeenSet;
   TSIntegerSet tranBeenSet;
   TSIntegerSet scaleBeenSet;
   rotBeenSet.setAll(mShape->nodes.size());
   tranBeenSet.setAll(mShape->nodes.size());
   scaleBeenSet.setAll(mShape->nodes.size());
   smNodeLocalTransformDirty.clearAll();

   S32 i,j,nodeIndex,subShapeFirstNode,subShapeLastNode,start,end,firstBlend = mThreadList.size();

   for (i=0; i<mThreadList.size(); i++)
   {
      TSThread * curThread = mThreadList[i];

      if (curThread->getSequence()->isBlend())
      {
         // blend sequences need default (if not set by other sequence)
         // break rather than continue because the rest will be blends too
         firstBlend = i;
         break;
      }
      rotBeenSet.takeAway(curThread->getSequence()->rotationMatters);
      tranBeenSet.takeAway(curThread->getSequence()->translationMatters);
      scaleBeenSet.takeAway(curThread->getSequence()->scaleMatters);
   }

   rotBeenSet.takeAway(mCallbackNodes);
   rotBeenSet.takeAway(mHandsOffNodes);
   rotBeenSet.overlap(mMaskRotationNodes);

   TSIntegerSet maskPosNodes=mMaskPosXNodes;
   maskPosNodes.overlap(mMaskPosYNodes);
   maskPosNodes.overlap(mMaskPosZNodes);
   tranBeenSet.overlap(maskPosNodes);

   tranBeenSet.takeAway(mCallbackNodes);
   tranBeenSet.takeAway(mHandsOffNodes);
   // can't add masked nodes since x, y, & z masked separately...
   // we'll set default regardless of mask status

   // all the nodes marked above need to have the default transform
   subShapeFirstNode = mShape->subShapeFirstNode[ss];
   subShapeLastNode = subShapeFirstNode + mShape->subShapeNumNodes[ss];

   for (i=subShapeFirstNode; i<subShapeLastNode; i++)
   {
      if (rotBeenSet.test(i))
      {
         mShape->defaultRotations[i].getQuatF(&smNodeCurrentRotations[i]);
         smRotationThreads[i] = NULL;
      }

      if (tranBeenSet.test(i))
      {
         smNodeCurrentTranslations[i] = mShape->defaultTranslations[i];
         smTranslationThreads[i] = NULL;
      }
   }

   // don't want a transform in these cases...
   rotBeenSet.overlap(mHandsOffNodes);
   rotBeenSet.overlap(mCallbackNodes);

   tranBeenSet.takeAway(maskPosNodes);
   tranBeenSet.overlap(mHandsOffNodes);
   tranBeenSet.overlap(mCallbackNodes);

   // default scale
   if (scaleCurrentlyAnimated())
      handleDefaultScale(subShapeFirstNode,subShapeLastNode,scaleBeenSet);   

   // handle non-blend sequences
   for (i = 0; i < firstBlend; i++)
   {
      TSThread * curThread = mThreadList[i];

      j=0;
      start = curThread->getSequence()->rotationMatters.start();
      end   = subShapeLastNode;

      for (nodeIndex = start; nodeIndex < end; curThread->getSequence()->rotationMatters.next(nodeIndex), j++)
      {
         // skip nodes outside of this detail
         if (nodeIndex < subShapeFirstNode)
            continue;

         if (!rotBeenSet.test(nodeIndex))
         {
			QuatF q1,q2;
			mShape->getRotation(*curThread->getSequence(), curThread->keyNum1, j, &q1);
			mShape->getRotation(*curThread->getSequence(), curThread->keyNum2, j, &q2);
			TSTransform::interpolate(q1, q2, curThread->keyPos, &smNodeCurrentRotations[nodeIndex]);
			rotBeenSet.set(nodeIndex);
			smRotationThreads[nodeIndex] = curThread;
         }
      }

      j=0;
      start = curThread->getSequence()->translationMatters.start();
      end   = subShapeLastNode;

      for (nodeIndex = start; nodeIndex < end; curThread->getSequence()->translationMatters.next(nodeIndex), j++)
      {
         if (nodeIndex < subShapeFirstNode)
            continue;

         if (!tranBeenSet.test(nodeIndex))
         {
            if (maskPosNodes.test(nodeIndex))
               handleMaskedPositionNode(curThread,nodeIndex,j);
            else
            {
               const Point3F & p1 = mShape->getTranslation(*curThread->getSequence(), curThread->keyNum1, j);
               const Point3F & p2 = mShape->getTranslation(*curThread->getSequence(), curThread->keyNum2, j);
               TSTransform::interpolate(p1, p2, curThread->keyPos, &smNodeCurrentTranslations[nodeIndex]);
               smTranslationThreads[nodeIndex] = curThread;
            }
            tranBeenSet.set(nodeIndex);
         }
      }

      if (scaleCurrentlyAnimated())
         handleAnimatedScale(curThread,subShapeFirstNode,subShapeLastNode,scaleBeenSet);
   }   

   // compute transforms
   for (i=subShapeFirstNode; i<subShapeLastNode; i++)
   {
	   if (!mHandsOffNodes.test(i))
	   {
		   TSTransform::setMatrix(smNodeCurrentRotations[i], smNodeCurrentTranslations[i], &smNodeLocalTransforms[i]);
	   }
	   else
	   {
		   smNodeLocalTransforms[i] = mNodeTransforms[i];     // in case mNodeTransform was changed externally
	   }
   }

   // add scale onto transforms
   if (scaleCurrentlyAnimated())
      handleNodeScale(subShapeFirstNode,subShapeLastNode);

   // get callbacks...
   start = getMax(mCallbackNodes.start(),subShapeFirstNode);
   end = getMin(mCallbackNodes.end(),subShapeLastNode);
   for (i=0; i<mNodeCallbacks.size(); i++)
   {
      AssertFatal(mNodeCallbacks[i].callback, "No callback method defined");
      S32 nodeIndex = mNodeCallbacks[i].nodeIndex;
      if (nodeIndex>=start && nodeIndex<end)
      {
         mNodeCallbacks[i].callback->setNodeTransform(this, nodeIndex, smNodeLocalTransforms[nodeIndex]);
         smNodeLocalTransformDirty.set(nodeIndex);
      }
   }

   // handle blend sequences
   for (i=firstBlend; i<mThreadList.size(); i++)
   {
      TSThread * th = mThreadList[i];
      if (th->blendDisabled)
         continue;

      handleBlendSequence(th,subShapeFirstNode,subShapeLastNode);
   }

   // transitions...
   if (inTransition())
      handleTransitionNodes(subShapeFirstNode,subShapeLastNode);

   // multiply transforms...
   for (i = subShapeFirstNode; i < subShapeLastNode; i++)
   {
      S32 parentIdx = mShape->nodes[i].parentIndex;
      if (parentIdx < 0)
         mNodeTransforms[i] = smNodeLocalTransforms[i];
      else
         mNodeTransforms[i].mul(mNodeTransforms[parentIdx],smNodeLocalTransforms[i]);
   }

   if (mShapeBase && reinterpret_cast<ShapeBaseData *>(mShapeBase->getDataBlock())->IKisActive &&
	   Con::getBoolVariable( "$pref::Features::IK", true ))
   {
	   animateHead();
   }
}

void TSShapeInstance::animateHead()
{	
	static const F32 radiansPerSecond = M_PI_F / 6.0f;
	static const F32 percentageRemRot = 0.3f;
	static const F32 minTargetDistSquared = 200.0f;
	
	if (!mUpdateTimer)
	{
		mUpdateTimer = PlatformTimer::create();
	}

	if (mShapeBase)
	{ 
		if (mIKnodes.empty())
		{
			// fill the static vector for the first and only time
			for (U32 i = 0; i < getShape()->nodes.size(); i++)
			{
				if (getShape()->getNodeName(i) == String("Bip001_Ponytail1") )
				{
					MyNode node;

					node.index = i;
					node.dof = EulerF(0.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); //Bip001_Ponytail1

					node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/2.0f, 0.0f, M_PI_F/2.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Head	

					node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/2.0f, 0.0f, M_PI_F/2.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Neck2

					/*node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/3.0f, 0.0f, M_PI_F/3.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node);*/ // Bip001_Neck1

					break;
				}
			}
		} 
		else
		{
			//MatrixF nodeTransform;
			Point3F desiredEnd = Point3F(0.0f);
			bool hasTarget = false;

			//Con::printf("Animating the head of %d", this);
			//Con::printf("Position of Bip001_Ponytail1 %f %f %f", curEnd.x, curEnd.y, curEnd.z);

			// Search for the nearest player to look at
			{			
				auto players = Player::getClientSidePlayers();
				F32 desiredEndDistance = std::numeric_limits<F32>::max();

				for (int i=0; i < players.size(); ++i)
				{
					if (players[i] != mShapeBase)
					{
						F32 distance = Point3F(players[i]->getPosition() - mShapeBase->getPosition()).lenSquared();
						
						if (distance < minTargetDistSquared && distance < desiredEndDistance)
						{
							Point3F shapeToPlayer = players[i]->getPosition() - mShapeBase->getTransform().getPosition();
							shapeToPlayer.normalizeSafe();
							Point3F shapeForward = mShapeBase->getTransform().getForwardVector();
							shapeForward.normalizeSafe();

							auto cosAngle = mDot(shapeToPlayer, shapeForward);

							// check if target is behind player
							if (cosAngle > 0.2f)
							{
								MatrixF playerEyeTransform;
								players[i]->getEyeTransform(&playerEyeTransform);
								Point3F playerEyePosition = playerEyeTransform.getPosition();

								desiredEnd = playerEyePosition;
								desiredEndDistance = distance;
								hasTarget = true;
							}							
						}
					}
				}
			}

			if (hasTarget)
			{
				// transform target position from world reference to shape position
				MatrixF shapeInvTransform = mShapeBase->getTransform();
				shapeInvTransform.inverse();
				shapeInvTransform.mulP(desiredEnd);
			}

			F32 allowedRadiansPerFrame = (radiansPerSecond * (F32)mUpdateTimer->getElapsedMs() / 1000.0f);
			mUpdateTimer->reset(); // reset the timer

			for (int i = 1; i < mIKnodes.size(); i++)
			{
				Point3F rootPos,curVector, targetVector,curEnd;
				EulerF nodeCurrentRotation(0.0f);
				EulerF finalNeededRotation(0.0f);

				if (hasTarget)
				{
					// POSITION OF THE END EFFECTOR
					curEnd = mNodeTransforms[mIKnodes[0].index].getPosition();

					// THE POSITION OF THE ROOT OF THIS BONE
					rootPos = mNodeTransforms[mIKnodes[i].index].getPosition();

					// CREATE THE VECTOR TO THE CURRENT EFFECTOR POS
					curVector = curEnd - rootPos;

					// CREATE THE DESIRED EFFECTOR POSITION VECTOR
					targetVector = desiredEnd - rootPos;

					Point3F curVectorX = curVector;
					curVectorX.x = 0.0f;

					Point3F curVectorY = curVector;
					curVectorY.y = 0.0f;

					Point3F curVectorZ = curVector;
					curVectorZ.z = 0.0f;

					Point3F targetVectorX = targetVector;
					targetVectorX.x = 0.0f;

					Point3F targetVectorY = targetVector;
					targetVectorY.y = 0.0f;

					Point3F targetVectorZ = targetVector;
					targetVectorZ.z = 0.0f;

					auto angleBetween = [] (Point3F &v, Point3F &w, F32 sign) -> F32
					{
						v.normalize();
						w.normalize();

						auto cosAngle = mDot(v, w);

						Point3F crossResult = mCross(v, w);

						F32 factor = crossResult.x + crossResult.y + crossResult.z;

						if (factor < 0.0f)
						{
							return -mAcos(cosAngle) * sign;
						}
						else
						{
							return mAcos(cosAngle) * sign;
						}

					};


					finalNeededRotation.x = angleBetween(curVectorZ, targetVectorZ, 1.0);
					finalNeededRotation.y = angleBetween(curVectorY, targetVectorY, -1.0);
					finalNeededRotation.z = angleBetween(curVectorX, targetVectorX, -1.0);
				}
				

				// Degree Of Freedom control
				finalNeededRotation.x = (finalNeededRotation.x > mIKnodes[i].dof.x) ? mIKnodes[i].dof.x : ((finalNeededRotation.x < -mIKnodes[i].dof.x) ? -mIKnodes[i].dof.x : finalNeededRotation.x);
				finalNeededRotation.y = (finalNeededRotation.y > mIKnodes[i].dof.y) ? mIKnodes[i].dof.y : ((finalNeededRotation.y < -mIKnodes[i].dof.y) ? -mIKnodes[i].dof.y : finalNeededRotation.y);
				finalNeededRotation.z = (finalNeededRotation.z > mIKnodes[i].dof.z) ? mIKnodes[i].dof.z : ((finalNeededRotation.z < -mIKnodes[i].dof.z) ? -mIKnodes[i].dof.z : finalNeededRotation.z);

				auto getRotationAngle = [&] (F32 finalNeededRotation, F32 lastRotation) -> F32
				{
					F32 rotDir;

					if (finalNeededRotation < 0.0f)
					{
						rotDir = -1.0f;
					}
					else if (finalNeededRotation > 0.0f)
					{
						rotDir = 1.0f;
					}
					else
					{
						// go to the opposite side of the last rotation
						if (lastRotation < 0.0f)
						{
							rotDir = 1.0f;
						}
						else
						{
							rotDir = -1.0f;
						}
					}				

					bool exceedsNeededRotation;

					if (finalNeededRotation < 0.0f)
					{
						exceedsNeededRotation = ((rotDir * allowedRadiansPerFrame + lastRotation  ) > finalNeededRotation ) ? false : true;
					}
					else if (finalNeededRotation > 0.0f)
					{
						exceedsNeededRotation = ((rotDir * allowedRadiansPerFrame + lastRotation ) < finalNeededRotation ) ? false : true;
					}
					else
					{
						if ((rotDir * allowedRadiansPerFrame + lastRotation ) * lastRotation <= 0.0f) // the sign changed or is zero
						{
							exceedsNeededRotation = true;
						}
						else
						{
							exceedsNeededRotation = false;
						}
					}

					if (! exceedsNeededRotation)
					{
						return (rotDir * allowedRadiansPerFrame + lastRotation);
					}
					else
					{
						return finalNeededRotation;
					}
				};

				nodeCurrentRotation.x = getRotationAngle(finalNeededRotation.x, mIKnodes[i].lastRotation.x);
				nodeCurrentRotation.y = getRotationAngle(finalNeededRotation.y, mIKnodes[i].lastRotation.y);
				nodeCurrentRotation.z = getRotationAngle(finalNeededRotation.z, mIKnodes[i].lastRotation.z);


				// Clamp values (because we never know)
				nodeCurrentRotation.x = mClampF(nodeCurrentRotation.x, -mIKnodes[i].dof.x, mIKnodes[i].dof.x );
				nodeCurrentRotation.y = mClampF(nodeCurrentRotation.y, -mIKnodes[i].dof.y, mIKnodes[i].dof.y );
				nodeCurrentRotation.z = mClampF(nodeCurrentRotation.z, -mIKnodes[i].dof.z, mIKnodes[i].dof.z );

				// add the current rotation to the others
				mIKnodes[i].lastRotation = nodeCurrentRotation;

				// Apply the rotation to the current node
				smNodeCurrentRotations[mIKnodes[i].index] = QuatF(nodeCurrentRotation);		
				TSTransform::setMatrix(smNodeCurrentRotations[mIKnodes[i].index], smNodeCurrentTranslations[mIKnodes[i].index], &smNodeLocalTransforms[mIKnodes[i].index]);
				mNodeTransforms[mIKnodes[i].index].mul(mNodeTransforms[mShape->nodes[mIKnodes[i].index].parentIndex],smNodeLocalTransforms[mIKnodes[i].index]);

				// update transforms
				for (int j = i - 1; j >= 0; j--)
				{
					mNodeTransforms[mIKnodes[j].index].mul(mNodeTransforms[mIKnodes[j+1].index],smNodeLocalTransforms[mIKnodes[j].index]);
				}
			}
		}
	} 	
}


void TSShapeInstance::handleDefaultScale(S32 a, S32 b, TSIntegerSet & scaleBeenSet)
{
   // set default scale values (i.e., identity) and do any initialization
   // relating to animated scale (since scale normally not animated)

   smScaleThreads.setSize(mShape->nodes.size());
   scaleBeenSet.takeAway(mCallbackNodes);
   scaleBeenSet.takeAway(mHandsOffNodes);
   if (animatesUniformScale())
   {
      smNodeCurrentUniformScales.setSize(mShape->nodes.size());
      for (S32 i=a; i<b; i++)
         if (scaleBeenSet.test(i))
         {
            smNodeCurrentUniformScales[i] = 1.0f;
            smScaleThreads[i] = NULL;
         }
   }
   else if (animatesAlignedScale())
   {
      smNodeCurrentAlignedScales.setSize(mShape->nodes.size());
      for (S32 i=a; i<b; i++)
         if (scaleBeenSet.test(i))
         {
            smNodeCurrentAlignedScales[i].set(1.0f,1.0f,1.0f);
            smScaleThreads[i] = NULL;
         }
   }
   else
   {
      smNodeCurrentArbitraryScales.setSize(mShape->nodes.size());
      for (S32 i=a; i<b; i++)
         if (scaleBeenSet.test(i))
         {
            smNodeCurrentArbitraryScales[i].identity();
            smScaleThreads[i] = NULL;
         }
   }

   scaleBeenSet.overlap(mHandsOffNodes);
   scaleBeenSet.overlap(mCallbackNodes);
}

void TSShapeInstance::updateTransitionNodeTransforms(TSIntegerSet& transitionNodes)
{
   // handle transitions
   transitionNodes.clearAll();
   transitionNodes.overlap(mTransitionRotationNodes);
   transitionNodes.overlap(mTransitionTranslationNodes);
   transitionNodes.overlap(mTransitionScaleNodes);

   // Decompose transforms for nodes affected by the transition. Only need to do
   // for blended or scale-animated nodes, as all others are already up to date
   for (S32 i=transitionNodes.start(); i<MAX_TS_SET_SIZE; transitionNodes.next(i))
   {
      if (smNodeLocalTransformDirty.test(i))
      {
         if (scaleCurrentlyAnimated())
         {
            // @todo:No support for scale yet => need to do proper affine decomposition here
            smNodeCurrentTranslations[i] = smNodeLocalTransforms[i].getPosition();
            smNodeCurrentRotations[i].set(smNodeLocalTransforms[i]);
         }
         else
         {
            // Scale is identity => can do a cheap decomposition
            smNodeCurrentTranslations[i] = smNodeLocalTransforms[i].getPosition();
            smNodeCurrentRotations[i].set(smNodeLocalTransforms[i]);
         }
      }
   }
}

void TSShapeInstance::handleTransitionNodes(S32 a, S32 b)
{
   TSIntegerSet transitionNodes;
   updateTransitionNodeTransforms(transitionNodes);

   S32 nodeIndex;
   S32 start = mTransitionRotationNodes.start();
   S32 end   = b;
   for (nodeIndex=start; nodeIndex<end; mTransitionRotationNodes.next(nodeIndex))
   {
      if (nodeIndex<a)
         continue;
      TSThread * thread = smRotationThreads[nodeIndex];
      thread = thread && thread->transitionData.inTransition ? thread : NULL;
      if (!thread)
      {
         // if not controlled by a sequence in transition then there must be
         // some other thread out there that used to control us that is in
         // transition now...use that thread to control interpolation
         for (S32 i=0; i<mTransitionThreads.size(); i++)
         {
            if (mTransitionThreads[i]->transitionData.oldRotationNodes.test(nodeIndex) || mTransitionThreads[i]->getSequence()->rotationMatters.test(nodeIndex))
            {
               thread = mTransitionThreads[i];
               break;
            }
         }
         AssertFatal(thread!=NULL,"TSShapeInstance::handleRotTransitionNodes (rotation)");
      }
      QuatF tmpQ;
      TSTransform::interpolate(mNodeReferenceRotations[nodeIndex].getQuatF(&tmpQ),smNodeCurrentRotations[nodeIndex],thread->transitionData.pos,&smNodeCurrentRotations[nodeIndex]);
   }

   // then translation
   start = mTransitionTranslationNodes.start();
   end   = b;
   for (nodeIndex=start; nodeIndex<end; mTransitionTranslationNodes.next(nodeIndex))
   {
      TSThread * thread = smTranslationThreads[nodeIndex];
      thread = thread && thread->transitionData.inTransition ? thread : NULL;
      if (!thread)
      {
         // if not controlled by a sequence in transition then there must be
         // some other thread out there that used to control us that is in
         // transition now...use that thread to control interpolation
         for (S32 i=0; i<mTransitionThreads.size(); i++)
         {
            if (mTransitionThreads[i]->transitionData.oldTranslationNodes.test(nodeIndex) || mTransitionThreads[i]->getSequence()->translationMatters.test(nodeIndex))
            {
               thread = mTransitionThreads[i];
               break;
            }
         }
         AssertFatal(thread!=NULL,"TSShapeInstance::handleTransitionNodes (translation).");
      }
      Point3F & p = smNodeCurrentTranslations[nodeIndex];
      Point3F & p1 = mNodeReferenceTranslations[nodeIndex];
      Point3F & p2 = p;
      F32 k = thread->transitionData.pos;
      p.x = p1.x + k * (p2.x-p1.x);
      p.y = p1.y + k * (p2.y-p1.y);
      p.z = p1.z + k * (p2.z-p1.z);
   }

   // then scale...
   if (scaleCurrentlyAnimated())
   {
      start = mTransitionScaleNodes.start();
      end   = b;
      for (nodeIndex=start; nodeIndex<end; mTransitionScaleNodes.next(nodeIndex))
      {
         TSThread * thread = smScaleThreads[nodeIndex];
         thread = thread && thread->transitionData.inTransition ? thread : NULL;
         if (!thread)
         {
            // if not controlled by a sequence in transition then there must be
            // some other thread out there that used to control us that is in
            // transition now...use that thread to control interpolation
            for (S32 i=0; i<mTransitionThreads.size(); i++)
            {
               if (mTransitionThreads[i]->transitionData.oldScaleNodes.test(nodeIndex) || mTransitionThreads[i]->getSequence()->scaleMatters.test(nodeIndex))
               {
                  thread = mTransitionThreads[i];
                  break;
               }
            }
            AssertFatal(thread!=NULL,"TSShapeInstance::handleTransitionNodes (scale).");
         }
         if (animatesUniformScale())
            smNodeCurrentUniformScales[nodeIndex] += thread->transitionData.pos * (mNodeReferenceUniformScales[nodeIndex]-smNodeCurrentUniformScales[nodeIndex]);
         else if (animatesAlignedScale())
            TSTransform::interpolate(mNodeReferenceScaleFactors[nodeIndex],smNodeCurrentAlignedScales[nodeIndex],thread->transitionData.pos,&smNodeCurrentAlignedScales[nodeIndex]);
         else
         {
            QuatF q;
            TSTransform::interpolate(mNodeReferenceScaleFactors[nodeIndex],smNodeCurrentArbitraryScales[nodeIndex].mScale,thread->transitionData.pos,&smNodeCurrentArbitraryScales[nodeIndex].mScale);
            TSTransform::interpolate(mNodeReferenceArbitraryScaleRots[nodeIndex].getQuatF(&q),smNodeCurrentArbitraryScales[nodeIndex].mRotate,thread->transitionData.pos,&smNodeCurrentArbitraryScales[nodeIndex].mRotate);
         }
      }
   }

   // update transforms for transition nodes
   start = transitionNodes.start();
   end   = b;
   for (nodeIndex=start; nodeIndex<end; transitionNodes.next(nodeIndex))
   {
      TSTransform::setMatrix(smNodeCurrentRotations[nodeIndex], smNodeCurrentTranslations[nodeIndex], &smNodeLocalTransforms[nodeIndex]);
      if (scaleCurrentlyAnimated())
      {
         if (animatesUniformScale())
            TSTransform::applyScale(smNodeCurrentUniformScales[nodeIndex],&smNodeLocalTransforms[nodeIndex]);
         else if (animatesAlignedScale())
               TSTransform::applyScale(smNodeCurrentAlignedScales[nodeIndex],&smNodeLocalTransforms[nodeIndex]);
         else
            TSTransform::applyScale(smNodeCurrentArbitraryScales[nodeIndex],&smNodeLocalTransforms[nodeIndex]);
      }
   }
}

void TSShapeInstance::handleNodeScale(S32 a, S32 b)
{
   if (animatesUniformScale())
   {
      for (S32 i=a; i<b; i++)
         if (!mHandsOffNodes.test(i))
            TSTransform::applyScale(smNodeCurrentUniformScales[i],&smNodeLocalTransforms[i]);
   }
   else if (animatesAlignedScale())
   {
      for (S32 i=a; i<b; i++)
         if (!mHandsOffNodes.test(i))
            TSTransform::applyScale(smNodeCurrentAlignedScales[i],&smNodeLocalTransforms[i]);
   }
   else
   {
      for (S32 i=a; i<b; i++)
         if (!mHandsOffNodes.test(i))
            TSTransform::applyScale(smNodeCurrentArbitraryScales[i],&smNodeLocalTransforms[i]);
   }

   TSIntegerSet scaledNodes;
   scaledNodes.difference(mHandsOffNodes);
   smNodeLocalTransformDirty.overlap(scaledNodes);
}

void TSShapeInstance::handleAnimatedScale(TSThread * thread, S32 a, S32 b, TSIntegerSet & scaleBeenSet)
{
   S32 j=0;
   S32 start = thread->getSequence()->scaleMatters.start();
   S32 end   = b;

   // code the scale conversion (might need to "upgrade" from uniform to arbitrary, e.g.)
   // code uniform, aligned, and arbitrary as 0,1, and 2, respectively,
   // with sequence coding in first two bits, shape coding in next two bits
   S32 code = 0;
   if (thread->getSequence()->animatesAlignedScale())
      code += 1;
   else if (thread->getSequence()->animatesArbitraryScale())
      code += 2;
   if (animatesAlignedScale())
      code += 4;
   if (animatesArbitraryScale())
      code += 8;

   F32 uniformScale = 1.0f;
   Point3F alignedScale(0.0f, 0.0f, 0.0f);
   TSScale arbitraryScale;
   for (S32 nodeIndex=start; nodeIndex<end; thread->getSequence()->scaleMatters.next(nodeIndex), j++)
   {
      if (nodeIndex<a)
         continue;

      if (!scaleBeenSet.test(nodeIndex))
      {
         // compute scale in sequence format
         switch (code)
         {           // Sequence   Shape
            case 0:  // uniform -> uniform
            case 4:  // uniform -> aligned
            case 8:  // uniform -> arbitrary
            {
               F32 s1 = mShape->getUniformScale(*thread->getSequence(),thread->keyNum1,j);
               F32 s2 = mShape->getUniformScale(*thread->getSequence(),thread->keyNum2,j);
               uniformScale = TSTransform::interpolate(s1,s2,thread->keyPos);
               alignedScale.set(uniformScale,uniformScale,uniformScale);
               break;
            }
            case 5:  // aligned -> aligned
            case 9:  // aligned -> arbitrary
            {
               const Point3F & s1 = mShape->getAlignedScale(*thread->getSequence(),thread->keyNum1,j);
               const Point3F & s2 = mShape->getAlignedScale(*thread->getSequence(),thread->keyNum2,j);
               TSTransform::interpolate(s1,s2,thread->keyPos,&alignedScale);
               break;
            }
            case 10: // arbitrary -> arbitary
            {
               TSScale s1,s2;
               mShape->getArbitraryScale(*thread->getSequence(),thread->keyNum1,j,&s1);
               mShape->getArbitraryScale(*thread->getSequence(),thread->keyNum2,j,&s2);
               TSTransform::interpolate(s1,s2,thread->keyPos,&arbitraryScale);
               break;
            }
            default: AssertFatal(0,"TSShapeInstance::handleAnimatedScale"); break;
         }

         switch (code)
         {
            case 0:  // uniform -> uniform
            {
               smNodeCurrentUniformScales[nodeIndex] = uniformScale;
               break;
            }
            case 4:  // uniform -> aligned
            case 5:  // aligned -> aligned
               smNodeCurrentAlignedScales[nodeIndex] = alignedScale;
               break;
            case 8:  // uniform -> arbitrary
            case 9:  // aligned -> arbitrary
            {
               smNodeCurrentArbitraryScales[nodeIndex].identity();
               smNodeCurrentArbitraryScales[nodeIndex].mScale = alignedScale;
               break;
            }
            case 10: // arbitrary -> arbitary
            {
               smNodeCurrentArbitraryScales[nodeIndex] = arbitraryScale;
               break;
            }
            default: AssertFatal(0,"TSShapeInstance::handleAnimatedScale"); break;
         }
         smScaleThreads[nodeIndex] = thread;
         scaleBeenSet.set(nodeIndex);
      }
   }
}

void TSShapeInstance::handleMaskedPositionNode(TSThread * th, S32 nodeIndex, S32 offset)
{
   const Point3F & p1 = mShape->getTranslation(*th->getSequence(),th->keyNum1,offset);
   const Point3F & p2 = mShape->getTranslation(*th->getSequence(),th->keyNum2,offset);
   Point3F p;
   TSTransform::interpolate(p1,p2,th->keyPos,&p);

   if (!mMaskPosXNodes.test(nodeIndex))
      smNodeCurrentTranslations[nodeIndex].x = p.x;

   if (!mMaskPosYNodes.test(nodeIndex))
      smNodeCurrentTranslations[nodeIndex].y = p.y;

   if (!mMaskPosZNodes.test(nodeIndex))
      smNodeCurrentTranslations[nodeIndex].z = p.z;
}

void TSShapeInstance::handleBlendSequence(TSThread * thread, S32 a, S32 b)
{
   S32 jrot=0;
   S32 jtrans=0;
   S32 jscale=0;
   TSIntegerSet nodeMatters = thread->getSequence()->translationMatters;
   nodeMatters.overlap(thread->getSequence()->rotationMatters);
   nodeMatters.overlap(thread->getSequence()->scaleMatters);
   S32 start = nodeMatters.start();
   S32 end   = b;
   for (S32 nodeIndex=start; nodeIndex<end; nodeMatters.next(nodeIndex))
   {
      // skip nodes outside of this detail
      if (start<a || mDisableBlendNodes.test(nodeIndex))
      {
         if (thread->getSequence()->rotationMatters.test(nodeIndex))
            jrot++;
         if (thread->getSequence()->translationMatters.test(nodeIndex))
            jtrans++;
         if (thread->getSequence()->scaleMatters.test(nodeIndex))
            jscale++;
         continue;
      }

      MatrixF mat(true);
      if (thread->getSequence()->rotationMatters.test(nodeIndex))
      {
         QuatF q1,q2;
         mShape->getRotation(*thread->getSequence(),thread->keyNum1,jrot,&q1);
         mShape->getRotation(*thread->getSequence(),thread->keyNum2,jrot,&q2);
         QuatF quat;
         TSTransform::interpolate(q1,q2,thread->keyPos,&quat);
         TSTransform::setMatrix(quat,&mat);
         jrot++;
      }

      if (thread->getSequence()->translationMatters.test(nodeIndex))
      {
         const Point3F & p1 = mShape->getTranslation(*thread->getSequence(),thread->keyNum1,jtrans);
         const Point3F & p2 = mShape->getTranslation(*thread->getSequence(),thread->keyNum2,jtrans);
         Point3F p;
         TSTransform::interpolate(p1,p2,thread->keyPos,&p);
         mat.setColumn(3,p);
         jtrans++;
      }

      if (thread->getSequence()->scaleMatters.test(nodeIndex))
      {
         if (thread->getSequence()->animatesUniformScale())
         {
            F32 s1 = mShape->getUniformScale(*thread->getSequence(),thread->keyNum1,jscale);
            F32 s2 = mShape->getUniformScale(*thread->getSequence(),thread->keyNum2,jscale);
            F32 scale = TSTransform::interpolate(s1,s2,thread->keyPos);
            TSTransform::applyScale(scale,&mat);
         }
         else if (animatesAlignedScale())
         {
            Point3F s1 = mShape->getAlignedScale(*thread->getSequence(),thread->keyNum1,jscale);
            Point3F s2 = mShape->getAlignedScale(*thread->getSequence(),thread->keyNum2,jscale);
            Point3F scale;
            TSTransform::interpolate(s1,s2,thread->keyPos,&scale);
            TSTransform::applyScale(scale,&mat);
         }
         else
         {
            TSScale s1,s2;
            mShape->getArbitraryScale(*thread->getSequence(),thread->keyNum1,jscale,&s1);
            mShape->getArbitraryScale(*thread->getSequence(),thread->keyNum2,jscale,&s2);
            TSScale scale;
            TSTransform::interpolate(s1,s2,thread->keyPos,&scale);
            TSTransform::applyScale(scale,&mat);
         }
         jscale++;
      }

      // apply blend transform
      smNodeLocalTransforms[nodeIndex].mul(mat);
      smNodeLocalTransformDirty.set(nodeIndex);
   }
}

//-------------------------------------------------------------------------------------
// Other Animation:
//-------------------------------------------------------------------------------------

void TSShapeInstance::animateVisibility(S32 ss)
{
   PROFILE_SCOPE( TSShapeInstance_animateVisibility );

   S32 i;
   if (!mMeshObjects.size())
      return;

   // find out who needs default values set
   TSIntegerSet beenSet;
   beenSet.setAll(mMeshObjects.size());
   for (i=0; i<mThreadList.size(); i++)
      beenSet.takeAway(mThreadList[i]->getSequence()->visMatters);

   // set defaults
   S32 a = mShape->subShapeFirstObject[ss];
   S32 b = a + mShape->subShapeNumObjects[ss];
   for (i=a; i<b; i++)
   {
      if (beenSet.test(i))
         mMeshObjects[i].visible = mShape->objectStates[i].vis;
   }

   // go through each thread and set visibility on those objects that
   // are not set yet and are controlled by that thread
   for (i=0; i<mThreadList.size(); i++)
   {
      TSThread * th = mThreadList[i];

      // For better or worse, object states are stored together (frame,
      // matFrame, visibility all in one structure).  Thus, indexing into
      // object state array for animation for any of these attributes needs to
      // take into account whether or not the other attributes are also animated.
      // The object states should eventually be separated (like the node states were)
      // in order to save memory and save the following step.
      TSIntegerSet objectMatters = th->getSequence()->frameMatters;
      objectMatters.overlap(th->getSequence()->matFrameMatters);
      objectMatters.overlap(th->getSequence()->visMatters);

      // skip to beginning of this sub-shape
      S32 j=0;
      S32 start = objectMatters.start();
      S32 end = b;
      for (S32 objectIndex = start; objectIndex<end; objectMatters.next(objectIndex), j++)
      {
         if (!beenSet.test(objectIndex) && th->getSequence()->visMatters.test(objectIndex))
         {
            F32 state1 = mShape->getObjectState(*th->getSequence(),th->keyNum1,j).vis;
            F32 state2 = mShape->getObjectState(*th->getSequence(),th->keyNum2,j).vis;
            if ((state1-state2) * (state1-state2) > 0.99f)
               // goes from 0 to 1 -- discreet jump
               mMeshObjects[objectIndex].visible = th->keyPos<0.5f ? state1 : state2;
            else
               // interpolate between keyframes when visibility change is gradual
               mMeshObjects[objectIndex].visible = (1.0f-th->keyPos) * state1 + th->keyPos * state2;

            // record change so that later threads don't over-write us...
            beenSet.set(objectIndex);
         }
      }
   }
}

void TSShapeInstance::animateFrame(S32 ss)
{
   PROFILE_SCOPE( TSShapeInstance_animateFrame );

   S32 i;
   if (!mMeshObjects.size())
      return;

   // find out who needs default values set
   TSIntegerSet beenSet;
   beenSet.setAll(mMeshObjects.size());
   for (i=0; i<mThreadList.size(); i++)
      beenSet.takeAway(mThreadList[i]->getSequence()->frameMatters);

   // set defaults
   S32 a = mShape->subShapeFirstObject[ss];
   S32 b = a + mShape->subShapeNumObjects[ss];
   for (i=a; i<b; i++)
      if (beenSet.test(i))
         mMeshObjects[i].frame = mShape->objectStates[i].frameIndex;

   // go through each thread and set frame on those objects that
   // are not set yet and are controlled by that thread
   for (i=0; i<mThreadList.size(); i++)
   {
      TSThread * th = mThreadList[i];

      // For better or worse, object states are stored together (frame,
      // matFrame, visibility all in one structure).  Thus, indexing into
      // object state array for animation for any of these attributes needs to
      // take into account whether or not the other attributes are also animated.
      // The object states should eventually be separated (like the node states were)
      // in order to save memory and save the following step.
      TSIntegerSet objectMatters = th->getSequence()->frameMatters;
      objectMatters.overlap(th->getSequence()->matFrameMatters);
      objectMatters.overlap(th->getSequence()->visMatters);

      // skip to beginning of this sub-shape
      S32 j=0;
      S32 start = objectMatters.start();
      S32 end = b;
      for (S32 objectIndex = start; objectIndex<end; objectMatters.next(objectIndex), j++)
      {
         if (!beenSet.test(objectIndex) && th->getSequence()->frameMatters.test(objectIndex))
         {
            S32 key = (th->keyPos<0.5f) ? th->keyNum1 : th->keyNum2;
            mMeshObjects[objectIndex].frame = mShape->getObjectState(*th->getSequence(),key,j).frameIndex;

            // record change so that later threads don't over-write us...
            beenSet.set(objectIndex);
         }
      }
   }
}

void TSShapeInstance::animateMatFrame(S32 ss)
{
   PROFILE_SCOPE( TSShapeInstance_animateMatFrame );

   S32 i;
   if (!mMeshObjects.size())
      return;

   // find out who needs default values set
   TSIntegerSet beenSet;
   beenSet.setAll(mMeshObjects.size());
   for (i=0; i<mThreadList.size(); i++)
      beenSet.takeAway(mThreadList[i]->getSequence()->matFrameMatters);

   // set defaults
   S32 a = mShape->subShapeFirstObject[ss];
   S32 b = a + mShape->subShapeNumObjects[ss];
   for (i=a; i<b; i++)
      if (beenSet.test(i))
         mMeshObjects[i].matFrame = mShape->objectStates[i].matFrameIndex;

   // go through each thread and set matFrame on those objects that
   // are not set yet and are controlled by that thread
   for (i=0; i<mThreadList.size(); i++)
   {
      TSThread * th = mThreadList[i];

      // For better or worse, object states are stored together (frame,
      // matFrame, visibility all in one structure).  Thus, indexing into
      // object state array for animation for any of these attributes needs to
      // take into account whether or not the other attributes are also animated.
      // The object states should eventually be separated (like the node states were)
      // in order to save memory and save the following step.
      TSIntegerSet objectMatters = th->getSequence()->frameMatters;
      objectMatters.overlap(th->getSequence()->matFrameMatters);
      objectMatters.overlap(th->getSequence()->visMatters);

      // skip to beginining of this sub-shape
      S32 j=0;
      S32 start = objectMatters.start();
      S32 end = b;
      for (S32 objectIndex = start; objectIndex<end; objectMatters.next(objectIndex), j++)
      {
         if (!beenSet.test(objectIndex) && th->getSequence()->matFrameMatters.test(objectIndex))
         {
            S32 key = (th->keyPos<0.5f) ? th->keyNum1 : th->keyNum2;
            mMeshObjects[objectIndex].matFrame = mShape->getObjectState(*th->getSequence(),key,j).matFrameIndex;

            // record change so that later threads don't over-write us...
            beenSet.set(objectIndex);
         }
      }
   }
}

//-------------------------------------------------------------------------------------
// Animate (and initialize detail levels)
//-------------------------------------------------------------------------------------

void TSShapeInstance::animate(S32 dl)
{
   PROFILE_SCOPE( TSShapeInstance_animate );

   if (dl==-1)
      // nothing to do
      return;

   S32 ss = mShape->details[dl].subShapeNum;

   // this is a billboard detail...
   if (ss<0)
      return;

   U32 dirtyFlags = mDirtyFlags[ss];

   if (dirtyFlags & ThreadDirty)
      sortThreads();

   // animate nodes?
   if (dirtyFlags & TransformDirty)
      animateNodes(ss);

   // animate objects?
   if (dirtyFlags & VisDirty)
      animateVisibility(ss);

   if (dirtyFlags & FrameDirty)
      animateFrame(ss);

   if (dirtyFlags & MatFrameDirty)
      animateMatFrame(ss);

   mDirtyFlags[ss] = 0;
}

void TSShapeInstance::animateNodeSubtrees(bool forceFull)
{
   // animate all the nodes for all the detail levels...

   if (forceFull)
      // force transforms to animate
      setDirty(TransformDirty);

   for (S32 i=0; i<mShape->subShapeNumNodes.size(); i++)
   {
      if (mDirtyFlags[i] & TransformDirty)
      {
         animateNodes(i);
         mDirtyFlags[i] &= ~TransformDirty;
      }
   }
}

void TSShapeInstance::animateSubtrees(bool forceFull)
{
   // animate all the subtrees

   if (forceFull)
      // force full animate
      setDirty(AllDirtyMask);

   for (S32 i=0; i<mShape->subShapeNumNodes.size(); i++)
   {
      if (mDirtyFlags[i] & TransformDirty)
      {
         animate(i);
         mDirtyFlags[i] = 0;
      }
   }
}

void TSShapeInstance::addPath(TSThread *gt, F32 start, F32 end, MatrixF *mat)
{
   // never get here while in transition...
   AssertFatal(!gt->transitionData.inTransition,"TSShapeInstance::addPath");

   if (!mat)
      mat = &mGroundTransform;

   MatrixF startInvM;
   gt->getGround(start,&startInvM);
   startInvM.inverse();

   MatrixF endM;
   gt->getGround(end,&endM);

   MatrixF addM;
   addM.mul(startInvM,endM);
   endM.mul(*mat,addM);
   *mat = endM;
}

bool TSShapeInstance::initGround()
{
   for (S32 i=0; i<mThreadList.size(); i++)
   {
      TSThread * th = mThreadList[i];
      if (!th->transitionData.inTransition && th->getSequence()->numGroundFrames>0)
      {
         mGroundThread = th;
         return true;
      }
   }
   return false;
}

void TSShapeInstance::animateGround()
{
   mGroundTransform.identity();

   // pick thread which controlls ground transform
   // if we haven't already...
   if (!mGroundThread && !initGround())
      return;

   S32 & loop    = mGroundThread->path.loop;
   F32 & start = mGroundThread->path.start;
   F32 & end   = mGroundThread->path.end;

   // accumulate path transform
   if (loop>0)
   {
      addPath(mGroundThread,start,1.0f);
      while (--loop)
         addPath(mGroundThread,0.0f,1.0f);
      addPath(mGroundThread,0.0f,end);
   }
   else if (loop<0)
   {
      addPath(mGroundThread,start,0.0f);
      while (++loop)
         addPath(mGroundThread,1.0f,0.0f);
      addPath(mGroundThread,1.0f,end);
   }
   else
      addPath(mGroundThread,start,end);
   start = end; // in case user tries to animateGround twice
}

void TSShapeInstance::deltaGround(TSThread * thread, F32 start, F32 end, MatrixF * mat)
{
   if (!mat)
      mat = &mGroundTransform;

   mat->identity();
   if (thread->transitionData.inTransition)
      return;

   F32 invDuration = 1.0f / thread->getDuration();
   start *= invDuration;
   end *= invDuration;

   addPath(thread,start,end,mat);
}

// Simple case of above- get ground delta at given position in unit range
void TSShapeInstance::deltaGround1(TSThread * thread, F32 start, F32 end, MatrixF& mat)
{
   mat.identity();
   if (thread->transitionData.inTransition)
      return;
   addPath(thread, start, end, &mat);
}

void TSShapeInstance::setTriggerState(U32 stateNum, bool on)
{
   AssertFatal(stateNum<=32 && stateNum>0,"TSShapeInstance::setTriggerState: state index out of range");

   stateNum--; // stateNum externally 1..32, internally 0..31
   U32 bit = 1 << stateNum;
   if (on)
      mTriggerStates |= bit;
   else
      mTriggerStates &= ~bit;
}

void TSShapeInstance::setTriggerStateBit(U32 stateBit, bool on)
{
   if (on)
      mTriggerStates |= stateBit;
   else
      mTriggerStates &= ~stateBit;
}

bool TSShapeInstance::getTriggerState(U32 stateNum, bool clearState)
{
   AssertFatal(stateNum<=32 && stateNum>0,"TSShapeInstance::getTriggerState: state index out of range");

   stateNum--; // stateNum externally 1..32, internally 0..31
   U32 bit = 1 << stateNum;
   bool ret = ((mTriggerStates & bit)!=0);
   if (clearState)
      mTriggerStates &= ~bit;
   return ret;
}

void TSShapeInstance::setNodeAnimationState(S32 nodeIndex, U32 animationState, TSCallback * callback)
{
   AssertFatal((animationState & ~(MaskNodeAll|MaskNodeHandsOff|MaskNodeCallback)) == 0,"TSShapeInstance::setNodeAnimationState (1)");

   // don't handle callback nodes in this method
   if (callback)
      animationState |= MaskNodeCallback;
   else
      animationState &= ~MaskNodeCallback;

   // hands-off takes precedance
   if (animationState & MaskNodeHandsOff)
      animationState = MaskNodeHandsOff | MaskNodeBlend;
   else if (animationState & MaskNodeCallback)
      animationState = MaskNodeCallback | MaskNodeBlend;

   // if we're not changing anything then get out of here now
   if (animationState == getNodeAnimationState(nodeIndex))
      return;

   setDirty(AllDirtyMask);

   if (animationState & MaskNodeAllButBlend)
   {
      if (animationState & MaskNodeRotation)
         mMaskRotationNodes.set(nodeIndex);
      if (animationState & MaskNodePosX)
         mMaskPosXNodes.set(nodeIndex);
      if (animationState & MaskNodePosY)
         mMaskPosYNodes.set(nodeIndex);
      if (animationState & MaskNodePosZ)
         mMaskPosZNodes.set(nodeIndex);
   }
   else
   {
      // no masking clear out all the masking lists
      mMaskRotationNodes.clear(nodeIndex);
      mMaskPosXNodes.clear(nodeIndex);
      mMaskPosYNodes.clear(nodeIndex);
      mMaskPosZNodes.clear(nodeIndex);
   }

   if (animationState & MaskNodeBlend)
      mDisableBlendNodes.set(nodeIndex);
   else
      mDisableBlendNodes.clear(nodeIndex);

   if (animationState & MaskNodeHandsOff)
      mHandsOffNodes.set(nodeIndex);
   else
      mHandsOffNodes.clear(nodeIndex);

   // clear out of node callbacks
   for (S32 i=0; i<mNodeCallbacks.size(); i++)
   {
      if (mNodeCallbacks[i].nodeIndex == nodeIndex)
      {
         mNodeCallbacks.erase_fast(i);
         break;
      }
   }

   if (animationState & MaskNodeCallback)
   {
      mCallbackNodes.set(nodeIndex);
      mNodeCallbacks.increment();
      mNodeCallbacks.last().callback = callback;
      mNodeCallbacks.last().nodeIndex = nodeIndex;
   }
   else
      mCallbackNodes.clear(nodeIndex);
}

U32 TSShapeInstance::getNodeAnimationState(S32 nodeIndex)
{
   U32 ret = 0;
   if (mMaskRotationNodes.test(nodeIndex))
      ret |= MaskNodeRotation;
   if (mMaskPosXNodes.test(nodeIndex))
      ret |= MaskNodePosX;
   if (mMaskPosYNodes.test(nodeIndex))
      ret |= MaskNodePosY;
   if (mMaskPosZNodes.test(nodeIndex))
      ret |= MaskNodePosZ;
   if (mDisableBlendNodes.test(nodeIndex))
      ret |= MaskNodeBlend;
   if (mHandsOffNodes.test(nodeIndex))
      ret |= MaskNodeHandsOff;
   if (mCallbackNodes.test(nodeIndex))
      ret |= MaskNodeCallback;
   return ret;
}


/*
void TSShapeInstance::animateHead()
{	
	static const F32 radiansPerSecond = M_PI_F / 3.0f;
	static const F32 percentageRemRot = 0.3f;
	
	if (!mUpdateTimer)
	{
		mUpdateTimer = PlatformTimer::create();
	}

	if (mShapeBase)
	{ 
		if (mIKnodes.empty())
		{
			// fill the static vector for the first and only time
			for (U32 i = 0; i < getShape()->nodes.size(); i++)
			{
				if (getShape()->getNodeName(i) == String("Bip001_Ponytail1") )
				{
					MyNode node;

					node.index = i;
					node.dof = EulerF(0.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); //Bip001_Ponytail1

					node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/2.0f, 0.0f, M_PI_F/2.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Head	

					/ *node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/3.0f, 0.0f, M_PI_F/3.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Neck2* /

					/ *node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/4.0f, 0.0f, M_PI_F/4.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Neck1* /

					break;
				}
			}
		} 
		else
		{
			MatrixF nodeTransform;
			Point3F rootPos,curEnd,desiredEnd;

			//Con::printf("Animating the head of %d", this);

			// POSITION OF THE END EFFECTOR
			getShape()->getNodeWorldTransform(mIKnodes[0].index, &nodeTransform);
			//nodeWorldTransform.mul(mShapeBase->getTransform(), nodeTransform);
			//curEnd = nodeWorldTransform.getPosition();
			curEnd = nodeTransform.getPosition();
			//curEnd = getShape()->node nodeTransform.getPosition();
			//curEnd = mNodeTransforms[mIKnodes[0].index].getPosition();

			// DESIRED END EFFECTOR POSITION
			TSStatic* mycube = reinterpret_cast<TSStatic *>(Sim::findObject("mycube"));
			if (!mycube)
			{
				return;
			}
			desiredEnd = mycube->getPosition();
			MatrixF shapeInvTransform = mShapeBase->getTransform();
			shapeInvTransform.inverse();
			shapeInvTransform.mulP(desiredEnd);

			F32 allowedRadiansPerFrame = (radiansPerSecond * (F32)mUpdateTimer->getElapsedMs() / 1000.0f);
			mUpdateTimer->reset(); // reset the timer

			for (int i = 1; i < mIKnodes.size(); i++)
			{
				Point3F curVector, targetVector;

				// THE COORDS OF THE X,Y,Z POSITION OF THE ROOT OF THIS BONE IS IN THE MATRIX
				// TRANSLATION PART WHICH IS IN THE 12,13,14 POSITION OF THE MATRIX

				getShape()->getNodeWorldTransform(mIKnodes[i].index, &nodeTransform);
				//nodeWorldTransform.mul(mShapeBase->getTransform(), nodeTransform);
				rootPos = nodeTransform.getPosition();
				//rootPos = mNodeTransforms[mIKnodes[i].index].getPosition();

				// CREATE THE VECTOR TO THE CURRENT EFFECTOR POS
				curVector = curEnd - rootPos;

				// CREATE THE DESIRED EFFECTOR POSITION VECTOR
				targetVector = desiredEnd - rootPos;

				// NORMALIZE THE VECTORS (EXPENSIVE, REQUIRES A SQRT)
				curVector.normalize();
				targetVector.normalize();

				// THE DOT PRODUCT GIVES ME THE COSINE OF THE DESIRED ANGLE
				auto cosAngle = mDot(curVector, targetVector);

				EulerF nodeCurrentRotation(0.0f);
				EulerF finalNeededRotation(0.0f);

				// IF THE DOT PRODUCT RETURNS 1.0, I DON'T NEED TO ROTATE AS IT IS 0 DEGREES
				if (cosAngle < 0.99999f && cosAngle > 0.1f)
				{
					auto angleBetween = [] (Point3F &v, Point3F &w, F32 sign) -> F32
					{
						v.normalize();
						w.normalize();

						auto cosAngle = mDot(v, w);

						Point3F crossResult = mCross(v, w);

						F32 factor = crossResult.x + crossResult.y + crossResult.z;

						if (factor < 0.0f)
						{
							return -mAcos(cosAngle) * sign;
						}
						else
						{
							return mAcos(cosAngle) * sign;
						}

					};


					Point3F curVectorX = curVector;
					curVectorX.x = 0.0f;

					Point3F curVectorY = curVector;
					curVectorY.y = 0.0f;

					Point3F curVectorZ = curVector;
					curVectorZ.z = 0.0f;

					Point3F targetVectorX = targetVector;
					targetVectorX.x = 0.0f;

					Point3F targetVectorY = targetVector;
					targetVectorY.y = 0.0f;

					Point3F targetVectorZ = targetVector;
					targetVectorZ.z = 0.0f;



					finalNeededRotation.x = angleBetween(curVectorZ, targetVectorZ, 1.0);
					//totalNeededRotation.y = angleBetween(curVectorY, targetVectorY, 1.0);
					finalNeededRotation.z = angleBetween(curVectorX, targetVectorX, -1.0);
				}
				else
				{
					finalNeededRotation.x = 0.0f;
					//totalNeededRotation.y = 0.0f;
					finalNeededRotation.z = 0.0f;
				}

				// Degree Of Freedom control
				finalNeededRotation.x = (finalNeededRotation.x > mIKnodes[i].dof.x) ? mIKnodes[i].dof.x : ((finalNeededRotation.x < -mIKnodes[i].dof.x) ? -mIKnodes[i].dof.x : finalNeededRotation.x);
				finalNeededRotation.y = (finalNeededRotation.y > mIKnodes[i].dof.y) ? mIKnodes[i].dof.y : ((finalNeededRotation.y < -mIKnodes[i].dof.y) ? -mIKnodes[i].dof.y : finalNeededRotation.y);
				finalNeededRotation.z = (finalNeededRotation.z > mIKnodes[i].dof.z) ? mIKnodes[i].dof.z : ((finalNeededRotation.z < -mIKnodes[i].dof.z) ? -mIKnodes[i].dof.z : finalNeededRotation.z);

				auto getRotationAngle = [&] (F32 finalNeededRotation, F32 lastRotation) -> F32
				{
					F32 rotDir = (finalNeededRotation < 0.0f) ? -1.0f : 1.0f ;

					F32 remainingRadians = finalNeededRotation - ( rotDir * allowedRadiansPerFrame + lastRotation);

					bool exceedsNeededRotation;

					if (finalNeededRotation < 0.0f)
					{
						exceedsNeededRotation = (rotDir * allowedRadiansPerFrame + lastRotation + remainingRadians * percentageRemRot > finalNeededRotation ) ? false : true;
					}
					else
					{
						exceedsNeededRotation = (rotDir * allowedRadiansPerFrame + lastRotation + remainingRadians * percentageRemRot < finalNeededRotation ) ? false : true;
					}

					if (! exceedsNeededRotation)
					{
						return (rotDir * allowedRadiansPerFrame + lastRotation);
					}
					else
					{
						return finalNeededRotation;
					}
				};

				/ *auto getRotationAngle = [&] (F32 finalNeededRotation, F32 lastRotation) -> F32
				{
					F32 rotDir = (finalNeededRotation < 0.0f) ? -1.0f : 1.0f ;

					F32 remainingRadians = finalNeededRotation - lastRotation;

					if (abs(remainingRadians) < allowedRadiansPerFrame)
					{
						return (remainingRadians + lastRotation);
					}
					else
					{
						return (rotDir * allowedRadiansPerFrame + lastRotation);
					}
				};* /

				nodeCurrentRotation.x = getRotationAngle(finalNeededRotation.x, mIKnodes[i].lastRotation.x);
				nodeCurrentRotation.z = getRotationAngle(finalNeededRotation.z, mIKnodes[i].lastRotation.z);

				// add the current rotation to the others
				mIKnodes[i].lastRotation = nodeCurrentRotation;

				// Apply the rotation to the current node
				smNodeCurrentRotations[mIKnodes[i].index] = QuatF(nodeCurrentRotation);						 
			}
		}		
	} 	
}*/


/*
void TSShapeInstance::animateHead()
{	
	static const F32 radiansPerSecond = M_PI_F / 4.0f;
	static const F32 percentageRemRot = 0.3f;
	
	if (!mUpdateTimer)
	{
		mUpdateTimer = PlatformTimer::create();
	}

	if (mShapeBase)
	{ 
		if (mIKnodes.empty())
		{
			// fill the static vector for the first and only time
			for (U32 i = 0; i < getShape()->nodes.size(); i++)
			{
				if (getShape()->getNodeName(i) == String("Bip001_Ponytail1") )
				{
					MyNode node;

					node.index = i;
					node.dof = EulerF(0.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); //Bip001_Ponytail1

					node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/2.0f, 0.0f, M_PI_F/2.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Head	

					node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/4.0f, 0.0f, M_PI_F/4.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Neck2

					/ *node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/8.0f, 0.0f, M_PI_F/2.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Neck1* /

					break;
				}
			}
		} 
		else
		{
			//MatrixF nodeTransform;
			Point3F desiredEnd = Point3F(0.0f);
			bool hasTarget = false;

			//Con::printf("Animating the head of %d", this);
			//Con::printf("Position of Bip001_Ponytail1 %f %f %f", curEnd.x, curEnd.y, curEnd.z);

			// DESIRED END EFFECTOR POSITION
			// Search for the nearest player to look at
			{			
				auto players = Player::getClientSidePlayers();
				F32 desiredEndDistance = std::numeric_limits<F32>::max();

				for (int i=0; i < players.size(); ++i)
				{
					if (players[i] != mShapeBase && players[i]->getPosition() != mShapeBase->getPosition())
					{
						F32 distance = Point3F(players[i]->getPosition() - mShapeBase->getPosition()).lenSquared();
						
						if (distance < desiredEndDistance)
						{
							desiredEnd = players[i]->getPosition();
							desiredEndDistance = distance;
							hasTarget = true;
						}
					}
				}
			}

			if (hasTarget)
			{
				// transform target position from world reference to shape position
				MatrixF shapeInvTransform = mShapeBase->getTransform();
				shapeInvTransform.inverse();
				shapeInvTransform.mulP(desiredEnd);
			}
			

			F32 allowedRadiansPerFrame = (radiansPerSecond * (F32)mUpdateTimer->getElapsedMs() / 1000.0f);
			mUpdateTimer->reset(); // reset the timer

			/ *for (int i = 0; i < mIKnodes.size(); i++)
			{
				mNodeTransforms[mIKnodes[i].index] = smNodeLocalTransforms[mIKnodes[i].index];
			}* /

			for (int i = 1; i < mIKnodes.size(); i++)
			{
				Point3F rootPos,curVector, targetVector,curEnd;
				EulerF nodeCurrentRotation(0.0f);
				EulerF finalNeededRotation(0.0f);

				if (hasTarget)
				{
					// POSITION OF THE END EFFECTOR
					curEnd = mNodeTransforms[mIKnodes[0].index].getPosition();

					// THE POSITION OF THE ROOT OF THIS BONE
					rootPos = mNodeTransforms[mIKnodes[i].index].getPosition();

					// CREATE THE VECTOR TO THE CURRENT EFFECTOR POS
					curVector = curEnd - rootPos;

					// CREATE THE DESIRED EFFECTOR POSITION VECTOR
					targetVector = desiredEnd - rootPos;

					// NORMALIZE THE VECTORS (EXPENSIVE, REQUIRES A SQRT)
					curVector.normalize();
					targetVector.normalize();

					// THE DOT PRODUCT GIVES ME THE COSINE OF THE DESIRED ANGLE
					auto cosAngle = mDot(curVector, targetVector);

					// IF THE DOT PRODUCT RETURNS 1.0, I DON'T NEED TO ROTATE AS IT IS 0 DEGREES
					if (cosAngle < 0.99999f && cosAngle > 0.1f)
					{
						auto angleBetween = [] (Point3F &v, Point3F &w, F32 sign) -> F32
						{
							v.normalize();
							w.normalize();

							auto cosAngle = mDot(v, w);

							Point3F crossResult = mCross(v, w);

							F32 factor = crossResult.x + crossResult.y + crossResult.z;

							if (factor < 0.0f)
							{
								return -mAcos(cosAngle) * sign;
							}
							else
							{
								return mAcos(cosAngle) * sign;
							}

						};


						Point3F curVectorX = curVector;
						curVectorX.x = 0.0f;

						Point3F curVectorY = curVector;
						curVectorY.y = 0.0f;

						Point3F curVectorZ = curVector;
						curVectorZ.z = 0.0f;

						Point3F targetVectorX = targetVector;
						targetVectorX.x = 0.0f;

						Point3F targetVectorY = targetVector;
						targetVectorY.y = 0.0f;

						Point3F targetVectorZ = targetVector;
						targetVectorZ.z = 0.0f;



						finalNeededRotation.x = angleBetween(curVectorZ, targetVectorZ, 1.0);
						//totalNeededRotation.y = angleBetween(curVectorY, targetVectorY, 1.0);
						finalNeededRotation.z = angleBetween(curVectorX, targetVectorX, -1.0);
					}
				}

				

				// Degree Of Freedom control
				/ *finalNeededRotation.x = (finalNeededRotation.x > mIKnodes[i].dof.x) ? mIKnodes[i].dof.x : ((finalNeededRotation.x < -mIKnodes[i].dof.x) ? -mIKnodes[i].dof.x : finalNeededRotation.x);
				//finalNeededRotation.y = (finalNeededRotation.y > mIKnodes[i].dof.y) ? mIKnodes[i].dof.y : ((finalNeededRotation.y < -mIKnodes[i].dof.y) ? -mIKnodes[i].dof.y : finalNeededRotation.y);
				finalNeededRotation.z = (finalNeededRotation.z > mIKnodes[i].dof.z) ? mIKnodes[i].dof.z : ((finalNeededRotation.z < -mIKnodes[i].dof.z) ? -mIKnodes[i].dof.z : finalNeededRotation.z);* /

				auto getRotationAngle = [&] (F32 finalNeededRotation, F32 lastRotation) -> F32
				{
					F32 rotDir = (finalNeededRotation < 0.0f) ? -1.0f : 1.0f ;

					F32 remainingRadians = finalNeededRotation - ( rotDir * allowedRadiansPerFrame + lastRotation);

					bool exceedsNeededRotation;

					if (finalNeededRotation < 0.0f)
					{
						exceedsNeededRotation = (rotDir * allowedRadiansPerFrame + lastRotation + remainingRadians * percentageRemRot > finalNeededRotation ) ? false : true;
					}
					else
					{
						exceedsNeededRotation = (rotDir * allowedRadiansPerFrame + lastRotation + remainingRadians * percentageRemRot < finalNeededRotation ) ? false : true;
					}

					if (! exceedsNeededRotation)
					{
						return (rotDir * allowedRadiansPerFrame + lastRotation);
					}
					else
					{
						return finalNeededRotation;
					}
				};

				nodeCurrentRotation.x = getRotationAngle(finalNeededRotation.x, mIKnodes[i].lastRotation.x);
				//nodeCurrentRotation.y = getRotationAngle(finalNeededRotation.y, mIKnodes[i].lastRotation.y);
				nodeCurrentRotation.z = getRotationAngle(finalNeededRotation.z, mIKnodes[i].lastRotation.z);

				// add the current rotation to the others
				mIKnodes[i].lastRotation = nodeCurrentRotation;


				// Apply the rotation to the current node
				/ *smNodeCurrentRotations[mIKnodes[i].index] = QuatF(nodeCurrentRotation);		
				TSTransform::setMatrix(smNodeCurrentRotations[mIKnodes[i].index], smNodeCurrentTranslations[mIKnodes[i].index], &smNodeLocalTransforms[mIKnodes[i].index]);
				mNodeTransforms[mIKnodes[i].index].mul(smNodeLocalTransforms[mShape->nodes[mIKnodes[i].index].parentIndex],smNodeLocalTransforms[mIKnodes[i].index]);

				// update transforms
				for (int j = i - 1; j >= 0; j--)
				{
					mNodeTransforms[mIKnodes[j].index].mul(mNodeTransforms[mIKnodes[j+1].index]/ *,mNodeTransforms[mIKnodes[j].index]* /);
				}* /

				// Apply the rotation to the current node
				smNodeCurrentRotations[mIKnodes[i].index] = QuatF(nodeCurrentRotation);		
				TSTransform::setMatrix(smNodeCurrentRotations[mIKnodes[i].index], smNodeCurrentTranslations[mIKnodes[i].index], &smNodeLocalTransforms[mIKnodes[i].index]);
				/ *mNodeTransforms[mIKnodes[i].index].mul(mNodeTransforms[mShape->nodes[mIKnodes[i].index].parentIndex],smNodeLocalTransforms[mIKnodes[i].index]);

				// update transforms
				for (int j = i - 1; j >= 0; j--)
				{
					mNodeTransforms[mIKnodes[j].index].mul(mNodeTransforms[mIKnodes[j+1].index],smNodeLocalTransforms[mIKnodes[j].index]);
				}* /
			}
		}		
	} 	
}*/



/*
void TSShapeInstance::animateHead()
{	
	static const F32 radiansPerSecond = M_PI_F / 4.0f;
	static const F32 percentageRemRot = 0.3f;
	static const F32 minTargetDistSquared = 200.0f;
	
	if (!mUpdateTimer)
	{
		mUpdateTimer = PlatformTimer::create();
	}

	if (mShapeBase)
	{ 
		if (mIKnodes.empty())
		{
			// fill the static vector for the first and only time
			for (U32 i = 0; i < getShape()->nodes.size(); i++)
			{
				if (getShape()->getNodeName(i) == String("Bip001_Ponytail1") )
				{
					MyNode node;

					node.index = i;
					node.dof = EulerF(0.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); //Bip001_Ponytail1

					node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/3.0f, 0.0f, M_PI_F/3.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Head	

					node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/3.0f, 0.0f, M_PI_F/3.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node); // Bip001_Neck2

					/ *node.index = mShape->nodes[node.index].parentIndex;
					node.dof = EulerF(M_PI_F/3.0f, 0.0f, M_PI_F/3.0f);
					node.lastRotation = EulerF(0.0f);
					mIKnodes.push_back(node);* / // Bip001_Neck1

					break;
				}
			}
		} 
		else
		{
			//MatrixF nodeTransform;
			Point3F desiredEnd = Point3F(0.0f);
			bool hasTarget = false;

			//Con::printf("Animating the head of %d", this);
			//Con::printf("Position of Bip001_Ponytail1 %f %f %f", curEnd.x, curEnd.y, curEnd.z);

			// Search for the nearest player to look at
			{			
				auto players = Player::getClientSidePlayers();
				F32 desiredEndDistance = std::numeric_limits<F32>::max();

				for (int i=0; i < players.size(); ++i)
				{
					if (players[i] != mShapeBase)
					{
						F32 distance = Point3F(players[i]->getPosition() - mShapeBase->getPosition()).lenSquared();
						
						if (distance < minTargetDistSquared && distance < desiredEndDistance)
						{
							Point3F shapeToPlayer = players[i]->getPosition() - mShapeBase->getTransform().getPosition();
							shapeToPlayer.normalizeSafe();
							Point3F shapeForward = mShapeBase->getTransform().getForwardVector();
							shapeForward.normalizeSafe();

							auto cosAngle = mDot(shapeToPlayer, shapeForward);

							// check if target is behind player
							if (cosAngle > 0.2f)
							{
								MatrixF playerEyeTransform;
								players[i]->getEyeTransform(&playerEyeTransform);
								Point3F playerEyePosition = playerEyeTransform.getPosition();

								desiredEnd = playerEyePosition;
								desiredEndDistance = distance;
								hasTarget = true;
							}							
						}
					}
				}
			}

			if (hasTarget)
			{
				// transform target position from world reference to shape position
				MatrixF shapeInvTransform = mShapeBase->getTransform();
				shapeInvTransform.inverse();
				shapeInvTransform.mulP(desiredEnd);
			}		
			else
			{
				int test = 0;
			}

			F32 allowedRadiansPerFrame = (radiansPerSecond * (F32)mUpdateTimer->getElapsedMs() / 1000.0f);
			mUpdateTimer->reset(); // reset the timer

			for (int i = 1; i < mIKnodes.size(); i++)
			{
				Point3F rootPos,curVector, targetVector,curEnd;
				EulerF nodeCurrentRotation(0.0f);
				EulerF finalNeededRotation(0.0f);

				if (hasTarget)
				{
					// POSITION OF THE END EFFECTOR
					curEnd = mNodeTransforms[mIKnodes[0].index].getPosition();

					// THE POSITION OF THE ROOT OF THIS BONE
					rootPos = mNodeTransforms[mIKnodes[i].index].getPosition();

					// CREATE THE VECTOR TO THE CURRENT EFFECTOR POS
					curVector = curEnd - rootPos;

					// CREATE THE DESIRED EFFECTOR POSITION VECTOR
					targetVector = desiredEnd - rootPos;

					Point3F curVectorX = curVector;
					curVectorX.x = 0.0f;

					Point3F curVectorY = curVector;
					curVectorY.y = 0.0f;

					Point3F curVectorZ = curVector;
					curVectorZ.z = 0.0f;

					Point3F targetVectorX = targetVector;
					targetVectorX.x = 0.0f;

					Point3F targetVectorY = targetVector;
					targetVectorY.y = 0.0f;

					Point3F targetVectorZ = targetVector;
					targetVectorZ.z = 0.0f;

					auto angleBetween = [] (Point3F &v, Point3F &w, F32 sign) -> F32
					{
						v.normalize();
						w.normalize();

						auto cosAngle = mDot(v, w);

						Point3F crossResult = mCross(v, w);

						F32 factor = crossResult.x + crossResult.y + crossResult.z;

						if (factor < 0.0f)
						{
							return -mAcos(cosAngle) * sign;
						}
						else
						{
							return mAcos(cosAngle) * sign;
						}

					};


					finalNeededRotation.x = angleBetween(curVectorZ, targetVectorZ, 1.0);
					finalNeededRotation.y = angleBetween(curVectorY, targetVectorY, -1.0);
					finalNeededRotation.z = angleBetween(curVectorX, targetVectorX, -1.0);
				}
				

				// Degree Of Freedom control
				finalNeededRotation.x = (finalNeededRotation.x > mIKnodes[i].dof.x) ? mIKnodes[i].dof.x : ((finalNeededRotation.x < -mIKnodes[i].dof.x) ? -mIKnodes[i].dof.x : finalNeededRotation.x);
				finalNeededRotation.y = (finalNeededRotation.y > mIKnodes[i].dof.y) ? mIKnodes[i].dof.y : ((finalNeededRotation.y < -mIKnodes[i].dof.y) ? -mIKnodes[i].dof.y : finalNeededRotation.y);
				finalNeededRotation.z = (finalNeededRotation.z > mIKnodes[i].dof.z) ? mIKnodes[i].dof.z : ((finalNeededRotation.z < -mIKnodes[i].dof.z) ? -mIKnodes[i].dof.z : finalNeededRotation.z);

				auto getRotationAngle = [&] (F32 finalNeededRotation, F32 lastRotation) -> F32
				{
					F32 rotDir = (finalNeededRotation < 0.0f) ? -1.0f : 1.0f ;

					/ *F32 remainingRadians = finalNeededRotation - ( rotDir * allowedRadiansPerFrame + lastRotation);* /

					bool exceedsNeededRotation;

					if (finalNeededRotation < 0.0f)
					{
						//exceedsNeededRotation = ((rotDir * allowedRadiansPerFrame + lastRotation + remainingRadians * percentageRemRot) > finalNeededRotation ) ? false : true;
						exceedsNeededRotation = ((rotDir * allowedRadiansPerFrame + lastRotation  ) > finalNeededRotation ) ? false : true;
					}
					else
					{
						//exceedsNeededRotation = ((rotDir * allowedRadiansPerFrame + lastRotation + remainingRadians * percentageRemRot) < finalNeededRotation ) ? false : true;
						exceedsNeededRotation = ((rotDir * allowedRadiansPerFrame + lastRotation ) < finalNeededRotation ) ? false : true;
					}

					if (! exceedsNeededRotation)
					{
						return (rotDir * allowedRadiansPerFrame + lastRotation);
					}
					else
					{
						return finalNeededRotation;
					}
				};

				nodeCurrentRotation.x = getRotationAngle(finalNeededRotation.x, mIKnodes[i].lastRotation.x);
				nodeCurrentRotation.y = getRotationAngle(finalNeededRotation.y, mIKnodes[i].lastRotation.y);
				nodeCurrentRotation.z = getRotationAngle(finalNeededRotation.z, mIKnodes[i].lastRotation.z);


				// Clamp values (because we never know)
				nodeCurrentRotation.x = mClampF(nodeCurrentRotation.x, -mIKnodes[i].dof.x, mIKnodes[i].dof.x );
				nodeCurrentRotation.y = mClampF(nodeCurrentRotation.y, -mIKnodes[i].dof.y, mIKnodes[i].dof.y );
				nodeCurrentRotation.z = mClampF(nodeCurrentRotation.z, -mIKnodes[i].dof.z, mIKnodes[i].dof.z );

				// add the current rotation to the others
				mIKnodes[i].lastRotation = nodeCurrentRotation;			

				// Apply the rotation to the current node
				smNodeCurrentRotations[mIKnodes[i].index] = QuatF(nodeCurrentRotation);		
				TSTransform::setMatrix(smNodeCurrentRotations[mIKnodes[i].index], smNodeCurrentTranslations[mIKnodes[i].index], &smNodeLocalTransforms[mIKnodes[i].index]);
				mNodeTransforms[mIKnodes[i].index].mul(mNodeTransforms[mShape->nodes[mIKnodes[i].index].parentIndex],smNodeLocalTransforms[mIKnodes[i].index]);

				// update transforms
				for (int j = i - 1; j >= 0; j--)
				{
					mNodeTransforms[mIKnodes[j].index].mul(mNodeTransforms[mIKnodes[j+1].index],smNodeLocalTransforms[mIKnodes[j].index]);
				}				
			}
		}		
	} 	
}*/