#include "RangedManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

RangedManager::RangedManager()
{
}

void RangedManager::executeMicro(const BWAPI::Unitset & targets)
{
	if (formSquad(targets, 32 * 8, 32 * 8, 90, 40)){
		formed = true;
	}
	else {
		formed = false;
	}
	
	assignTargetsOld(targets);
}
std::unordered_map<BWAPI::Unit, BWAPI::Unit> RangedManager::assignEnemy(const BWAPI::Unitset &meleeUnits, BWAPI::Unitset & meleeUnitTargets)
{
	std::unordered_map<BWAPI::Unit, BWAPI::Unit> attacker2target;
	std::vector<PairEdge> edges(meleeUnits.size()*meleeUnitTargets.size());
	int top = 0;
	for (auto &attacker : meleeUnits)
	{
		for (auto &target : meleeUnitTargets)
		{
			if (!target)
				continue;
			edges[top].attacker = attacker;
			edges[top].target = target;
			int groundWeaponRange = attacker->getType().groundWeapon().maxRange();
			edges[top++].distance = -getRealPriority(attacker, target);
		}
	}
	sort(edges.begin(), edges.end());
	sort(edges.begin(), edges.end(), [](PairEdge a, PairEdge b)
	{
		return a.target->getID() <b.target->getID();
	});
	PairEdge dummy;
	dummy.target = nullptr;
	edges.push_back(dummy);
	BWAPI::Unit p = nullptr;
	int sum = 0;
	for (auto idx = edges.begin(); idx!=edges.end(); idx++)
	{
		if (!idx->target)
			continue;
		double Health = (((double)idx->target->getHitPoints() + idx->target->getShields()));
		if (idx->target->getType().size() == BWAPI::UnitSizeTypes::Small)
			Health *= 2;
		if (idx->target->getType().size() == BWAPI::UnitSizeTypes::Medium)
			Health *= 1.5;
		if (p != idx->target)
		{
			sum = 0;
			p = idx->target;			
		}
		else
		{
			sum++;
			//assign at most 7 units attack
			//BWAPI::Broodwar <<( idx->target->getHitPoints()+idx->target->getShields()) / idx->attacker->getType().groundWeapon().damageAmount() << std::endl;
			if (sum>std::min(8, 1+(int)(Health / idx->attacker->getType().groundWeapon().damageAmount())))
			{
				idx->attacker = nullptr;
				if (meleeUnitTargets.find(idx->target) != meleeUnitTargets.end())
					meleeUnitTargets.erase(idx->target);
			}
		}
	}
	int t = 0;
	for (bool halt=false; halt == false; halt = true)
	{
		halt= true;
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
		for (auto kdx = maxRangeStart; kdx < maxRangeEnd; kdx++)
		{
			if (attacker2target.find(kdx->attacker) != attacker2target.end())
				continue;
			attacker2target[kdx->attacker] = kdx->target;
			halt = false;
		}
		t++;
	}
	return attacker2target;
}

bool RangedManager::rangedUnitShouldRetreat(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets)
{
	// terran don't regen so it doesn't make any sense to retreat
	if (meleeUnit->getType().getRace() == BWAPI::Races::Terran)
	{
		return false;
	}

	// we don't want to retreat the melee unit if its shields or hit points are above the threshold set in the config file
	// set those values to zero if you never want the unit to retreat from combat individually
	// if there is a ranged enemy unit within attack range of this melee unit then we shouldn't bother retreating since it could fire and kill it anyway
	if (meleeUnit->getGroundWeaponCooldown() != 0)
		return true;
	else
		return rand() % 100 < 60;
}

