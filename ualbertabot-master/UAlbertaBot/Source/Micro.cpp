#include "Micro.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

size_t TotalCommands = 0;

const int dotRadius = 2;

void Micro::drawAPM(int x, int y)
{
	int bwapiAPM = BWAPI::Broodwar->getAPM();
	int myAPM = (int)(TotalCommands / ((double)BWAPI::Broodwar->getFrameCount() / (24 * 60)));
	BWAPI::Broodwar->drawTextScreen(x, y, "%d %d", bwapiAPM, myAPM);
}



void Micro::SmartAttackUnit(BWAPI::Unit attacker, BWAPI::Unit target)
{
	UAB_ASSERT(attacker, "SmartAttackUnit: Attacker not valid");
	UAB_ASSERT(target, "SmartAttackUnit: Target not valid");

	if (!attacker || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	// if we've already told this unit to attack this target, ignore this command
	if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&	currentCommand.getTarget() == target)
	{
		return;
	}
	if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&	currentCommand.getTarget() == target&&!attacker->getType().isWorker() && attacker->getType()!=BWAPI::UnitTypes::Protoss_Carrier)
	{
		if (attacker->getDistance(target)>250)
		{
			attacker->stop();
			attacker->attack(target);
		}
		return;
	}
	// if nothing prevents it, attack the target
	attacker->attack(target);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::Red, true);
		BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Red, true);
		BWAPI::Broodwar->drawLineMap(attacker->getPosition(), target->getPosition(), BWAPI::Colors::Red);
	}
}

void Micro::SmartAttackMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition)
{
	//UAB_ASSERT(attacker, "SmartAttackMove: Attacker not valid");
	//UAB_ASSERT(targetPosition.isValid(), "SmartAttackMove: targetPosition not valid");

	if (!attacker || !targetPosition.isValid())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	// if we've already told this unit to attack this target, ignore this command
	if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Move &&	currentCommand.getTargetPosition() == targetPosition)
	{
		return;
	}

	// if nothing prevents it, attack the target
	attacker->attack(targetPosition);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::Orange, true);
		BWAPI::Broodwar->drawCircleMap(targetPosition, dotRadius, BWAPI::Colors::Orange, true);
		BWAPI::Broodwar->drawLineMap(attacker->getPosition(), targetPosition, BWAPI::Colors::Orange);
	}
}

void Micro::SmartMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition,bool step)
{
	//UAB_ASSERT(attacker, "SmartAttackMove: Attacker not valid");
	//UAB_ASSERT(targetPosition.isValid(), "SmartAttackMove: targetPosition not valid");

	if (!attacker || !targetPosition.isValid())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	int dragoonfix = attacker->getType() == BWAPI::UnitTypes::Protoss_Dragoon?8:0;
	if (attacker->getLastCommandFrame()+dragoonfix >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Move) && (currentCommand.getTargetPosition() == targetPosition) && attacker->isMoving())
	{
		return;
	}

	// if nothing prevents it, attack the target
	attacker->move(targetPosition);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::White, true);
		BWAPI::Broodwar->drawCircleMap(targetPosition, dotRadius, BWAPI::Colors::White, true);
		BWAPI::Broodwar->drawLineMap(attacker->getPosition(), targetPosition, BWAPI::Colors::White);
	}
}


void Micro::SmartRightClick(BWAPI::Unit unit, BWAPI::Unit target)
{
	UAB_ASSERT(unit, "SmartRightClick: Unit not valid");
	UAB_ASSERT(target, "SmartRightClick: Target not valid");

	if (!unit || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (unit->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || unit->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(unit->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Right_Click_Unit) && (currentCommand.getTargetPosition() == target->getPosition()))
	{
		return;
	}

	// if nothing prevents it, attack the target
	unit->rightClick(target);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		BWAPI::Broodwar->drawCircleMap(unit->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		BWAPI::Broodwar->drawLineMap(unit->getPosition(), target->getPosition(), BWAPI::Colors::Cyan);
	}
}

void Micro::SmartLaySpiderMine(BWAPI::Unit unit, BWAPI::Position pos)
{
	if (!unit)
	{
		return;
	}

	if (!unit->canUseTech(BWAPI::TechTypes::Spider_Mines, pos))
	{
		return;
	}

	BWAPI::UnitCommand currentCommand(unit->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Use_Tech_Position) && (currentCommand.getTargetPosition() == pos))
	{
		return;
	}

	unit->canUseTechPosition(BWAPI::TechTypes::Spider_Mines, pos);
}

