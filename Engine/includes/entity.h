#pragma once

#include <cstdint>
#include <array>
#include "memory.h"

namespace ECS
{
	typedef uint32_t EntityID;
	typedef uint32_t ComponentID;
	constexpr ComponentID INVALID_COMPONENT_ID = std::numeric_limits<ComponentID>::max();
	typedef uint16_t ComponentTypeID;
	typedef uint16_t SingletonComponentTypeID;

	template<class T, class T_Counter>
	class TypeIdGenerator
	{
	public:

		static inline T_Counter m_count = 0;

	public:

		template<class U>
		static T_Counter GetID()
		{
			static const T_Counter idCounter = m_count++;
			assert( idCounter < std::numeric_limits<T_Counter>::max() ); //Will this even work?
			return idCounter;
		}

		static T_Counter GetCurrentCount()
		{
			static const T_Counter count = m_count;
			return count;
		}
	};

	class ComponentBase
	{};

	typedef TypeIdGenerator<ComponentBase, ComponentTypeID> ComponentTypeIDGenerator_t;

	class SingletonComponentBase
	{};

	typedef TypeIdGenerator<SingletonComponentBase, SingletonComponentTypeID> SingletonComponentTypeIDGenerator_t;

	#define REGISTER_COMPONENT_TYPE( type__ ) static const auto type__ ## _comp_t = ECS::ComponentTypeIDGenerator_t::GetID<type__>()
	#define REGISTER_SINGLETON_COMPONENT_TYPE( type__ ) static const auto type__ ## _singleton_comp_t = ECS::SingletonComponentTypeIDGenerator_t::GetID<type__>()


	class ArchetypeKey
	{
	private:
		//TODO: I could do something much better with a bit array
		std::vector< ComponentTypeID > componentTypeIds;

	public:

		template< typename ... ComponentTypeIds>
		ArchetypeKey( ComponentTypeIds ... ids )
			:componentTypeIds { ids ... }
		{
			//Sort them so we don't end up with multiple archetypes if we mix the components
			std::sort( componentTypeIds.begin(), componentTypeIds.end() );
		}


		template< typename ... ComponentTypes >
		static ArchetypeKey Create()
		{
			ArchetypeKey key( ComponentTypeIDGenerator_t::GetID< ComponentTypes >() ... );
			return key;
		}

		bool operator==( const ArchetypeKey &other ) const
		{
			const size_t idsCount = componentTypeIds.size();
			if( idsCount != other.componentTypeIds.size() )
				return false;

			for( size_t i = 0; i < idsCount; ++i )
				if( componentTypeIds[i] != other.componentTypeIds[i] )
					return false;

			return true;
		}

		bool Contains( ComponentTypeID id ) const
		{
			return std::find( componentTypeIds.begin(), componentTypeIds.end(), id ) != componentTypeIds.end();
		}

		template< typename ... T >
		bool Contains( ComponentTypeID id, T ... ids ) const
		{
			return Contains( id ) && Contains( ids ... );
		}

		bool Contains( const ArchetypeKey& other ) const
		{
			for( ComponentTypeID otherComponentTypeId : other.componentTypeIds )
				if( !Contains( otherComponentTypeId ) )
					return false;

			return true;
		}
	};

	class Archetype
	{
	private:
		ArchetypeKey key;
		std::vector< std::pair< ComponentID, ComponentTypeID > > componentsIds;

	public:
		template< typename ... componentIds >
		Archetype( ArchetypeKey && key, componentIds ... ids )
			: key( std::move( key ) ), componentsIds{ ids ... }
		{
		}

		Archetype()
		{
		}

		template< typename ... ComponentTypes, typename ... componentIds >
		static Archetype Create( componentIds ... ids )
		{
			return Archetype( ArchetypeKey::Create<ComponentTypes ...>(), std::pair< ComponentID, ComponentTypeID >( ids, ComponentTypeIDGenerator_t::GetID<ComponentTypes>() ) ... );
		}

		ComponentID GetComponentID( ComponentTypeID typeId ) const
		{
			auto it = std::find_if( componentsIds.begin(), componentsIds.end(),
				[typeId]( const std::pair< ComponentID, ComponentTypeID >& pair ) { return pair.second == typeId; }
			);
			return it != componentsIds.end() ? it->first : INVALID_COMPONENT_ID;
		}