void RangedManager::assignTargetsOld(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & rangedUnits = getUnits();

	// figure out targets
	BWAPI::Unitset rangedUnitTargets;
	for (auto & target : targets)
	{
		// conditions for targeting
		if (!(target->getType() == BWAPI::UnitTypes::Zerg_Larva) &&
			!(target->getType() == BWAPI::UnitTypes::Zerg_Egg) &&
			!(target->getType() == BWAPI::UnitTypes::Buildings) &&
			(target->isTargetable()) &&
			target->isVisible() &&
			target->getType() != BWAPI::UnitTypes::Resource_Vespene_Geyser)
		{
			rangedUnitTargets.insert(target);
		}
	}
	auto attacker2target = assignEnemy(rangedUnits, rangedUnitTargets);
	BWAPI::Position shome = BWAPI::Position(BWTA::getStartLocation(BWAPI::Broodwar->self())->getPosition());

	BWAPI::Unitset _rangedUnits = rangedUnits;
	auto _rangedUnitTargets = rangedUnitTargets;
	int maxChoices = rangedUnits.size() / 3 + 1, i = 0;

	std::map<BWAPI::Unit, std::pair<BWAPI::Unit, BWAPI::Unit> > dodgeUnits;
	BWAPI::Unitset threatens;
	for (BWAPI::Unit threatenClosest = NULL, unitClosetEnemy = NULL;;)
	{
		if (i++ > maxChoices)
			break;
		double dist = BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge)?96:96;
		double enerange = BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge) ? 80 : 80;
		for (auto & rangedUnit : _rangedUnits)
		{
			for (auto target : _rangedUnitTargets)
			{
				if (target->getType().isWorker() || (target->getType().isBuilding()))
					continue;
				if (target->getType().groundWeapon().maxRange() <= enerange)
				{
					if (rangedUnit->getDistance(target) < dist)
					{
						threatenClosest = target;
						unitClosetEnemy = rangedUnit;
						dist = rangedUnit->getDistance(target);
					}
				}
			}
		}
		if (threatenClosest != NULL)
		{
			threatens.insert(threatenClosest);
			BWAPI::Broodwar->drawCircleMap(threatenClosest->getPosition(), 2, BWAPI::Colors::Red, true);
			auto vec1 = unitClosetEnemy->getPosition() - threatenClosest->getPosition();
			dodgeUnits[unitClosetEnemy] = std::make_pair(unitClosetEnemy, threatenClosest);
			auto __rangedUnits = rangedUnits;
			for (auto rangedUnit:__rangedUnits)
			{
				if (rangedUnit->getDistance(unitClosetEnemy) > unitClosetEnemy->getDistance(threatenClosest)+64)
					continue;
				auto vec2 = rangedUnit->getPosition() - threatenClosest->getPosition();
				int judge = (vec1.x*vec2.x + vec1.y*vec2.y)*(vec1.x*vec2.x + vec1.y*vec2.y);
				if (judge * 6 > (vec1.x*vec1.x + vec1.y*vec1.y)*(vec2.x*vec2.x + vec2.y*vec2.y) * 5)
				{
					dodgeUnits[rangedUnit] = std::make_pair(unitClosetEnemy, threatenClosest);
					_rangedUnits.erase(rangedUnit);
				}
			}
		}
		else
			break;
	}
	BWAPI::Unitset withoutBuildingsAndWorkers = rangedUnitTargets;
	auto _withoutBuildingsAndWorkers = withoutBuildingsAndWorkers;
	for (auto target : _withoutBuildingsAndWorkers)
	if (target->getType().isBuilding() || target->getType().isWorker())
		withoutBuildingsAndWorkers.erase(target);
	
	for (auto & rangedUnit : rangedUnits)
	{
		// train sub units such as scarabs or interceptors
		//trainSubUnits(rangedUnit);
		// if the order is to attack or defend
		int chokeZealot = 0;
		for (const auto & zealot : BWAPI::Broodwar->self()->getUnits())
		{
			// trivial case: unit which exists matches the type
			if (zealot->getType() != BWAPI::UnitTypes::Protoss_Zealot)
				continue;
			if (zealot->isCompleted() == false)
				continue;
			auto ckpt = BWTA::getNearestChokepoint(zealot->getPosition());
			if (zealot->getDistance(rangedUnit->getPosition()) < 300)
			{
				if (ckpt && ckpt->getWidth()<256 && ckpt->getCenter().getDistance(zealot->getPosition()) < 128)
					chokeZealot += 3;
				else
					chokeZealot += 1;
			}
			
		}

		bool danceMode;
		if (order.getType() == SquadOrderTypes::Defend)
			danceMode = chokeZealot < 3 && rangedUnits.size()<20;
		else
			danceMode = chokeZealot < 3 && rangedUnits.size()<20;

		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{

			// if there are targets
			if (!rangedUnitTargets.empty())
			{
				// find the best target for this zealot
				auto targetIdx = attacker2target.find(rangedUnit);
				BWAPI::Unit target = targetIdx == attacker2target.end() ? getTarget(rangedUnit, rangedUnitTargets) : targetIdx->second;
				// attack it				
				if (dodgeUnits.find(rangedUnit) == dodgeUnits.end())
				{
					Micro::SmartAttackUnit(rangedUnit, target);
				}
				else
				{
					if (danceMode)
						Micro::SmartKiteTarget(rangedUnit, target, dodgeUnits[rangedUnit].second, withoutBuildingsAndWorkers);
					else
						Micro::SmartAttackUnit(rangedUnit, target);
					//modified
					//BWAPI::Broodwar->sendText("%s", "old kite");
					//Micro::SmartKiteTarget(rangedUnit, dodgeUnits[rangedUnit].second);

				}
			}
			// if there are no targets
			else
			{
				Micro::SmartAttackMove(rangedUnit, order.getPosition());
			}
		}
	}
}