void Micro::SmartRepair(BWAPI::Unit unit, BWAPI::Unit target)
{
	UAB_ASSERT(unit, "SmartRightClick: Unit not valid");
	UAB_ASSERT(target, "SmartRightClick: Target not valid");

	if (!unit || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (unit->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || unit->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(unit->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Repair) && (currentCommand.getTarget() == target))
	{
		return;
	}

	// if nothing prevents it, attack the target
	unit->repair(target);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		BWAPI::Broodwar->drawCircleMap(unit->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		BWAPI::Broodwar->drawLineMap(unit->getPosition(), target->getPosition(), BWAPI::Colors::Cyan);
	}
}


BWAPI::Position Micro::GetKiteVector(BWAPI::Unit unit, BWAPI::Unit target, int range)
{
	BWAPI::Position fleeVec(target->getPosition() - unit->getPosition());
	double fleeAngle = atan2(fleeVec.y, fleeVec.x);
	fleeVec = BWAPI::Position(static_cast<int>(range * cos(fleeAngle)), static_cast<int>(range * sin(fleeAngle)));
	return fleeVec;
}

bool Micro::CheckBuilding(BWAPI::Position pos,BWAPI::UnitType tp)
{
	auto units = BWAPI::Broodwar->getUnitsInRectangle(pos.x - tp.dimensionLeft(), pos.y-tp.dimensionUp(), pos.x+tp.dimensionRight(), pos.y+tp.dimensionDown());
	int left = (pos.x - tp.dimensionLeft()) / 8;
	int right = (pos.x + tp.dimensionRight()) / 8;
	int up = (pos.y - tp.dimensionUp()) / 8;
	int down = (pos.y + tp.dimensionDown()) / 8;
	for (int i = left; i <= right; i++)
	{
		for (int j = up; j <= down; j++)
		if (!BWAPI::Broodwar->isWalkable(i, j))
			return false;
	}
	return units.empty();
}
bool Micro::RealWalkable(BWAPI::Position pos,BWAPI::UnitType tp)
{
	int left = (pos.x - tp.dimensionLeft()) / 8;
	int right = (pos.x + tp.dimensionRight()) / 8;
	int up = (pos.y - tp.dimensionUp()) / 8;
	int down = (pos.y + tp.dimensionDown()) / 8;
	for (int i = left; i <= right; i++)
	{
		for (int j = up; j <= down; j++)
		if (!BWAPI::Broodwar->isWalkable(i, j))
			return false;
	}
	return true;
}
bool Micro::ValidPath(BWAPI::Position from, BWAPI::Position to)
{
	if (!to.isValid() || !from.isValid())
		return false;
	if (!BWAPI::Broodwar->isWalkable(BWAPI::WalkPosition(to)))
		return false;
	if (BWTA::getRegion(to) == BWTA::getRegion(from))
	{
		return true;
	}
	else if (to.isValid()&&BWTA::getRegion(to) != NULL)
	{
		if (BWTA::getNearestChokepoint(from)->getCenter() == BWTA::getNearestChokepoint(to)->getCenter())
		{
			auto pos = BWTA::getNearestChokepoint(from)->getCenter();
			if (pos.getDistance(from) + pos.getDistance(to) < 2*from.getDistance(to))
				return true;
		}
	}
	return false;
}


void Micro::SmarterKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target, BWAPI::Unit Closest, const BWAPI::Unitset& targets, BWAPI::Unit attackee)
{
	UAB_ASSERT(rangedUnit, "SmartKiteTarget: Unit not valid");
	UAB_ASSERT(target, "SmartKiteTarget: Target not valid");
	if (!rangedUnit || !target)
	{
		return;
	}

	double range(rangedUnit->getType().groundWeapon().maxRange());
	if (rangedUnit->getType() == BWAPI::UnitTypes::Protoss_Dragoon && BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge))
	{
		range = 6 * 32;
	}

	// determine whether the target can be kited
	bool kiteLonger = Config::Micro::KiteLongerRangedUnits.find(rangedUnit->getType()) != Config::Micro::KiteLongerRangedUnits.end();
	if (!kiteLonger && (range <= target->getType().groundWeapon().maxRange()))
	{
		// if we can't kite it, there's no point
		Micro::SmartAttackUnit(rangedUnit, attackee);
		return;
	}

	bool    kite(true);
	double  dist(rangedUnit->getDistance(target));
	double  speed(target->getType().topSpeed());


	// if the unit can't attack back don't kite
	if ((rangedUnit->isFlying() && !UnitUtil::CanAttackAir(target)) || (!rangedUnit->isFlying() && !UnitUtil::CanAttackGround(target)))
	{
		//kite = false;
	}


	if (target->getType().isBuilding())
	{
		kite = false;
	}

	auto u = UnitUtil::GetClosestOurUnitTypeToTarget(BWAPI::UnitTypes::Protoss_Zealot, Closest->getPosition());
	// if we can't shoot, run away
	if (kite)
	{
		//already moving ignore new 
		BWAPI::Position fleePosition(0,0);
		if (Closest == rangedUnit)
		{
			BWAPI::Broodwar->drawCircleMap(rangedUnit->getPosition(), 5, BWAPI::Colors::Cyan, true);
			fleePosition = GetKiteVector(rangedUnit, target, std::max(0, 112 - Closest->getDistance(target)), targets);
		}
		else
		{
			BWAPI::Broodwar->drawCircleMap(rangedUnit->getPosition(), 5, BWAPI::Colors::Red, true);
			for (int i = 1, makeroom = 0; i <= 4; i ++)
			{
				makeroom += 16 * (i*(i % 2 == 0 ? -1 : 1));
				fleePosition = rangedUnit->getPosition()+Rotate(GetKiteVector(rangedUnit, target, makeroom), 90);
				if (ValidPath(rangedUnit->getPosition(), fleePosition) && (CheckBuilding(fleePosition, rangedUnit->getType())))
					break;
				fleePosition = BWAPI::Position(0, 0);				
			}
		}
		if (fleePosition != BWAPI::Position(0, 0) && !rangedUnit->isStuck())
		{			
			Micro::SmartMove(rangedUnit, fleePosition);
		}
		else
		{
			//cannot escape
			Micro::SmartAttackUnit(rangedUnit, attackee);
		}		
	}
	// otherwise shoot
	else
	{
		Micro::SmartAttackUnit(rangedUnit, attackee);
	}
}