		template< typename C >
		ComponentID GetComponentID() const
		{
			const ComponentTypeID typeId = ComponentTypeIDGenerator_t::GetID<C>();
			return GetComponentID(typeId);
		}

		size_t GetAllComponentIDsAndTypes( const std::pair< ComponentID, ComponentTypeID >** o_componentIds ) const
		{
			*o_componentIds = componentsIds.data();
			return componentsIds.size();
		}

		const ArchetypeKey& GetKey()
		{
			return key;
		}
	};

	class EntityContainer
	{
	private:
		static constexpr size_t MAX_ENTITIES = 256;
		std::vector< Archetype > entities_entries;
		std::vector<bool> data_used;
		uint32_t entities_count;

	private:
		const Archetype& GetEntry( EntityID entityId ) const
		{
			assert( entityId < entities_entries.size() && data_used[entityId] );
			return entities_entries[entityId];
		}

	public:
		EntityContainer()
			: entities_count(0)
		{
			entities_entries.resize( MAX_ENTITIES );
			data_used.resize( MAX_ENTITIES );
		}

		template< typename ... ComponentTypes, typename ... ComponentIDs >
		EntityID CreateEntity( ComponentIDs ... componentIds )
		{
			EntityID freeSlotIndex = 0;
			while( freeSlotIndex < data_used.size() && data_used[freeSlotIndex] )
				++freeSlotIndex;
			if( freeSlotIndex >= data_used.size() )
				throw std::runtime_error( "Out of entity slots" );

			const EntityID entityId = freeSlotIndex;
			data_used[entityId] = true;
			entities_entries[entityId] = Archetype::Create< ComponentTypes ... >( componentIds ... );

			++entities_count;

			return entityId;
		}

		void RemoveEntity( EntityID entityId )
		{
			assert( entityId < entities_entries.size() && data_used[entityId] );
			data_used[entityId] = false;
			--entities_count;
		}

		template< class C >
		ComponentID GetComponentId( EntityID entityId ) const
		{
			const Archetype& entry = GetEntry( entityId );
			return entry.GetComponentID<C>();
		}

		size_t GetAllComponentIDsAndTypes( EntityID entityId, const std::pair< ComponentID, ComponentTypeID >** o_componentIds ) const
		{
			const Archetype& entry = GetEntry( entityId );
			return entry.GetAllComponentIDsAndTypes( o_componentIds );
		}

		uint32_t GetEntitiesWithKey( const ArchetypeKey& reference_key, EntityID* o_ids, uint32_t ids_max )
		{
			uint32_t used_entities_count = 0;
			uint32_t candidates_count = 0;
			for( uint32_t i = 0 ; i < data_used.size() && used_entities_count < entities_count && candidates_count < ids_max; ++i )
			{
				if( data_used[i] )
				{
					++used_entities_count;
					if( entities_entries[i].GetKey().Contains( reference_key ) )
					{
						assert( candidates_count < MAX_ENTITIES );
						const EntityID entityId = i;
						o_ids[candidates_count++] = entityId;
					}
				}
			}

			return candidates_count;
		}
	};

	class ComponentContainer
	{
	private:
		//TODO: with this implementation you can't find with just the componentID, you also need the type
		std::vector< void* > data;
		std::vector< size_t > data_sizes;
		std::vector< ComponentID > data_counts;

		std::vector<std::vector<bool>> data_used;

	public:
		ComponentContainer()
		{
			const size_t componentTypesCount = ComponentTypeIDGenerator_t::GetCurrentCount();
			assert( componentTypesCount > 0 );

			data.resize( componentTypesCount );
			MEM::zero( data.data(), data.size() * sizeof( void* ) );
			data_sizes.resize( componentTypesCount );
			MEM::zero( data_sizes.data(), data_sizes.size() * sizeof( size_t ) );
			data_counts.resize( componentTypesCount );
			MEM::zero( data_counts.data(), data_counts.size() * sizeof( ComponentID ) );

			data_used.resize( componentTypesCount );
		}

		~ComponentContainer()
		{
			for( void*& ptr : data )
			{
				free( ptr );
				ptr = nullptr;
			}
		}

