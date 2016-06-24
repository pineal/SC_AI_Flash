#include "CarrierManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

CarrierManager::CarrierManager()
{
}

void CarrierManager::executeMicro(const BWAPI::Unitset & targets)
{
	assignTargetsOld(targets);
}


bool CarrierManager::carrierShouldRetreat(BWAPI::Unit rangedUnit, BWAPI::Unitset rangedUnitTargets)
{
	if (rangedUnit->getShields() > 2)
	{
		return false;
	}
	// if there is a ranged enemy unit within attack range of this melee unit then we shouldn't bother retreating since it could fire and kill it anyway
	else
		return true;
	

}


void CarrierManager::assignTargetsOld(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & rangedUnits = getUnits();

	// figure out targets
	BWAPI::Unitset rangedUnitTargets;
	std::copy_if(targets.begin(), targets.end(), std::inserter(rangedUnitTargets, rangedUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });
	for (auto & rangedUnit : rangedUnits)
	{
		// train sub units such as scarabs or interceptors
		//trainSubUnits(rangedUnit);

		// if the order is to attack or defend
		BWAPI::Position shome = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());

		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
		{
			// if there are targets
			if (!rangedUnitTargets.empty())
			{
				// find the best target for this zealot
				BWAPI::Unit target = getTarget(rangedUnit, rangedUnitTargets);

				if (target && Config::Debug::DrawUnitTargetInfo)
				{
					BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), rangedUnit->getTargetPosition(), BWAPI::Colors::Purple);
				}

				if ((carrierShouldRetreat(rangedUnit, rangedUnitTargets) && (rangedUnit->isUnderAttack())))
				{
					BWAPI::Position fleeTo(rangedUnit->getPosition());
					BWAPI::Unitset EnmiesRemovedWorker;
					for (auto target : rangedUnitTargets)
					if (UnitUtil::CanAttackAir(target))
						EnmiesRemovedWorker.insert(target);
					BWAPI::Position dodge(positionShift(EnmiesRemovedWorker.getPosition(), rangedUnit->getPosition(), 100));
					BWAPI::Position home(positionShift(rangedUnit->getPosition(), shome, 100));
					fleeTo.x += dodge.x;
					fleeTo.y += dodge.y;
					if (order.getType() == SquadOrderTypes::Attack)
					{
						fleeTo.x += home.x;
						fleeTo.y += home.y;
					}
					auto tfleeTo = positionShift(rangedUnit->getPosition(), fleeTo, 96);
					fleeTo = rangedUnit->getPosition() + tfleeTo;
					Micro::SmartMove(rangedUnit, fleeTo);
				}
				else if (rangedUnit->getInterceptors().getPosition().getDistance(rangedUnit->getPosition()) < 20.0)
					// attack it
				{
					Micro::SmartAttackUnit(rangedUnit, target);
				}
				else
				{
					auto nearestUnwalkable = BWTA::getNearestUnwalkablePosition(rangedUnit->getPosition());
					if (nearestUnwalkable.getDistance(rangedUnit->getPosition())<200)
						Micro::SmartMove(rangedUnit, nearestUnwalkable);
					else
						Micro::SmartMove(rangedUnit, rangedUnit->getPosition());

				}
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (unitClosestToEnemy != NULL)
				{
					if (rangedUnit->getDistance(unitClosestToEnemy->getPosition()) > 200)
					{
						// move to it
						Micro::SmartAttackMove(rangedUnit, unitClosestToEnemy->getPosition());
					}
				}
				else
				{
					if (rangedUnit->getDistance(order.getPosition()) > 200)
					{
						// move to it
						Micro::SmartAttackMove(rangedUnit, order.getPosition());
					}
				}
					
			}
		}
	}
}

// get the attack priority of a type in relation to a zergling
int CarrierManager::getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target)
{
	BWAPI::UnitType rangedType = rangedUnit->getType();
	BWAPI::UnitType targetType = target->getType();
	//attack long range Unit
	if (target->getType().groundWeapon().maxRange() > 200)
	{
		return 101;
	}
	if (target->getType() == BWAPI::UnitTypes::Protoss_Photon_Cannon || target->getType() == BWAPI::UnitTypes::Zerg_Sunken_Colony)
	{
		return 102;
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
	}

	// if the target is building something near our base something is fishy
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	if (target->getType().isWorker() && (target->isConstructing() || target->isRepairing()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 100;
	}

	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Bunker || isThreat)
	{
		return 11;
	}
	// next priority is worker
	else if (targetType.isWorker())
	{
		if (rangedUnit->getType() == BWAPI::UnitTypes::Terran_Vulture)
		{
			return 11;
		}

		return 11;
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

BWAPI::Unit CarrierManager::getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets)
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
