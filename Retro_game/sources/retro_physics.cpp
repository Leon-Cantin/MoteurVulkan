#pragma once

#include "retro_physics.h"

#include <vector>

namespace phs
{
	State g_state;

	constexpr int playerObjectIndex = 5;

	void CreateState( btVector3 player_location, float player_collision_radius, const CollisionMesh& ground_plane )
	{
		g_state.collisionConfig = new btDefaultCollisionConfiguration();
		g_state.dispatcher = new btCollisionDispatcher( g_state.collisionConfig );
		g_state.overlappingPairCache = new btDbvtBroadphase();
		g_state.btWorld = new btCollisionWorld( g_state.dispatcher, g_state.overlappingPairCache, g_state.collisionConfig );

		g_state.mainCharacterCollisionShape = btSphereShape( player_collision_radius );
		g_state.mainCharacterCollisionObject = btCollisionObject();
		g_state.mainCharacterCollisionObject.setCollisionShape( &g_state.mainCharacterCollisionShape );
		g_state.mainCharacterCollisionObject.setUserIndex( playerObjectIndex );
		btTransform playerWorld;
		playerWorld.setIdentity();
		playerWorld.setOrigin( player_location );
		g_state.mainCharacterCollisionObject.setWorldTransform( playerWorld );
		g_state.btWorld->addCollisionObject( &g_state.mainCharacterCollisionObject, eCollisionGroups::PLAYER, eCollisionGroups::STATIC );

		btIndexedMesh groundMesh;
		groundMesh.m_indexType = PHY_INTEGER;
		groundMesh.m_numTriangles = ( int )ground_plane.indices.size() / 3;
		groundMesh.m_numVertices = ( int )ground_plane.vertices.size();
		groundMesh.m_triangleIndexBase = (unsigned char*)ground_plane.indices.data();
		groundMesh.m_triangleIndexStride = sizeof( uint32_t ) * 3;
		groundMesh.m_vertexBase = ( unsigned char* )ground_plane.vertices.data();
		groundMesh.m_vertexStride = sizeof( glm::vec3 );
		groundMesh.m_vertexType = PHY_FLOAT;

		g_state.groundVertexArray = btTriangleIndexVertexArray();
		g_state.groundVertexArray.addIndexedMesh( groundMesh );
		g_state.groundMeshShape = new btBvhTriangleMeshShape( &g_state.groundVertexArray, false );
		g_state.groundPlaneCollisionObject = btCollisionObject();
		g_state.groundPlaneCollisionObject.setCollisionShape( g_state.groundMeshShape );
		btTransform planeTransform;
		planeTransform.setIdentity();
		planeTransform.setOrigin( btVector3( 0.0f, 0.0f, 0.0f ) );
		g_state.groundPlaneCollisionObject.setWorldTransform( planeTransform );
		g_state.btWorld->addCollisionObject( &g_state.groundPlaneCollisionObject, eCollisionGroups::STATIC, eCollisionGroups::PLAYER );

		g_state.btWorld->setDebugDrawer( &g_state.bulletDebugDraw );
	}

	void Destroy()
	{
		delete g_state.btWorld;
		delete g_state.overlappingPairCache;
		delete g_state.dispatcher;
		delete g_state.collisionConfig;
		delete g_state.groundMeshShape;
	}

	class RayResultCallbackTyped : public btCollisionWorld::RayResultCallback
	{
		btVector3 m_hitNormalLocal;

	public:
		RayResultCallbackTyped( int group, int mask )
			: btCollisionWorld::RayResultCallback()
		{
			m_collisionFilterMask = mask;
			m_collisionFilterGroup = group;
		}

		/*virtual bool needsCollision( btBroadphaseProxy *proxy0 ) const
		{
			return proxy0->m_collisionFilterGroup & m_mask == m_mask && btCollisionWorld::RayResultCallback::needsCollision( proxy0 );
		}*/

		virtual btScalar addSingleResult( btCollisionWorld::LocalRayResult &rayResult, bool normalInWorldSpace )
		{
			m_collisionObject = rayResult.m_collisionObject;
			m_closestHitFraction = rayResult.m_hitFraction;
			m_hitNormalLocal = rayResult.m_hitNormalLocal;

			return m_closestHitFraction;
		}
	};

	void AlignPlayerToTerrain( float deltaTime, SceneInstance* mainCharacterSceneInstance )
	{
		RayResultCallbackTyped rayResultCallback( eCollisionGroups::PLAYER, eCollisionGroups::STATIC );
		btVector3 fromVector( mainCharacterSceneInstance->location.x, mainCharacterSceneInstance->location.y + 5.0f, mainCharacterSceneInstance->location.z );
		btVector3 toVector( mainCharacterSceneInstance->location.x, mainCharacterSceneInstance->location.y - 100.0f, mainCharacterSceneInstance->location.z );
		g_state.btWorld->rayTest( fromVector, toVector,	rayResultCallback );
		btVector3 hitPoint = fromVector + ( toVector - fromVector ) * rayResultCallback.m_closestHitFraction;
		btVector3 alignedToGround = hitPoint + btVector3( 0.0f, 0.5f, 0.0f );

		mainCharacterSceneInstance->location.x = alignedToGround.x();
		mainCharacterSceneInstance->location.y = alignedToGround.y();
		mainCharacterSceneInstance->location.z = alignedToGround.z();
	}

	void Update( float deltaTime, SceneInstance* mainCharacterSceneInstance )
	{
		g_state.mainCharacterCollisionObject.getWorldTransform().setOrigin( btVector3( mainCharacterSceneInstance->location.x, mainCharacterSceneInstance->location.y, mainCharacterSceneInstance->location.z ) );

		AlignPlayerToTerrain( deltaTime, mainCharacterSceneInstance );

		/*bool contact = false;
		btCollisionWorld* btWorld = g_state.btWorld;
		btWorld->performDiscreteCollisionDetection();
		const auto numManifolds = btWorld->getDispatcher()->getNumManifolds();
		for( int i = 0; i < numManifolds; i++ ) {
			btPersistentManifold* contactManifold = btWorld->getDispatcher()->getManifoldByIndexInternal( i );
			const auto numContacts = contactManifold->getNumContacts();
			if( numContacts )
			{
				const btManifoldPoint& contactPoint = contactManifold->getContactPoint( 0 );
				mainCharacterSceneInstance->location.y += 0.5f - abs( contactPoint.m_localPointA.getY() );
				contact = true;
			}
		}

		if( !contact )
		{
			mainCharacterSceneInstance->location.y -= 0.5f * (deltaTime / 1000.0f);
		}*/
	}

	void BeginDebugDraw()
	{
		g_state.bulletDebugDraw.setDebugMode( btIDebugDraw::DBG_DrawWireframe );
		g_state.btWorld->debugDrawWorld();
	}
}