void Micro::SmartKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target,BWAPI::Unit dodge,BWAPI::Unitset&targets)
{
	UAB_ASSERT(rangedUnit, "SmartKiteTarget: Unit not valid");
	UAB_ASSERT(target, "SmartKiteTarget: Target not valid");
	
	if (!rangedUnit || !target)
	{
		return;
	}

	double range(rangedUnit->getType().groundWeapon().maxRange());
	if (rangedUnit->getType() == BWAPI::UnitTypes::Protoss_Dragoon && BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge))
	{
		range = 6 * 32;
	}

	// determine whether the target can be kited
	bool kiteLonger = Config::Micro::KiteLongerRangedUnits.find(rangedUnit->getType()) != Config::Micro::KiteLongerRangedUnits.end();
	if (!kiteLonger && (range <= dodge->getType().groundWeapon().maxRange()))
	{
		// if we can't kite it, there's no point
		Micro::SmartAttackUnit(rangedUnit, target);
		return;
	}

	bool    kite(true);
	double  dist(rangedUnit->getDistance(target));
	double  speed(rangedUnit->getType().topSpeed());


	// if the unit can't attack back don't kite
	if ((rangedUnit->isFlying() && !UnitUtil::CanAttackAir(target)) || (!rangedUnit->isFlying() && !UnitUtil::CanAttackGround(target)))
	{
		//kite = false;
	}

	double timeToEnter = std::max(0.0, (dist - range) / speed);

	if ((timeToEnter >= rangedUnit->getGroundWeaponCooldown()))
	{
		kite = false;
	}

	if (target->getType().isBuilding())
	{
		kite = false;
	}

	double minDis = rangedUnit->getGroundWeaponCooldown()*speed;

	// if we can't shoot, run away
	if (kite)
	{
		BWAPI::Position fleePosition = GetKiteVector(rangedUnit, dodge, std::min(minDis,range - rangedUnit->getDistance(dodge)), targets);
		BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), fleePosition, BWAPI::Colors::Cyan);
		BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), target->getPosition(), BWAPI::Colors::Yellow);
		if (fleePosition != BWAPI::Position(0, 0)&&!rangedUnit->isStuck())
		{
			BWAPI::Broodwar->drawCircleMap(rangedUnit->getPosition(), 5, BWAPI::Colors::Green, true);
			Micro::SmartMove(rangedUnit, fleePosition);
		}
		else
		{
			//cannot escape

			if (rangedUnit->isInWeaponRange(target))
				Micro::SmartAttackUnit(rangedUnit, target);
			else
				Micro::SmartAttackUnit(rangedUnit, dodge);
		}
	}
	// otherwise shoot
	else
	{
		if (rangedUnit->isInWeaponRange(target))
			Micro::SmartAttackUnit(rangedUnit, target);
		else 
			Micro::SmartAttackUnit(rangedUnit, dodge);
	}
}


