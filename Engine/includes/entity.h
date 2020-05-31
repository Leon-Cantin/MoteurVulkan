#pragma once

#include <cstdint>
#include <array>

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

	template<class C>
	class Component : public ComponentBase
	{
	private:

		static inline const ComponentTypeID m_typeId = ComponentTypeIDGenerator_t::GetID<C>();

	public:

		static ComponentTypeID GetTypeId()
		{
			return m_typeId;
		}
	};

	//Force Component type IDs to be generated
	/*template< typename T >
	Component<T> RegisterComponentType()
	{
		static const Component<T> comp;
		return comp;
	}*/

	template< typename T >
	struct RegisterComponentType
	{
		static const Component<T> comp;
	};

	//TODO Singleton copies too much from component
	class SingletonComponentBase
	{};

	typedef TypeIdGenerator<SingletonComponentBase, SingletonComponentTypeID> SingletonComponentTypeIDGenerator_t;

	template<class C>
	class SingletonComponent : public SingletonComponentBase
	{
	private:

		static inline const SingletonComponentTypeID m_typeId = SingletonComponentTypeIDGenerator_t::GetID<C>();

	public:

		static SingletonComponentTypeID GetTypeId()
		{
			return m_typeId;
		}
	};

	template< typename T >
	struct RegisterSingletonComponentType
	{
		static const SingletonComponent<T> comp;
	};

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

	class Entity
	{
	private:
		EntityID m_entityId;
		class EntityComponentSystem* m_parent_ecs;
		static constexpr EntityID INVALID_ENTITY_ID = std::numeric_limits<EntityID>::max();

	public:
		Entity()
			: m_entityId( INVALID_ENTITY_ID ), m_parent_ecs( nullptr )
		{

		}

		Entity( EntityID entityId, class EntityComponentSystem* parent_ecs )
			: m_entityId( entityId ), m_parent_ecs( parent_ecs )
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
			return m_parent_ecs->GetComponentForEntity<C>( m_entityId );
		}

		bool IsValid() const
		{
			return m_entityId != INVALID_ENTITY_ID;
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
		Entity CreateEntity( class EntityComponentSystem* ecs, ComponentIDs ... componentIds )
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

			return Entity { entityId, ecs };
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

		void ForEachEntityWithKey( const ArchetypeKey& reference_key, class EntityComponentSystem* ecs, void( *func )( Entity*, uint32_t, EntityComponentSystem* ) )
		{
			Entity entities [MAX_ENTITIES];
			uint32_t used_entities_count = 0;
			uint32_t candidates_count = 0;
			for( uint32_t i = 0 ; i < data_used.size() && used_entities_count < entities_count; ++i )
			{
				if( data_used[i] )
				{
					++used_entities_count;
					if( entities_entries[i].GetKey().Contains( reference_key ) )
					{
						assert( candidates_count < MAX_ENTITIES );
						const EntityID entityId = i;
						entities[candidates_count++] = Entity( entityId, ecs );
					}
				}
			}


			func( entities, candidates_count, ecs );
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
			ZeroMemory( data.data(), data.size() * sizeof( void* ) );
			data_sizes.resize( componentTypesCount );
			ZeroMemory( data_sizes.data(), data_sizes.size() * sizeof( size_t ) );
			data_counts.resize( componentTypesCount );
			ZeroMemory( data_counts.data(), data_counts.size() * sizeof( ComponentID ) );

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
			ZeroMemory( data.data(), data.size() * sizeof( void* ) );
			data_sizes.resize( componentTypesCount );
			ZeroMemory( data_sizes.data(), data_sizes.size() * sizeof( size_t ) );
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

			ZeroMemory( data[typeId], sizeof( C ) );

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
			return GetComponent<C>();
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
			return GetComponent<C>();
		}

		ComponentID GetComponentId() const
		{
			return m_componentId;
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

	class System
	{
	private:
		ArchetypeKey m_key;

		ComponentContainer* m_componentContainer;
		EntityContainer* m_entityContainer;

	public:
		void( *m_update ) ( Entity*, uint32_t, EntityComponentSystem* );

		System( ArchetypeKey key, void( *update ) ( Entity*, uint32_t, EntityComponentSystem* ) )
			: m_key( key ), m_update( update )
		{
		}

		template< typename ... ComponentTypes >
		static System Create( void( *func_update ) ( Entity*, uint32_t, EntityComponentSystem* ) )
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
			key.Contains( m_key );
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
		ComponentContainer componentContainer;
		EntityContainer entityContainer;

	public:
		template<class C, class ... Args>
		ComponentHandle<C> CreateComponent( Args ... componentArgs )
		{
			const ComponentID componentId = componentContainer.CreateComponent<C>( componentArgs ... );
			return ComponentHandle<C>( &componentContainer, componentId );
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
			return entityContainer.CreateEntity< ComponentTypes ... >( this, componentHandles.GetComponentId() ... );
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
		C* GetSingletonComponent() const
		{
			return singletonComponentContainer.GetComponent< C >();
		}

		template< class C >
		C* GetComponentForEntity( EntityID entityId ) const
		{
			const ComponentID componentId = entityContainer.GetComponentId< C >( entityId );
			return componentId != INVALID_COMPONENT_ID ? componentContainer.GetComponent< C >( componentId ) : nullptr;
		}

		void Destroy( Entity* entity )
		{
			if( entity->IsValid() )
			{
				const std::pair< ComponentID, ComponentTypeID >* componentIds;

				const size_t count = entityContainer.GetAllComponentIDsAndTypes( entity->GetId(), &componentIds );
				for( size_t i = 0; i < count; ++i )
				{
					componentContainer.DestroyComponent( componentIds[i].second, componentIds[i].first );
				}
				entityContainer.RemoveEntity( entity->GetId() );

				*entity = Entity();
			}
		}

		void RunSystem( const System& system )
		{
			entityContainer.ForEachEntityWithKey( system.GetKey(), this, system.m_update );
		}
	};
}