std::pair<BWAPI::Unit, BWAPI::Unit> RangedManager::findClosestUnitPair(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets)
{
	std::pair<BWAPI::Unit, BWAPI::Unit> closestPair(nullptr, nullptr);
	double closestDistance = std::numeric_limits<double>::max();

	for (auto & attacker : attackers)
	{
		BWAPI::Unit target = getTarget(attacker, targets);
		double dist = attacker->getDistance(attacker);

		if (!closestPair.first || (dist < closestDistance))
		{
			closestPair.first = attacker;
			closestPair.second = target;
			closestDistance = dist;
		}
	}

	return closestPair;
}

// get a target for the zealot to attack
BWAPI::Unit RangedManager::getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets)
{
	int bestPriorityDistance = 1000000;
	int bestPriority = 0;

	double bestLTD = 0;

	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::infinity();
	BWAPI::Unit closestTarget = nullptr;
	
	for (const auto & target : targets)
	{
		double distance = rangedUnit->getDistance(target);
		double LTD = UnitUtil::CalculateLTD(target, rangedUnit);
		int priority = getAttackPriority(rangedUnit, target);
		bool targetIsThreat = LTD > 0;

		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist))
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = target;
		}

	}

	return closestTarget;
}

// get the attack priority of a type in relation to a zergling


BWAPI::Unit RangedManager::closestrangedUnit(BWAPI::Unit target, std::set<BWAPI::Unit> & rangedUnitsToAssign)
{
	double minDistance = 0;
	BWAPI::Unit closest = nullptr;

	for (auto & rangedUnit : rangedUnitsToAssign)
	{
		double distance = rangedUnit->getDistance(target);
		if (!closest || distance < minDistance)
		{
			minDistance = distance;
			closest = rangedUnit;
		}
	}

	return closest;
}