/*
void Micro::SmartKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target)
{
	UAB_ASSERT(rangedUnit, "SmartKiteTarget: Unit not valid");
	UAB_ASSERT(target, "SmartKiteTarget: Target not valid");

	if (!rangedUnit || !target)
	{
		return;
	}

	double range(rangedUnit->getType().groundWeapon().maxRange());
	if (rangedUnit->getType() == BWAPI::UnitTypes::Protoss_Dragoon && BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge))
	{
		range = 6 * 32;
	}

	// determine whether the target can be kited
	bool kiteLonger = Config::Micro::KiteLongerRangedUnits.find(rangedUnit->getType()) != Config::Micro::KiteLongerRangedUnits.end();
	if (!kiteLonger && (range <= target->getType().groundWeapon().maxRange()))
	{
		// if we can't kite it, there's no point
		Micro::SmartAttackUnit(rangedUnit, target);
		return;
	}

	bool    kite(true);
	double  dist(rangedUnit->getDistance(target));
	double  speed(rangedUnit->getType().topSpeed());


	// if the unit can't attack back don't kite
	if ((rangedUnit->isFlying() && !UnitUtil::CanAttackAir(target)) || (!rangedUnit->isFlying() && !UnitUtil::CanAttackGround(target)))
	{
		//kite = false;
	}

	double timeToEnter = std::max(0.0, (dist - range) / speed);
	if ((timeToEnter >= rangedUnit->getGroundWeaponCooldown()))
	{
		kite = false;
	}

	if (target->getType().isBuilding())
	{
		kite = false;
	}

	// if we can't shoot, run away
	if (kite)
	{
		BWAPI::Broodwar->drawTextScreen(200, 340, "%s", "in the kite");

		BWAPI::Position fleePosition(rangedUnit->getPosition() - target->getPosition() + rangedUnit->getPosition());
		//BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), fleePosition, BWAPI::Colors::Cyan);
		Micro::SmartMove(rangedUnit, fleePosition);
	}
	// otherwise shoot
	else
	{
		Micro::SmartAttackUnit(rangedUnit, target);
	}
}
*/

/*
void Micro::MutaDanceTarget(BWAPI::Unit muta, BWAPI::Unit target)
{
	UAB_ASSERT(muta, "MutaDanceTarget: Muta not valid");
	UAB_ASSERT(target, "MutaDanceTarget: Target not valid");

	if (!muta || !target)
	{
		return;
	}

	const int cooldown = muta->getType().groundWeapon().damageCooldown();
	const int latency = BWAPI::Broodwar->getLatency();
	const double speed = muta->getType().topSpeed();
	const double range = muta->getType().groundWeapon().maxRange();
	const double distanceToTarget = muta->getDistance(target);
	const double distanceToFiringRange = std::max(distanceToTarget - range, 0.0);
	const double timeToEnterFiringRange = distanceToFiringRange / speed;
	const int framesToAttack = static_cast<int>(timeToEnterFiringRange)+2 * latency;

	// How many frames are left before we can attack?
	const int currentCooldown = muta->isStartingAttack() ? cooldown : muta->getGroundWeaponCooldown();

	BWAPI::Position fleeVector = GetKiteVector(target, muta);
	BWAPI::Position moveToPosition(muta->getPosition() + fleeVector);

	// If we can attack by the time we reach our firing range
	if (currentCooldown <= framesToAttack)
	{
		// Move towards and attack the target
		muta->attack(target);
	}
	else // Otherwise we cannot attack and should temporarily back off
	{
		// Determine direction to flee
		// Determine point to flee to
		if (moveToPosition.isValid())
		{
			muta->rightClick(moveToPosition);
		}
	}
}
*/