		template<class C, class ... Args>
		ComponentID CreateComponent( Args ... componentArgs )
		{
			constexpr ComponentID MaxComponents = 256;
			const ComponentTypeID typeId = ComponentTypeIDGenerator_t::GetID<C>();
			assert( typeId < ComponentTypeIDGenerator_t::GetCurrentCount() && typeId < data.size() );

			if( data[typeId] == nullptr )
			{
				//Create new componentType
				data[typeId] = malloc( sizeof( C ) * MaxComponents );
				MEM::zero( data[typeId], sizeof( C ) * MaxComponents );//We must zero mem for the assignation below.
				data_sizes[typeId] = sizeof( C );
				data_used[typeId].resize( MaxComponents );
			}

			data_counts[typeId]++;

			ComponentID freeSlotIndex = 0;
			while( freeSlotIndex < data_used[typeId].size() && data_used[typeId][freeSlotIndex] )
				++freeSlotIndex;
			assert( !data_used[typeId][freeSlotIndex] );
			if( freeSlotIndex >= data_used[typeId].size() )
				throw std::runtime_error( "Out of component slots" );

			data_used[typeId][freeSlotIndex] = true;
			const ComponentID newComponentId = freeSlotIndex;

			C* c_data = reinterpret_cast< C* >(data[typeId]);
			c_data[newComponentId] = { componentArgs ... };

			return newComponentId;
		}

		void DestroyComponent( ComponentTypeID typeId, ComponentID componentId )
		{
			assert( typeId < ComponentTypeIDGenerator_t::GetCurrentCount() );
			assert( componentId < data_used[typeId].size() && data_used[typeId][componentId] );

			--data_counts[typeId];
			data_used[typeId][componentId] = false;
		}

		template<class C>
		void DestroyComponent( ComponentID componentId )
		{
			const ComponentTypeID typeId = ComponentTypeIDGenerator_t::GetID<C>();
			DestroyComponent( typeId, componentId );
		}

		template< class C >
		C* GetComponent( ComponentID componentId ) const
		{
			const ComponentTypeID typeId = ComponentTypeIDGenerator_t::GetID<C>();
			C* c_data = reinterpret_cast< C* >(data[typeId]);
			return &c_data[componentId];
		}

		void* GetComponent( ComponentID componentId, ComponentTypeID componentTypeId ) const
		{
			const size_t dataSize = data_sizes[componentTypeId];
			uint8_t* c_data = reinterpret_cast< uint8_t* >( data[componentTypeId] );
			return &c_data[ dataSize * componentId ];
		}
	};

	class SingletonComponentContainer
	{
	private:
		std::vector< void* > data;
		std::vector< size_t > data_sizes;

	public:
		SingletonComponentContainer()
		{
			const size_t componentTypesCount = SingletonComponentTypeIDGenerator_t::GetCurrentCount();
			assert( componentTypesCount > 0 );

			data.resize( componentTypesCount );
			MEM::zero( data.data(), data.size() * sizeof( void* ) );
			data_sizes.resize( componentTypesCount );
			MEM::zero( data_sizes.data(), data_sizes.size() * sizeof( size_t ) );
		}

		~SingletonComponentContainer()
		{
			for( void*& ptr : data )
			{
				free( ptr );
				ptr = nullptr;
			}
		}

		template<class C, class ... Args>
		SingletonComponentTypeID CreateComponent( Args ... componentArgs )
		{
			const SingletonComponentTypeID typeId = SingletonComponentTypeIDGenerator_t::GetID<C>();
			assert( typeId < SingletonComponentTypeIDGenerator_t::GetCurrentCount() && typeId < data.size() );
			assert( data[typeId] == nullptr );

			//Create new componentType
			data[typeId] = malloc( sizeof( C ) );
			data_sizes[typeId] = sizeof( C );

			MEM::zero( data[typeId], sizeof( C ) );

			C* c_data = reinterpret_cast< C* >(data[typeId]);
			*c_data = { componentArgs ... };

			return typeId;
		}

		void* GetComponent( SingletonComponentTypeID singletonComponentTypeId ) const
		{
			assert( singletonComponentTypeId < data.size() );
			return data[singletonComponentTypeId];
		}