// still has bug in it somewhere, use Old version
void RangedManager::assignTargetsNew(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & rangedUnits = getUnits();

	// figure out targets
	BWAPI::Unitset rangedUnitTargets;
	std::copy_if(targets.begin(), targets.end(), std::inserter(rangedUnitTargets, rangedUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });

	BWAPI::Unitset rangedUnitsToAssign(rangedUnits);
	std::map<BWAPI::Unit, int> attackersAssigned;

	for (auto & unit : rangedUnitTargets)
	{
		attackersAssigned[unit] = 0;
	}

	// keep assigning targets while we have attackers and targets remaining
	while (!rangedUnitsToAssign.empty() && !rangedUnitTargets.empty())
	{
		auto attackerAssignment = findClosestUnitPair(rangedUnitsToAssign, rangedUnitTargets);
		BWAPI::Unit & attacker = attackerAssignment.first;
		BWAPI::Unit & target = attackerAssignment.second;

		UAB_ASSERT_WARNING(attacker, "We should have chosen an attacker!");

		if (!attacker)
		{
			break;
		}

		if (!target)
		{
			Micro::SmartAttackMove(attacker, order.getPosition());
			continue;
		}

		if (Config::Micro::KiteWithRangedUnits)
		{
			if (attacker->getType() == BWAPI::UnitTypes::Zerg_Mutalisk || attacker->getType() == BWAPI::UnitTypes::Terran_Vulture)
			{
				Micro::SmartAttackUnit(attacker, target);
			}
			else
			{
				//Micro::MutaDanceTarget(attacker, target);
			}
		}
		else
		{
			Micro::SmartAttackUnit(attacker, target);
		}

		// update the number of units assigned to attack the target we found
		int & assigned = attackersAssigned[attackerAssignment.second];
		assigned++;

		// if it's a small / fast unit and there's more than 2 things attacking it already, don't assign more
		if ((target->getType().isWorker() || target->getType() == BWAPI::UnitTypes::Zerg_Zergling) && (assigned > 2))
		{
			rangedUnitTargets.erase(target);
		}
		// if it's a building and there's more than 10 things assigned to it already, don't assign more
		else if (target->getType().isBuilding() && (assigned > 10))
		{
			rangedUnitTargets.erase(target);
		}

		rangedUnitsToAssign.erase(attacker);
	}

	// if there's no targets left, attack move to the order destination
	if (rangedUnitTargets.empty())
	{
		for (auto & unit : rangedUnitsToAssign)
		{
			if (unit->getDistance(order.getPosition()) > 100)
			{
				// move to it
				Micro::SmartAttackMove(unit, order.getPosition());
			}
		}
	}
}

double RangedManager::getRealPriority(BWAPI::Unit attacker, BWAPI::Unit target)
{
	if (!attacker||!target)
		return 0;
	int groundWeaponRange = attacker->getType().groundWeapon().maxRange();
	double distA2T = std::max(0, attacker->getDistance(target) - groundWeaponRange);
	//Protoss_Interceptor ignore distance
	if (attacker->getType() == BWAPI::UnitTypes::Protoss_Interceptor)
		distA2T = 0;
	double Health = (((double)target->getHitPoints() + target->getShields()));
	if (target->getType().size() == BWAPI::UnitSizeTypes::Small)
		Health *= 2;
	if (target->getType().size() == BWAPI::UnitSizeTypes::Medium)
		Health *= 1.5;
	return getAttackPriority(attacker, target)*exp(-distA2T / 5) / (Health + 160);
}

int RangedManager::getAttackPriority(BWAPI::Unit attacker, BWAPI::Unit unit)
{
	auto closestCarrier = UnitUtil::GetClosestOurUnitTypeToTarget(BWAPI::UnitTypes::Protoss_Carrier, attacker->getPosition());
	if (closestCarrier&&closestCarrier->isCompleted()&&closestCarrier->getDistance(unit)<unit->getType().airWeapon().maxRange())
		return getPrioritySaveCarrier(attacker, unit);
	else
		return getPriorityDefault(attacker, unit);
}