BWAPI::Position Micro::GetKiteVector(BWAPI::Unit unit, BWAPI::Unit target,int range,const BWAPI::Unitset& targets)
{
	auto newPos = target->getPosition()*2 + targets.getPosition();
	auto occupied = InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->self());
	auto dist = target->getDistance(unit);
	BWAPI::Position hm;
	newPos.x /= 3;
	newPos.y /= 3;
	int k = 1;
	if (occupied.find(BWTA::getRegion(unit->getPosition())) != occupied.end())
	{
		hm = BWTA::getRegion(unit->getPosition())->getCenter();
		k = 4;
	}
	else
		hm = RegionManager::Instance().optimalRegroupPosition(unit->getPosition());
	if (unit->getDistance(target) < 64)
	{
		newPos = targets.getPosition();
	}
	BWAPI::Position fleeVec1(unit->getPosition() - newPos);
	double fleeAngle = atan2((double)fleeVec1.y, (double)fleeVec1.x);
	fleeVec1.x = static_cast<int>((range) * cos(fleeAngle));
	fleeVec1.y = static_cast<int>((range) * sin(fleeAngle));
	auto fleeVec2 = hm - unit->getPosition();
	fleeAngle = atan2(fleeVec2.y, fleeVec2.x);
	fleeVec2.x = static_cast<int>(range / 2 * cos(fleeAngle));
	fleeVec2.y = static_cast<int>(range / 2 * sin(fleeAngle));
	auto res = fleeVec1 + fleeVec2;
	fleeAngle = atan2(res.y, res.x);
	res.x = static_cast<int>((range) * cos(fleeAngle));
	res.y = static_cast<int>((range) * sin(fleeAngle));

	auto flee = fleeVec1;
	auto rangedUnit = unit;
	BWAPI::Position fleePosition = flee + rangedUnit->getPosition();
	BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), fleePosition, BWAPI::Colors::Cyan);
	BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), target->getPosition(), BWAPI::Colors::Yellow);
	int angle = 0;

	bool found = false;
	int longer = BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge) ? 8 : 6;

	for (int i = 0; i <= 180; i += 15)
	{
		auto rotatedFlee = Rotate(flee, angle);
		auto shorterFlee = rotatedFlee;
		shorterFlee.x /= 4;
		shorterFlee.y /= 4;
		fleePosition = rotatedFlee + rangedUnit->getPosition();
		if (ValidPath(rangedUnit->getPosition(), fleePosition))
		{
			for (int j = 0; j < longer; j++)
			{
				fleePosition = rotatedFlee + rangedUnit->getPosition();
				if (ValidPath(rangedUnit->getPosition(), fleePosition) && (CheckBuilding(fleePosition, rangedUnit->getType())))
				{
					return fleePosition;
				}
				rotatedFlee = rotatedFlee + shorterFlee;
			}
		}
		angle += (i*(i % 2 == 0 ? -1 : 1));
	}
	return BWAPI::Position(0, 0);	
}



const double PI = 3.14159265;
BWAPI::Position Micro::Rotate(BWAPI::Position old, double angle)
{
	angle = angle*PI / 180.0;
	auto res = old;
	res.x = (old.x * cos(angle)) - (old.y * sin(angle));
	res.y = (old.y * cos(angle)) + (old.x * sin(angle));
	return res;
}

void Micro::Normalize(double &x, double &y)
{
	double length = sqrt((x * x) + (y * y));
	if (length != 0)
	{
		x = (x / length);
		y = (y / length);
	}
}


double Micro::getGroupDamages(const BWAPI::Unitset & units){
	double damages = 0;
	for (auto unit : units){
		damages += (double)unit->getType().groundWeapon().damageAmount();
		damages += (double)unit->getType().airWeapon().damageAmount();
		damages += (double)unit->getType().armor();
	}
	return damages;
}

double Micro::getGroupHealth(const BWAPI::Unitset & units){
	double groupHealth = 0;
	for (auto unit : units){
		groupHealth += (double)unit->getHitPoints() + (double)unit->getShields();
	}
	return groupHealth;
}