		template< class C >
		C* GetComponent() const
		{
			const SingletonComponentTypeID typeId = SingletonComponentTypeIDGenerator_t::GetID<C>();
			C* c_data = reinterpret_cast< C* >(GetComponent( typeId ));
			return c_data;
		}


	};

	template< typename C >
	class ComponentHandle
	{
	private:
		const ComponentContainer* m_componentContainer;
		ComponentID m_componentId;

	public:
		ComponentHandle( const ComponentContainer* componentContainer, ComponentID componentId )
			: m_componentContainer( componentContainer ), m_componentId( componentId )
		{}

		C* GetComponent()
		{
			return m_componentContainer->GetComponent<C>( m_componentId );
		}

		const C* GetComponent() const
		{
			return const_cast<ComponentHandle*>(this)->GetComponent();
		}

		ComponentID GetComponentId() const
		{
			return m_componentId;
		}
	};

	template< typename C >
	class SingletonComponentHandle
	{
	private:
		const SingletonComponentContainer* m_componentContainer;

	public:
		SingletonComponentHandle( const SingletonComponentContainer* componentContainer )
			: m_componentContainer( componentContainer )
		{}

		C* GetComponent()
		{
			return m_componentContainer->GetComponent<C>();
		}

		const C* GetComponent() const
		{
			return const_cast<SingletonComponentHandle*>(this)->GetComponent();
		}
	};

	class GenericComponentHandle
	{
	private:
		const ComponentContainer* m_componentContainer;
		ComponentID m_componentId;
		ComponentTypeID m_componentTypeId;

	public:
		GenericComponentHandle( const ComponentContainer* componentContainer, ComponentID componentId, ComponentTypeID componentTypeId )
			: m_componentContainer( componentContainer ), m_componentId( componentId ), m_componentTypeId( componentTypeId )
		{}

		void* GetComponent()
		{
			return m_componentContainer->GetComponent( m_componentId, m_componentTypeId );
		}

		const void* GetComponent() const
		{
			return GetComponent();
		}

		ComponentID GetComponentId() const
		{
			return m_componentId;
		}
	};

	class EntityComponentContainer
	{
	private:
		ComponentContainer componentContainer;
		EntityContainer entityContainer;
		
	public:
		template<class C, class ... Args>
		ComponentHandle<C> CreateComponent( Args ... componentArgs )
		{
			const ComponentID componentId = componentContainer.CreateComponent<C>( componentArgs ... );
			return ComponentHandle<C>( &componentContainer, componentId );
		}

		template< template< typename > typename ComponentHandles, typename ... ComponentTypes >
		EntityID CreateEntity( ComponentHandles<ComponentTypes>& ... componentHandles )
		{
			return entityContainer.CreateEntity< ComponentTypes ... >( componentHandles.GetComponentId() ... );
		}

		template< class C >
		C* GetComponent( ComponentID componentId ) const
		{
			return componentContainer.GetComponent< C >( componentId );
		}

		void* GetComponent( ComponentID componentId, ComponentTypeID componentTypeId ) const
		{
			return componentContainer.GetComponent( componentId, componentTypeId );
		}

		template< class C >
		C* GetComponentForEntity( EntityID entityId ) const
		{
			const ComponentID componentId = entityContainer.GetComponentId< C >( entityId );
			return componentId != INVALID_COMPONENT_ID ? componentContainer.GetComponent< C >( componentId ) : nullptr;
		}

		void Destroy( EntityID entityId )
		{
			const std::pair< ComponentID, ComponentTypeID >* componentIds;

			const size_t count = entityContainer.GetAllComponentIDsAndTypes( entityId, &componentIds );
			for( size_t i = 0; i < count; ++i )
			{
				componentContainer.DestroyComponent( componentIds[i].second, componentIds[i].first );
			}
			entityContainer.RemoveEntity( entityId );
		}

		uint32_t GetEntitiesWithKey( const ArchetypeKey& reference_key, EntityID* o_ids, uint32_t ids_max )
		{
			return entityContainer.GetEntitiesWithKey( reference_key, o_ids, ids_max);
		}
	};

	class Entity
	{
	private:
		EntityID m_entityId;
		EntityComponentContainer* m_parent_ecc;
		static constexpr EntityID INVALID_ENTITY_ID = std::numeric_limits<EntityID>::max();

