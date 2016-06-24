#pragma once;

#include <Common.h>
#include "MicroManager.h"

namespace UAlbertaBot
{
class MicroManager;
class MeleeManager : public MicroManager
{
private:
	int combatNum;
public:
	BWAPI::Position pullPosition;
	MeleeManager();
	~MeleeManager() {}
	void executeMicro(const BWAPI::Unitset & targets);

	BWAPI::Unit chooseTarget(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets, std::map<BWAPI::Unit, int> & numTargeting);
	BWAPI::Unit closestMeleeUnit(BWAPI::Unit target, const BWAPI::Unitset & meleeUnitToAssign);
	int getAttackPriority(BWAPI::Unit attacker, BWAPI::Unit unit);
	BWAPI::Unit getTarget(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets);
    bool meleeUnitShouldRetreat(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets);
	std::unordered_map<BWAPI::Unit, BWAPI::Unit> findClosestUnitPairs(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets);
	void assignTargetsNew(const BWAPI::Unitset & targets);
    void assignTargetsOld(const BWAPI::Unitset & targets);
	double getRealPriority(BWAPI::Unit attacker, BWAPI::Unit target);
	int getPriorityDefault(BWAPI::Unit attacker, BWAPI::Unit target);
	int getPriorityCounterFlyer(BWAPI::Unit attacker, BWAPI::Unit target);
	int getPrioritySaveCarrier(BWAPI::Unit attacker, BWAPI::Unit target);
	int compareTwoTargets(BWAPI::Unit attacker, BWAPI::Unit target1, BWAPI::Unit target2);
	int myMicroConstruct(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets);
	std::unordered_map<BWAPI::Unit, BWAPI::Unit> assignEnemy(const BWAPI::Unitset &,const BWAPI::Unitset & );
};
}