////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_storage_inline.h
//	Created 	: 15.06.2004
//  Modified 	: 15.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol path storage inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CPatrolPathStorage::CPatrolPathStorage()
{
}

IC const CPatrolPathStorage::PATROL_REGISTRY &CPatrolPathStorage::patrol_paths() const
{
	return (m_registry);
}

IC const CPatrolPath *CPatrolPathStorage::path(shared_str patrol_name, bool no_assert) const
{
	const_iterator it = patrol_paths().find(patrol_name);

	if (it != patrol_paths().end())		
		return (*it).second;
	
	Msg("! ERROR: There is no patrol path [%s]", *patrol_name);
	R_ASSERT(no_assert);
	return nullptr;	
}
