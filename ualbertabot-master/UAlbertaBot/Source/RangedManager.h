#pragma once;

#include <Common.h>
#include <set>
#include <map>
#include "MicroManager.h"

namespace UAlbertaBot
{
class RangedManager : public MicroManager
{
public:
	BWAPI::Position centerOfAttackers;
	RangedManager();
	void executeMicro(const BWAPI::Unitset & targets);

	BWAPI::Unit chooseTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets, std::map<BWAPI::Unit, int> & numTargeting);
	BWAPI::Unit closestrangedUnit(BWAPI::Unit target, std::set<BWAPI::Unit> & rangedUnitsToAssign);
    std::pair<BWAPI::Unit, BWAPI::Unit> findClosestUnitPair(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets);

	int getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target);
	double getRealPriority(BWAPI::Unit attacker, BWAPI::Unit target);
	std::unordered_map<BWAPI::Unit, BWAPI::Unit> assignEnemy(const BWAPI::Unitset &meleeUnits, BWAPI::Unitset & meleeUnitTargets);
	BWAPI::Unit getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets);
//	void			    virtual regroup(const BWAPI::Position & regroupPosition) const;

    void assignTargetsNew(const BWAPI::Unitset & targets);
    void assignTargetsOld(const BWAPI::Unitset & targets);

	int getPriorityDefault(BWAPI::Unit attacker, BWAPI::Unit target);
	int getPriorityCounterFlyer(BWAPI::Unit attacker, BWAPI::Unit target);
	int getPrioritySaveCarrier(BWAPI::Unit attacker, BWAPI::Unit target);


	bool rangedUnitShouldRetreat(BWAPI::Unit rangeUnit, const BWAPI::Unitset & targets);
};
}