int RangedManager::getPrioritySaveCarrier(BWAPI::Unit attacker, BWAPI::Unit unit)
{
	BWAPI::UnitType type = unit->getType();
	int tmpPriority = getPriorityDefault(attacker, unit);
	if (unit->getType().airWeapon().getID() != BWAPI::WeaponTypes::Enum::None || unit->getType().isFlyer())
	{
		tmpPriority = static_cast<int>(getPriorityDefault(attacker, unit)*2);
	}
	if (unit->getType() == BWAPI::UnitTypes::Protoss_Photon_Cannon
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Spore_Colony
		|| unit->getType() == BWAPI::UnitTypes::Terran_Missile_Turret)
	{
		tmpPriority = std::max(tmpPriority, 10);
	}
	return tmpPriority;
}

int RangedManager::getPriorityDefault(BWAPI::Unit rangedUnit, BWAPI::Unit target)
{
	BWAPI::UnitType rangedType = rangedUnit->getType();
	BWAPI::UnitType targetType = target->getType();

	if (target->getType() == BWAPI::UnitTypes::Zerg_Overlord)
		return 1;

	if (target->getType() == BWAPI::UnitTypes::Protoss_Photon_Cannon 
	 || target->getType() == BWAPI::UnitTypes::Zerg_Sunken_Colony)
	{
		return 8;
	}
	if (rangedUnit->getType() == BWAPI::UnitTypes::Zerg_Scourge)
	{
		if (target->getType() == BWAPI::UnitTypes::Protoss_Carrier)
		{

			return 100;
		}

		if (target->getType() == BWAPI::UnitTypes::Protoss_Corsair)
		{
			return 90;
		}
	}

	bool isThreat = rangedType.isFlyer() ? targetType.airWeapon() != BWAPI::WeaponTypes::None : targetType.groundWeapon() != BWAPI::WeaponTypes::None;

	if (target->getType().isWorker())
	{
		isThreat = false;
	}

	if (target->getType() == BWAPI::UnitTypes::Zerg_Larva || target->getType() == BWAPI::UnitTypes::Zerg_Egg)
	{
		return 0;
	}

	if (rangedUnit->isFlying() && target->getType() == BWAPI::UnitTypes::Protoss_Carrier)
	{
		return 101;
	}	// if the target is building something near our base something is fishy


	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Bunker || isThreat)
	{
		return 11;
	}
	// next priority is worker
	else if (targetType.isWorker())
	{
		if (order.getType() != SquadOrderTypes::Defend&&BWTA::getRegion(rangedUnit->getPosition()) != BWTA::getRegion(BWAPI::Broodwar->self()->getStartLocation()))
		{
			return 4;
		}
		else
			return 1;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Zerg_Spawning_Pool)
	{
		return 5;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Protoss_Pylon)
	{
		return 5;
	}
	// next is buildings that cost gas
	else if (targetType.gasPrice() > 0)
	{
		return 4;
	}
	else if (targetType.mineralPrice() > 0)
	{
		return 3;
	}
	// then everything else
	else
	{
		return 1;
	}
}
/*
void RangedManager::regroup(const BWAPI::Position & regroupPosition) const
{
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	int regroupDistanceFromBase = MapTools::Instance().getGroundDistance(regroupPosition, ourBasePosition);
	auto units = getUnits();

	// for each of the units we have
	for (auto & unit : units)
	{
		int unitDistanceFromBase = MapTools::Instance().getGroundDistance(unit->getPosition(), ourBasePosition);

		// if the unit is outside the regroup area
		if (unitDistanceFromBase > regroupDistanceFromBase)
		{
			Micro::SmartMove(unit, ourBasePosition);
		}
		else if ((unit->getDistance(regroupPosition) > Config::Micro::CombatRegroupRadius&&unit->getAirWeaponCooldown()) && (unit->getGroundWeaponCooldown()>0))
		{
			// regroup it
			Micro::SmartMove(unit, regroupPosition);
		}
		else
		{
			Micro::SmartAttackMove(unit, regroupPosition);
		}
	}
}
*/