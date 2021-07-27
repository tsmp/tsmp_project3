#pragma once

#define TEMPLATE_SPECIALIZATION template < \
	typename _Object>

#define CStateMonsterMoveAroundPointAbstract CStateMonsterMoveAroundPoint<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveAroundPointAbstract::initialize()
{
	inherited::initialize();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveAroundPointAbstract::execute()
{
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveAroundPointAbstract::check_completion()
{
	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterMoveAroundPointAbstract
