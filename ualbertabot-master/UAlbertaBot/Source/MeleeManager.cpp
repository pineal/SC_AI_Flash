#include "MeleeManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

bool PairEdge::operator <(const PairEdge& that) const
{
	return this->distance<that.distance;
}

MeleeManager::MeleeManager()
{

}

void MeleeManager::executeMicro(const BWAPI::Unitset & targets)
{
	if (formSquad(targets, 32 * 5, 32 * 8, 90, 35)){
		formed = true;
	}
	else {
		formed = false;
	}
	assignTargetsOld(targets);
}


//assign the right enemy
std::unordered_map<BWAPI::Unit, BWAPI::Unit> MeleeManager::assignEnemy(const BWAPI::Unitset &meleeUnits, const BWAPI::Unitset & meleeUnitTargets)
{
	std::unordered_map<BWAPI::Unit, BWAPI::Unit> attacker2target;
	std::vector<PairEdge> edges(meleeUnits.size()*meleeUnitTargets.size());
	int top = 0;
	for (auto &attacker : meleeUnits)
	{
		for (auto &target : meleeUnitTargets)
		{
			edges[top].attacker = attacker;
			edges[top].target = target;
			int groundWeaponRange = attacker->getType().groundWeapon().maxRange();
			edges[top++].distance = -getRealPriority(attacker, target);
		}
	}
	sort(edges.begin(), edges.end());
	edges.resize(edges.size() / (meleeUnits.size() + 1));
	sort(edges.begin(), edges.end(), [](PairEdge a, PairEdge b)
	{
		return (int)a.target < (int)b.target;
	});
	PairEdge dummy;
	dummy.target = nullptr;
	edges.push_back(dummy);
	BWAPI::Unit p = nullptr;
	int sum = 0;
	for (auto idx = edges.begin(); idx->target != nullptr; idx++)
	{
		if (p != idx->target)
		{
			sum = 0;
			p = idx->target;
		}
		else
		{
			sum++;
			if (sum >= 6)
			{
				idx->attacker = nullptr;
			}
		}
	}
	for (bool halt = true; halt == false; halt = true)
	{
		auto tmpRangeStart = edges.begin();
		auto maxRangeStart = tmpRangeStart, maxRangeEnd = tmpRangeStart;
		double tmpsum = 0, tmpres = INT_MIN;
		for (auto idx = edges.begin(); idx->target != nullptr; idx++)
		{
			if (attacker2target.find(idx->attacker) != attacker2target.end())
				continue;
			if (idx->target != (idx + 1)->target)
			{
				if (tmpsum > tmpres)
				{
					tmpres = tmpsum;
					maxRangeStart = tmpRangeStart;
					maxRangeEnd = idx + 1;
				}
				tmpsum = 0;
				tmpRangeStart = idx + 1;
			}
			else
				tmpsum += getRealPriority(idx->attacker, idx->target);
		}
		for (auto kdx = maxRangeStart; kdx != maxRangeEnd; kdx++)
		{
			if (attacker2target.find(kdx->attacker) != attacker2target.end())
				continue;
			attacker2target[kdx->attacker] = kdx->target;
			halt = false;
		}
	}
	return attacker2target;
}

void MeleeManager::assignTargetsOld(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & meleeUnits = getUnits();
	pullPosition = calcCenter();
	// figure out targets
	BWAPI::Unitset meleeUnitTargets, localEnemyTargets;
	for (auto & target : targets)
	{
		// conditions for targeting
		if (!(target->getType().isFlyer()) &&
			!(target->isLifted()) &&
			!(target->getType() == BWAPI::UnitTypes::Zerg_Larva) &&
			!(target->getType() == BWAPI::UnitTypes::Zerg_Egg) &&
			!(target->getType() == BWAPI::UnitTypes::Buildings) &&
			(target->isTargetable()) &&
			target->isVisible() &&
			target->getType() != BWAPI::UnitTypes::Resource_Vespene_Geyser)
		{
			meleeUnitTargets.insert(target);
		}
	}
	std::unordered_map<BWAPI::Unit, BWAPI::Unit> attacker2target = assignEnemy(meleeUnits, meleeUnitTargets);
	// for each meleeUnit
	//special case,less than two combats, save combats;
	combatNum = meleeUnits.size();
	for (auto & meleeUnit : meleeUnits)
	{
		if (meleeUnit->getType().isWorker())
			combatNum--;
	}
	for (auto & meleeUnit : meleeUnits)
	{
		BWAPI::Position shome = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			if ((meleeUnitShouldRetreat(meleeUnit, meleeUnitTargets) && (meleeUnit->isUnderAttack()) && !meleeUnit->isStuck()))
			{
				BWAPI::Position fleeTo(meleeUnit->getPosition());
				BWAPI::Unitset EnmiesRemovedWorker;
				for (auto target : meleeUnitTargets)
				if (!target->getType().isWorker() && !target->getType().isBuilding())
					EnmiesRemovedWorker.insert(target);
				BWAPI::Position dodge(positionShift(EnmiesRemovedWorker.getPosition(), meleeUnit->getPosition(), 50));
				BWAPI::Position home(positionShift(meleeUnit->getPosition(), shome, 50));
				fleeTo.x += dodge.x;
				fleeTo.y += dodge.y;
				if (order.getType() == SquadOrderTypes::Attack)
				{
					fleeTo.x += home.x;
					fleeTo.y += home.y;
				}
				auto tfleeTo = positionShift(meleeUnit->getPosition(), fleeTo, 96);
				fleeTo = meleeUnit->getPosition() + tfleeTo;
				Micro::SmartMove(meleeUnit, fleeTo);
			}
			// if there are targets
			else if (!meleeUnitTargets.empty())
			{
				BWAPI::Unit target;
				if (attacker2target.find(meleeUnit) != attacker2target.end())
				{
					target = attacker2target[meleeUnit];
				}
				else  target = getTarget(meleeUnit, meleeUnitTargets);
				Micro::SmartAttackUnit(meleeUnit, target);

			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (meleeUnit->getDistance(order.getPosition()) > 100)
				{
					// move to it
					Micro::SmartMove(meleeUnit, order.getPosition());
				}
			}
		}
		if (Config::Debug::DrawUnitTargetInfo)
		{
			BWAPI::Broodwar->drawLineMap(meleeUnit->getPosition().x, meleeUnit->getPosition().y,
				meleeUnit->getTargetPosition().x, meleeUnit->getTargetPosition().y, Config::Debug::ColorLineTarget);
			//
		}
	}
}

std::unordered_map<BWAPI::Unit, BWAPI::Unit> MeleeManager::findClosestUnitPairs(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets)
{
	std::unordered_map<BWAPI::Unit, BWAPI::Unit> mapping;
	for (auto & attacker : attackers)
	{
		double closestDistance = std::numeric_limits<double>::max();
		for (auto &target : targets)
		{
			if (attacker->getDistance(target) < closestDistance)
			{
				closestDistance = attacker->getDistance(target);
				mapping[attacker] = target;
			}
		}
	}
	return mapping;
}

// get a target for the meleeUnit to attack
BWAPI::Unit MeleeManager::getTarget(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets)
{
	BWAPI::Unit bestTarget = nullptr;

	// for each target possiblity
	for (auto & unit : targets)
	{
		if (!bestTarget || getRealPriority(meleeUnit, unit) > getRealPriority(meleeUnit, bestTarget))
		{
			bestTarget = unit;
		}
	}

	return bestTarget;
}

//get real priority
double MeleeManager::getRealPriority(BWAPI::Unit attacker, BWAPI::Unit target)
{
	if (!(target->getType() == BWAPI::UnitTypes::Protoss_Photon_Cannon || target->getType() == BWAPI::UnitTypes::Zerg_Sunken_Colony) && target->getType().isBuilding())
		return -attacker->getDistance(target);
	int groundWeaponRange = attacker->getType().groundWeapon().maxRange();
	double distA2T = std::max(0, attacker->getDistance(target) - groundWeaponRange);
	double Health = (((double)target->getHitPoints() + target->getShields()));
	if (target->getType().isWorker() && order.getType() == SquadOrderTypes::Defend)
		return getAttackPriority(attacker, target)*exp(-(distA2T + 4000) / 5) / (Health*Health + 160);
	return getAttackPriority(attacker, target)*exp(-distA2T / 5) / (Health*Health + 160);
}

// get the attack priority of a type in relation to a zergling
int MeleeManager::getAttackPriority(BWAPI::Unit attacker, BWAPI::Unit unit)
{
	if (_microDecision->getFlightOrientatedFactor() == 1.0)
		return getPriorityDefault(attacker, unit);
	else
		return getPrioritySaveCarrier(attacker, unit);
}

int MeleeManager::getPrioritySaveCarrier(BWAPI::Unit attacker, BWAPI::Unit unit)
{
	BWAPI::UnitType type = unit->getType();
	int tmpPriority = getPriorityDefault(attacker, unit);
	if (unit->getType().airWeapon().getID() != BWAPI::WeaponTypes::Enum::None)
	{
		tmpPriority = static_cast<int>(getPriorityDefault(attacker, unit)*_microDecision->getFlightOrientatedFactor());
	}
	if (unit->getType() == BWAPI::UnitTypes::Protoss_Photon_Cannon
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Spore_Colony
		|| unit->getType() == BWAPI::UnitTypes::Terran_Missile_Turret)
	{
		tmpPriority = std::max(tmpPriority, static_cast<int>(6 * _microDecision->getFlightOrientatedFactor()));
	};
	return tmpPriority;
}

int MeleeManager::getPriorityDefault(BWAPI::Unit attacker, BWAPI::Unit unit)
{
	BWAPI::UnitType type = unit->getType();
	if (unit->getType() == BWAPI::UnitTypes::Protoss_Photon_Cannon
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Sunken_Colony)
	{
		return 8;
	}
	if (attacker->getType() == BWAPI::UnitTypes::Protoss_Interceptor || unit->getType() == BWAPI::UnitTypes::Terran_Bunker)
		return 8;
	if (attacker->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar
		&& unit->getType() == BWAPI::UnitTypes::Terran_Missile_Turret
		&& (BWAPI::Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar) == 0))
	{
		return 130;
	}

	if (attacker->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar && unit->getType().isWorker())
	{
		return 120;
	}

	// highest priority is something that can attack us or aid in combat
	if (type.isCloakable() || type.hasPermanentCloak())
	{
		return 11;
	}
	else if (type == BWAPI::UnitTypes::Terran_Medic ||
		(type.groundWeapon() != BWAPI::WeaponTypes::None && !type.isWorker()) ||
		type == BWAPI::UnitTypes::Terran_Bunker ||
		type == BWAPI::UnitTypes::Protoss_High_Templar ||
		type == BWAPI::UnitTypes::Protoss_Reaver ||
		(type.isWorker() && unitNearChokepoint(unit)))
	{
		return 100;
	}
	// next priority is worker
	else if (type.isWorker())
	{
		if (unit->isRepairing())
			return 9;
		else
			return 5;
	}
	else if (type == BWAPI::UnitTypes::Buildings)
	{
		return type.canProduce() ? 3 : 1;
	}
	else if (type.gasPrice() > 0)
	{
		return 4;
	}
	else if (type.mineralPrice() > 0)
	{
		return 3;
	}
	// then everything else
	else
	{
		return 1;
	}
}
BWAPI::Unit MeleeManager::closestMeleeUnit(BWAPI::Unit target, const BWAPI::Unitset & meleeUnitsToAssign)
{
	double minDistance = 0;
	BWAPI::Unit closest = nullptr;

	for (auto & meleeUnit : meleeUnitsToAssign)
	{
		double distance = meleeUnit->getDistance(target);
		if (!closest || distance < minDistance)
		{
			minDistance = distance;
			closest = meleeUnit;
		}
	}

	return closest;
}

bool MeleeManager::meleeUnitShouldRetreat(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets)
{
	auto ckpt = BWTA::getNearestChokepoint(meleeUnit->getPosition());
	if (ckpt&&ckpt->getWidth() < 196 && ckpt->getCenter().getDistance(meleeUnit->getPosition()) < 200 && 0<UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Dragoon))
		return false;

	if (!meleeUnit->getType().isWorker() && BWTA::getRegion(meleeUnit->getPosition()) == BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion())
		return false;
	// terran don't regen so it doesn't make any sense to retreat
	if (meleeUnit->getType().getRace() == BWAPI::Races::Terran)
	{
		return false;
	}

	// we don't want to retreat the melee unit if its shields or hit points are above the threshold set in the config file
	// set those values to zero if you never want the unit to retreat from combat individually
	if (meleeUnit->getShields() > Config::Micro::RetreatMeleeUnitShields || meleeUnit->getHitPoints() > Config::Micro::RetreatMeleeUnitHP)
	{
		return false;
	}

	// if there is a ranged enemy unit within attack range of this melee unit then we shouldn't bother retreating since it could fire and kill it anyway
	for (auto & unit : targets)
	{
		int groundWeaponRange = unit->getType().groundWeapon().maxRange();
		if (groundWeaponRange >= 64 && this->getUnits().size()>4)
		{
			//the possibility of retreat from rangeunits is determined by their distance and weaponrange
			if (rand() % 100 < std::max(0, 60 - ((int)unit->getDistance(meleeUnit) - groundWeaponRange)))
				return false;
		}
	}
	// if there is a ranged enemy unit within attack range of this melee unit then we shouldn't bother retreating since it could fire and kill it 
	if (meleeUnit->getGroundWeaponCooldown() != 0)
		return true;
	else
		return rand() % 100 < 60;
}


// still has bug in it somewhere, use Old version
/*
void MeleeManager::assignTargetsNew(const BWAPI::Unitset & targets)
{
const BWAPI::Unitset & meleeUnits = getUnits();
// figure out targets
BWAPI::Unitset meleeUnitTargets;
for (auto & target : targets)
{
// conditions for targeting
if (!(target->getType().isFlyer()) &&
!(target->isLifted()) &&
!(target->getType() == BWAPI::UnitTypes::Zerg_Larva) &&
!(target->getType() == BWAPI::UnitTypes::Zerg_Egg) &&
target->isVisible())
{
meleeUnitTargets.insert(target);
}
}
BWAPI::Unitset meleeUnitsToAssign(meleeUnits);
std::map<BWAPI::Unit, int> attackersAssigned;
for (auto & unit : meleeUnitTargets)
{
attackersAssigned[unit] = 0;
}
int smallThreshold = BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg ? 3 : 1;
int bigThreshold = BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg ? 12 : 3;
// keep assigning targets while we have attackers and targets remaining
while (!meleeUnitsToAssign.empty() && !meleeUnitTargets.empty())
{
auto attackerAssignment = findClosestUnitPair(meleeUnitsToAssign, meleeUnitTargets);
BWAPI::Unit & attacker = attackerAssignment.first;
BWAPI::Unit & target = attackerAssignment.second;
UAB_ASSERT_WARNING(attacker, "We should have chosen an attacker!");
if (!attacker)
{
break;
}
if (!target)
{
Micro::SmartMove(attacker, order.getPosition());
continue;
}
Micro::SmartAttackUnit(attacker, target);
// update the number of units assigned to attack the target we found
int & assigned = attackersAssigned[attackerAssignment.second];
assigned++;
// if it's a small / fast unit and there's more than 2 things attacking it already, don't assign more
if ((target->getType().isWorker() || target->getType() == BWAPI::UnitTypes::Zerg_Zergling) && (assigned >= smallThreshold))
{
meleeUnitTargets.erase(target);
}
// if it's a building and there's more than 10 things assigned to it already, don't assign more
else if (assigned > bigThreshold)
{
meleeUnitTargets.erase(target);
}
meleeUnitsToAssign.erase(attacker);
}
// if there's no targets left, attack move to the order destination
if (meleeUnitTargets.empty())
{
for (auto & unit : meleeUnitsToAssign)
{
if (unit->getDistance(order.getPosition()) > 100)
{
// move to it
Micro::SmartMove(unit, order.getPosition());
BWAPI::Broodwar->drawLineMap(unit->getPosition(), order.getPosition(), BWAPI::Colors::Yellow);
}
}
}
}
*/