	public:
		Entity()
			: m_entityId( INVALID_ENTITY_ID ), m_parent_ecc( nullptr )
		{

		}

		Entity( EntityID entityId, class EntityComponentContainer* parent_ecc )
			: m_entityId( entityId ), m_parent_ecc( parent_ecc )
		{

		}

		EntityID GetId() const
		{
			return m_entityId;
		}

		template< class C >
		C* GetComponent() const
		{
			assert( IsValid() );
			return m_parent_ecc->GetComponentForEntity<C>( m_entityId );
		}

		bool IsValid() const
		{
			return m_entityId != INVALID_ENTITY_ID;
		}
	};


	class System
	{
	private:
		ArchetypeKey m_key;

		ComponentContainer* m_componentContainer;
		EntityContainer* m_entityContainer;

	public:
		void( *m_update ) ( Entity*, uint32_t, class EntityComponentSystem* );

		System( ArchetypeKey key, void( *update ) ( Entity*, uint32_t, class EntityComponentSystem* ) )
			: m_key( key ), m_update( update )
		{
		}

		template< typename ... ComponentTypes >
		static System Create( void( *func_update ) ( Entity*, uint32_t, class EntityComponentSystem* ) )
		{
			return System( ArchetypeKey::Create< ComponentTypes ... >(), func_update );
		}

		void SetContainers( ComponentContainer* componentContainer, EntityContainer* entityContainer )
		{
			m_componentContainer = componentContainer;
			m_entityContainer = entityContainer;
		}

		bool CanRun( const ArchetypeKey& key ) const
		{
			return key.Contains( m_key );
		}

		const ArchetypeKey& GetKey() const
		{
			return m_key;
		}
	};

	class EntityComponentSystem
	{
	private:
		SingletonComponentContainer singletonComponentContainer;
		EntityComponentContainer entityComponentContainer;

	public:
		template<class C, class ... Args>
		ComponentHandle<C> CreateComponent( Args ... componentArgs )
		{
			return entityComponentContainer.CreateComponent<C>( componentArgs ... );
		}

		template<class C, class ... Args>
		SingletonComponentHandle<C> CreateSingletonComponent( Args ... componentArgs )
		{
			singletonComponentContainer.CreateComponent<C>( componentArgs ... );
			return SingletonComponentHandle<C>( &singletonComponentContainer );
		}

		//TODO: Modify to create the components and then create the entity
		/*template< typename ... ComponentTypes, typename ... ComponentIDs >
		Entity CreateEntity( ComponentIDs ... componentIds )
		{
			return entityContainer.CreateEntity< ComponentTypes ... >( this, componentIds ... );
		}*/

		template< template< typename > typename ComponentHandles, typename ... ComponentTypes >
		Entity CreateEntity( ComponentHandles<ComponentTypes>& ... componentHandles )
		{
			return Entity { entityComponentContainer.CreateEntity( componentHandles ... ), &entityComponentContainer };
		}

		template< class C >
		C* GetComponent( ComponentID componentId ) const
		{
			return entityComponentContainer.GetComponent< C >( componentId );
		}

		void* GetComponent( ComponentID componentId, ComponentTypeID componentTypeId ) const
		{
			return entityComponentContainer.GetComponent( componentId, componentTypeId );
		}

		template< class C >
		C* GetSingletonComponent() const
		{
			return singletonComponentContainer.GetComponent< C >();
		}

		template< class C >
		C* GetComponentForEntity( EntityID entityId ) const
		{
			return entityComponentContainer.GetComponentForEntity<C>( entityId );
		}

		void Destroy( Entity* entity )
		{
			if( entity->IsValid() )
			{
				entityComponentContainer.Destroy( entity->GetId() );
				*entity = Entity();
			}
		}

		void RunSystem( const System& system )
		{
			EntityID entitiesIds[256];
			uint32_t entities_count = entityComponentContainer.GetEntitiesWithKey(system.GetKey(), entitiesIds, 256);

			Entity entities [256];
			for( uint32_t i = 0; i < entities_count; ++i )
				entities[i] = Entity( entitiesIds[i], &entityComponentContainer );

			system.m_update( entities, entities_count, this );
		}
	};
}