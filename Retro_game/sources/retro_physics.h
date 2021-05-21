#pragma once

#include "btBulletCollisionCommon.h"
#include "glm/glm.hpp"
#include "scene_instance.h"

#include <vector>

namespace phs
{
	struct CollisionMesh
	{
		std::vector<glm::vec3> vertices;
		std::vector<uint32_t> indices;
	};

	enum eCollisionGroups : int
	{
		NONE = 1 << 0,
		STATIC = 1 << 1,
		PLAYER = 1 << 2,
	};

	class BulletDebugDraw : public btIDebugDraw
	{
		int m_debug_mode;
		void drawLine( const btVector3& from, const btVector3& to, const btVector3& color ) override;
		void drawContactPoint( const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color ) override;
		void reportErrorWarning( const char* warningString ) override;
		void draw3dText( const btVector3& location, const char* textString ) override;
	public:
		void setDebugMode( int debugMode ) override;
		int getDebugMode() const override;
	};

	struct State
	{
		btDefaultCollisionConfiguration* collisionConfig;
		btCollisionDispatcher* dispatcher;
		btDbvtBroadphase* overlappingPairCache;
		btCollisionWorld* btWorld;

		btSphereShape mainCharacterCollisionShape;
		btCollisionObject mainCharacterCollisionObject;

		btTriangleIndexVertexArray groundVertexArray;
		btBvhTriangleMeshShape* groundMeshShape;
		btCollisionObject groundPlaneCollisionObject;
		BulletDebugDraw bulletDebugDraw;


		State()
		:	mainCharacterCollisionShape( 0.0f )
		{}
	};

	void CreateState( btVector3 player_location, float player_collision_radius, const CollisionMesh& ground_plane );
	void Destroy();
	void Update( float deltaTime, SceneInstance* mainCharacterSceneInstance );

	void BeginDebugDraw();
}
