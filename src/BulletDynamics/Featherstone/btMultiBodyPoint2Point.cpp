/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2013 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

///This file was written by Erwin Coumans

#include "btMultiBodyPoint2Point.h"
#include "btMultiBodyLinkCollider.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

#ifndef BTMBP2PCONSTRAINT_BLOCK_ANGULAR_MOTION_TEST
	#define BTMBP2PCONSTRAINT_DIM 3
#else
	#define BTMBP2PCONSTRAINT_DIM 6
#endif

btMultiBodyPoint2Point::btMultiBodyPoint2Point(btMultiBody* body, int link, btRigidBody* bodyB, const btVector3& pivotInA, const btVector3& pivotInB)
	:btMultiBodyConstraint(body,0,link,-1,BTMBP2PCONSTRAINT_DIM,false),	
	m_rigidBodyA(0),
	m_rigidBodyB(bodyB),
	m_pivotInA(pivotInA),
	m_pivotInB(pivotInB)
{
}

btMultiBodyPoint2Point::btMultiBodyPoint2Point(btMultiBody* bodyA, int linkA, btMultiBody* bodyB, int linkB, const btVector3& pivotInA, const btVector3& pivotInB)
	:btMultiBodyConstraint(bodyA,bodyB,linkA,linkB,BTMBP2PCONSTRAINT_DIM,false),
	m_rigidBodyA(0),
	m_rigidBodyB(0),
	m_pivotInA(pivotInA),
	m_pivotInB(pivotInB)
{
}


btMultiBodyPoint2Point::~btMultiBodyPoint2Point()
{
}


int btMultiBodyPoint2Point::getIslandIdA() const
{
	if (m_rigidBodyA)
		return m_rigidBodyA->getIslandTag();

	if (m_bodyA)
	{
		btMultiBodyLinkCollider* col = m_bodyA->getBaseCollider();
		if (col)
			return col->getIslandTag();
		for (int i=0;i<m_bodyA->getNumLinks();i++)
		{
			if (m_bodyA->getLink(i).m_collider)
				return m_bodyA->getLink(i).m_collider->getIslandTag();
		}
	}
	return -1;
}

int btMultiBodyPoint2Point::getIslandIdB() const
{
	if (m_rigidBodyB)
		return m_rigidBodyB->getIslandTag();
	if (m_bodyB)
	{
		btMultiBodyLinkCollider* col = m_bodyB->getBaseCollider();
		if (col)
			return col->getIslandTag();

		for (int i=0;i<m_bodyB->getNumLinks();i++)
		{
			col = m_bodyB->getLink(i).m_collider;
			if (col)
				return col->getIslandTag();
		}
	}
	return -1;
}



void btMultiBodyPoint2Point::createConstraintRows(btMultiBodyConstraintArray& constraintRows,
		btMultiBodyJacobianData& data,
		const btContactSolverInfo& infoGlobal)
{

//	int i=1;
	for (int i=0;i<BTMBP2PCONSTRAINT_DIM;i++)
	{

		btMultiBodySolverConstraint& constraintRow = constraintRows.expandNonInitializing();

		constraintRow.m_solverBodyIdA = data.m_fixedBodyId;
		constraintRow.m_solverBodyIdB = data.m_fixedBodyId;
		
		btVector3 contactNormalOnB(0,0,0);
#ifndef BTMBP2PCONSTRAINT_BLOCK_ANGULAR_MOTION_TEST
		contactNormalOnB[i] = -1;
#else
		contactNormalOnB[i%3] = -1;
#endif

		btScalar penetration = 0;

		 // Convert local points back to world
		btVector3 pivotAworld = m_pivotInA;
		if (m_rigidBodyA)
		{
			
			constraintRow.m_solverBodyIdA = m_rigidBodyA->getCompanionId();
			pivotAworld = m_rigidBodyA->getCenterOfMassTransform()*m_pivotInA;
		} else
		{
			if (m_bodyA)
				pivotAworld = m_bodyA->localPosToWorld(m_linkA, m_pivotInA);
		}
		btVector3 pivotBworld = m_pivotInB;
		if (m_rigidBodyB)
		{
			constraintRow.m_solverBodyIdB = m_rigidBodyB->getCompanionId();
			pivotBworld = m_rigidBodyB->getCenterOfMassTransform()*m_pivotInB;
		} else
		{
			if (m_bodyB)
				pivotBworld = m_bodyB->localPosToWorld(m_linkB, m_pivotInB);
			
		}

		btScalar posError = i < 3 ? (pivotAworld-pivotBworld).dot(contactNormalOnB) : 0;

#ifndef BTMBP2PCONSTRAINT_BLOCK_ANGULAR_MOTION_TEST		
		

		fillMultiBodyConstraint(constraintRow, data, 0, 0,
															contactNormalOnB, pivotAworld, pivotBworld,						//sucks but let it be this way "for the time being"
															posError,
															infoGlobal,
															-m_maxAppliedImpulse, m_maxAppliedImpulse
															);
#else
		const btVector3 dummy(0, 0, 0);

		btAssert(m_bodyA->isMultiDof());

		btScalar* jac1 = jacobianA(i);
		const btVector3 &normalAng = i >= 3 ? contactNormalOnB : dummy;
		const btVector3 &normalLin = i < 3 ? contactNormalOnB : dummy;

		m_bodyA->filConstraintJacobianMultiDof(m_linkA, pivotAworld, normalAng, normalLin, jac1, data.scratch_r, data.scratch_v, data.scratch_m);

		fillMultiBodyConstraint(constraintRow, data, jac1, 0,
													dummy, dummy, dummy,						//sucks but let it be this way "for the time being"
													posError,
													infoGlobal,
													-m_maxAppliedImpulse, m_maxAppliedImpulse
													);
#endif
	}
}