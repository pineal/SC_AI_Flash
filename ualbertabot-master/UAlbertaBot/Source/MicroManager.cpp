#include "MicroManager.h"
#include "UnitUtil.h"
using namespace UAlbertaBot;

MicroManager::MicroManager() :_formation(true), fighting(false), formed(false)
{
}

void MicroManager::setUnits(const BWAPI::Unitset & u) 
{ 
	_units = u; 
}

//set whether it needs to form
void MicroManager::setFormation(bool f)
{
	_formation = f;
}


BWAPI::Position MicroManager::calcCenter() const
{
    if (_units.empty())
    {
        if (Config::Debug::DrawSquadInfo)
        {
            BWAPI::Broodwar->printf("Squad::calcCenter() called on empty squad");
        }
        return BWAPI::Position(0,0);
    }

	BWAPI::Position accum(0,0);
	for (auto & unit : _units)
	{
		accum += unit->getPosition();
	}
	return BWAPI::Position(accum.x / _units.size(), accum.y / _units.size());
}

void MicroManager::execute(const SquadOrder & inputOrder, std::string inputSquadName)
{
	// Nothing to do if we have no units
	if (_units.empty() || !(inputOrder.getType() == SquadOrderTypes::Attack || inputOrder.getType() == SquadOrderTypes::Defend))
	{
		fighting = false;
		formed = true;
		return;
	}

	order = inputOrder;
	SquadName = inputSquadName;
	drawOrderText();

	// Discover enemies within region of interest

	// if the order is to defend, we only care about units in the radius of the defense
	if (order.getType() == SquadOrderTypes::Defend)
	{
		MapGrid::Instance().GetUnits(nearbyEnemies, order.getPosition(), order.getRadius(), false, true);
	
	} // otherwise we want to see everything on the way
	else if (order.getType() == SquadOrderTypes::Attack) 
	{
		MapGrid::Instance().GetUnits(nearbyEnemies, order.getPosition(), order.getRadius(), false, true);
		for (auto & unit : _units) 
		{
			BWAPI::Unit u = unit;
			BWAPI::UnitType t = u->getType();
			MapGrid::Instance().GetUnits(nearbyEnemies, unit->getPosition(), order.getRadius(), false, true);
		}
	}

	// the following block of code attacks all units on the way to the order position
	// we want to do this if the order is attack, defend, or harass
	if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend) 
	{
        // if this is a worker defense force, only attack workers, unless there is no enemy workers
        if (SquadName == "ScoutDefense")
        {
			BWAPI::Unitset enemyWorkers;
			for (auto & enemyUnit : nearbyEnemies){
				if (enemyUnit->getType().isWorker())
					enemyWorkers.insert(enemyUnit);
			}
			if (!enemyWorkers.empty()){
				executeMicro(enemyWorkers);
				BWAPI::Broodwar->drawTextScreen(200, 310, "%s", "Enemy Worker Found! Scout Defense!");
			}
			else{
				executeMicro(nearbyEnemies);
				BWAPI::Broodwar->drawTextScreen(200, 310, "%s", "No worker target for ScoutDefense Squad");
			}
        }
        // otherwise it is a normal attack force
        else
        {
            // if this is a defense squad then we care about all units in the area
            if (order.getType() == SquadOrderTypes::Defend)
            {
                executeMicro(nearbyEnemies);
            }
            // otherwise we only care about workers if they are in their own region
            else
            {
                 // if this is the an attack squad
                BWAPI::Unitset workersRemoved;

                for (auto & enemyUnit : nearbyEnemies) 
		        {
                    // if its not a worker add it to the targets
			        if (!enemyUnit->getType().isWorker())
                    {
                        workersRemoved.insert(enemyUnit);
                    }
                    // if it is a worker
                    else
                    {
                        for (BWTA::Region * enemyRegion : InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->enemy()))
                        {
                            // only add it if it's in their region
                            if (BWTA::getRegion(BWAPI::TilePosition(enemyUnit->getPosition())) == enemyRegion)
                            {
                                workersRemoved.insert(enemyUnit);
                            }
                        }
                    }
		        }

		        // Allow micromanager to handle enemies
		        executeMicro(workersRemoved);
            }
        }
	}	
}

const BWAPI::Unitset & MicroManager::getUnits() const 
{ 
    return _units; 
}

//whether need to form
bool MicroManager ::getFormation() {
	return _formation;
}

//whether fighting
bool MicroManager::getFighting() {
	return fighting;
}

//dist is distance between middle unit and target gravity center, interval is length of arc between two units
bool MicroManager::formSquad(const BWAPI::Unitset & targets, int dist, int radius, double angle, int interval)
{
	const BWAPI::Unitset & units = getUnits();
	BWAPI::Position tpos = targets.getPosition();
	BWAPI::Position mpos = units.getPosition();
	bool a = false;
	int valid_number = 0;

	//do not from squad when fighting or close to enemy
	for (auto & unit : units){
		if (unit->isUnderAttack() || unit->isAttackFrame() || unit->getDistance(tpos) < 80)
			a = true;
		if (unit->getDistance(tpos) < 400){
			valid_number++;
		}
	}
	if (a) {
		fighting = true;
		return false;
	}
	else {
		fighting = false;
	}
	//if there is no target, don't form
	if (targets.size() == 0){
		return false;
	}
	//if there are less than 4 units, no need to form
	if (valid_number < 4){
		return true;
	}
	//if there is a building near the unit, do not form
	for (auto & target : targets){
		auto type = target->getType();
		if (type.isBuilding()){
			return false;
		}
	}
	//Formation is set false by Squad for 5 seconds after formation finished once or whichever manager is attacked
	if (!getFormation()){
		return false;
	}
	//BWAPI::Broodwar->drawTextScreen(200, 340, "%s", "Forming");

	const double PI = 3.14159265;

	double ang = angle / 180 * PI;

	//the angle of mid_point on arc
	double m_ang = atan2(mpos.y - tpos.y, mpos.x - tpos.x);
	//circle center
	int cx = (int)(tpos.x - (radius - dist) * cos(m_ang));
	int cy = (int)(tpos.y - (radius - dist) * sin(m_ang));
	BWAPI::Position c;
	c.x = cx; c.y = cy;
	//mid_point on arc
	BWAPI::Position m;
	m.x = (int)(cx + radius*cos(m_ang));
	m.y = (int)(cy + radius*sin(m_ang));
	//BWAPI::Broodwar->drawLineMap(c, m, BWAPI::Colors::Yellow);

	BWAPI::Unitset unassigned;
	for (auto & unit : units){
		unassigned.insert(unit);
	}

	//move every positions on the arc to the closest unit
	BWAPI::Position tmp;
	int try_time = 0;
	int r = radius;
	int total_dest_dist = 0;
	int num_assigned = 0;

	while (unassigned.size() > 0 && try_time < 5){
		double ang_interval = interval * 1.0 / r;
		double final_ang;
		int num_to_assign;
		int max_units = (int)(ang / ang_interval) + 1;
		if (unassigned.size() < (unsigned)max_units){
			num_to_assign = unassigned.size();
			final_ang = ang_interval * num_to_assign;
		}
		else {
			num_to_assign = max_units;
			final_ang = ang;
		}
		for (int i = 0; i < num_to_assign; i++) {
			//assign from two ends to middle
			double a = m_ang + pow(-1, i % 2)*(final_ang / 2 - (i / 2)*ang_interval);
			int min_dist = MAXINT;
			BWAPI::Unit closest_unit = nullptr;
			tmp.x = (int)(cx + r * cos(a));
			tmp.y = (int)(cy + r * sin(a));
			for (auto & unit : unassigned){
				int d = unit->getDistance(tmp);
				if (d < min_dist){
					min_dist = d;
					closest_unit = unit;
				}
			}
			//if it's a unit far away from fight, do not assign it to a position
			if (closest_unit && min_dist > 32*15){
				unassigned.erase(closest_unit);
				continue;
			}
			BWAPI::WalkPosition WP(tmp);
			//the valid form position should be walkable and in the same region as the arc
			if (tmp.isValid() && Micro::RealWalkable(tmp, closest_unit->getType()) && closest_unit && BWTA::getRegion(tmp) == BWTA::getRegion(m)){
				BWAPI::Broodwar->drawLineMap(closest_unit->getPosition(), tmp, BWAPI::Colors::Brown);
				//if this unit is already at the point, attackmove, otherwise move
				if (closest_unit->getDistance(tmp) > 32)
					Micro::SmartMove(closest_unit, tmp);
				else
					Micro::SmartAttackMove(closest_unit, tmp);
				unassigned.erase(closest_unit);
				//find the total distance between unit and destination
				total_dest_dist += min_dist;
				num_assigned++;
			}
		}
		r += interval;
		try_time++;
	}

	//if not assigned, move it to base
	for (auto & unit : unassigned) {
		Micro::SmartMove(unit, tpos);
	}

	//if max destination distance less than 32, means forming has been finished
	if (num_assigned > 0 && total_dest_dist / num_assigned <= 32){
		return true;
	}
	else {
		return false;
	}
}
BWAPI::Position MicroManager::positionShift(const BWAPI::Position &from, const BWAPI::Position &to, int distance)
{
	double theta = atan2(static_cast<double>(from.y - to.y), static_cast<double>(from.x - to.x));
	BWAPI::Position res;
	res.x = static_cast<int>(distance*cos(theta));
	res.y = static_cast<int>(distance*sin(theta));
	return res;
}

bool MicroManager::regroupForm(const BWAPI::Position & tpos, int dist, int radius, double angle, int interval) {
	const BWAPI::Unitset & units = getUnits();
	//get the last point on the path between regroup position and base, so that we can calculate the arc to form
	BWAPI::Position mpos = RegionManager::Instance().getNextPointToBase(BWAPI::TilePosition(tpos));
	if (mpos.x == tpos.x && mpos.y == tpos.y) {
		mpos = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	}

	bool f = false;

	const double PI = 3.14159265;

	double ang = angle / 180 * PI;

	//the angle of mid_point on arc
	double m_ang = atan2(mpos.y - tpos.y, mpos.x - tpos.x);
	//circle center
	int cx = (int)(tpos.x - (radius - dist) * cos(m_ang));
	int cy = (int)(tpos.y - (radius - dist) * sin(m_ang));
	BWAPI::Position c;
	c.x = cx; c.y = cy;
	//mid_point on arc
	BWAPI::Position m;
	m.x = (int)(cx + radius*cos(m_ang));
	m.y = (int)(cy + radius*sin(m_ang));
	//BWAPI::Broodwar->drawLineMap(c, m, BWAPI::Colors::Yellow);

	BWAPI::Unitset unassigned;
	for (auto & unit : units){
		//Intercepters don't regroup form
		if (unit->getType() != BWAPI::UnitTypes::Protoss_Interceptor){
			unassigned.insert(unit);
		}
	}

	//move every positions on the arc to the closest unit
	BWAPI::Position tmp;
	int try_time = 0;
	int r = radius;
	int total_dest_dist = 0;
	int num_assigned = 0;

	while (unassigned.size() > 0 && try_time < 5){
		double ang_interval = interval * 1.0 / r;
		double final_ang;
		int num_to_assign;
		int max_units = (int)(ang / ang_interval) + 1;
		if (unassigned.size() < (unsigned)max_units){
			num_to_assign = unassigned.size();
			final_ang = ang_interval * num_to_assign;
		}
		else {
			num_to_assign = max_units;
			final_ang = ang;
		}
		for (int i = 0; i < num_to_assign; i++) {
			//assign from two ends to middle
			double a = m_ang + pow(-1, i % 2)*(final_ang / 2 - (i / 2)*ang_interval);
			int min_dist = MAXINT;
			BWAPI::Unit closest_unit = nullptr;
			tmp.x = (int)(cx + r * cos(a));
			tmp.y = (int)(cy + r * sin(a));
			for (auto & unit : unassigned){
				int d = unit->getDistance(tmp);
				if (d < min_dist){
					min_dist = d;
					closest_unit = unit;
				}
			}
			BWAPI::WalkPosition WP(tmp);
			//the valid form position should be walkable and in the same region as the arc
			if (tmp.isValid() && Micro::RealWalkable(tmp,closest_unit->getType()) && closest_unit && BWTA::getRegion(tmp) == BWTA::getRegion(mpos)){
				BWAPI::Broodwar->drawLineMap(closest_unit->getPosition(), tmp, BWAPI::Colors::Brown);
				//if this unit is already at the point, attackmove, otherwise move
				if (closest_unit->getDistance(tmp) > 32)
				{
					Micro::SmartMove(closest_unit, tmp);
				}
				else{
					Micro::SmartAttackMove(closest_unit, tmp);
					if (closest_unit->isUnderAttack() || closest_unit->isAttackFrame())
						f = true;
				}
				unassigned.erase(closest_unit);
				//find the total distance between unit and destination, do not consider units far away
				if (min_dist < 1000){
					total_dest_dist += min_dist;
					num_assigned++;
				}
			}
		}
		r += interval;
		try_time++;
	}

	//if not assigned, move it to base
	for (auto & unit : unassigned) {
		Micro::SmartMove(unit, BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));
	}

	//if there is a unit already arrived the point and is fighting
	if (f) {
		fighting = true;
	}
	else {
		fighting = false;
	}

	//if no units, regard it as formed
	if (num_assigned == 0){
		return true;
	}
	//if average destination distance less than 48, means forming has been finished
	else if (num_assigned > 0 && total_dest_dist / num_assigned <= 32){
		return true;
	}
	else {
		return false;
	}
}


bool MicroManager::regroup(const BWAPI::Position & regroupPosition, int dist, int interval, int radius, double angle)
{
	if (regroupForm(regroupPosition, dist, radius, angle, interval)){
		return true;
	}
	else{
		return false;
	}
}

void MicroManager::regroup1(const BWAPI::Position & regroupPosition) const
{
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	int regroupDistanceFromBase = MapTools::Instance().getGroundDistance(regroupPosition, ourBasePosition);

	// for each of the units we have
	for (auto & unit : _units)
	{
		int unitDistanceFromBase = MapTools::Instance().getGroundDistance(unit->getPosition(), ourBasePosition);

		// if the unit is outside the regroup area
		if (unit->getDistance(regroupPosition) >Config::Micro::CombatRegroupRadius)
		{
			// regroup n
			Micro::SmartMove(unit, regroupPosition);
		}
		else
		{
			Micro::SmartAttackMove(unit, unit->getPosition());
		}
	}
}
void MicroManager::regroup2(const BWAPI::Position & regroupPosition) const
{
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	int regroupDistanceFromBase = MapTools::Instance().getGroundDistance(regroupPosition, ourBasePosition);
	int realRegroupPositionRadius = Config::Micro::CombatRegroupRadius + 50 * static_cast<int>(sqrt(_units.size()));
	if (UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Zealot) <= 2)
		Config::Micro::CombatRegroupRadius = 150;
	// for each of the units we have
	for (auto & unit : _units)
	{
		int unitDistanceFromBase = MapTools::Instance().getGroundDistance(unit->getPosition(), ourBasePosition);
		int factor = unit->getType() == BWAPI::UnitTypes::Protoss_Dragoon ? 4 : 1;
		// if the unit is outside the regroup area
		if (unitDistanceFromBase > regroupDistanceFromBase)
		{
			
			if (unit->getDistance(ourBasePosition) > (realRegroupPositionRadius*factor))
			{
				// regroup it
				Micro::SmartMove(unit, ourBasePosition);
			}
			else
			{
				Micro::SmartAttackMove(unit, unit->getPosition());
			}
		}
		else if (unit->getDistance(regroupPosition) > (Config::Micro::CombatRegroupRadius + 50 * static_cast<int>(sqrt(_units.size()))*factor))
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




bool MicroManager::unitNearEnemy(BWAPI::Unit unit) const
{
	assert(unit);

	BWAPI::Unitset enemyNear;

	MapGrid::Instance().GetUnits(enemyNear, unit->getPosition(), 800, false, true);

	return enemyNear.size() > 0;
}

// returns true if position:
// a) is walkable
// b) doesn't have buildings on it
// c) doesn't have a unit on it that can attack ground
bool MicroManager::checkPositionWalkable(BWAPI::Position pos) 
{
	// get x and y from the position
	int x(pos.x), y(pos.y);

	// walkable tiles exist every 8 pixels
	bool good = BWAPI::Broodwar->isWalkable(x/8, y/8);
	
	// if it's not walkable throw it out
	if (!good) return false;
	
	// for each of those units, if it's a building or an attacking enemy unit we don't want to go there
	for (auto & unit : BWAPI::Broodwar->getUnitsOnTile(x/32, y/32)) 
	{
		if	(unit->getType().isBuilding() || unit->getType().isResourceContainer() || 
			(unit->getPlayer() != BWAPI::Broodwar->self() && unit->getType().groundWeapon() != BWAPI::WeaponTypes::None)) 
		{		
				return false;
		}
	}

	// otherwise it's okay
	return true;
}

void MicroManager::trainSubUnits(BWAPI::Unit unit) const
{
	if (unit->getType() == BWAPI::UnitTypes::Protoss_Reaver)
	{
		unit->train(BWAPI::UnitTypes::Protoss_Scarab);
	}
	else if (unit->getType() == BWAPI::UnitTypes::Protoss_Carrier)
	{
		unit->train(BWAPI::UnitTypes::Protoss_Interceptor);
	}
}

bool MicroManager::unitNearChokepoint(BWAPI::Unit unit) const
{
	for (BWTA::Chokepoint * choke : BWTA::getChokepoints())
	{
		if (unit->getDistance(choke->getCenter()) < 80)
		{
			return true;
		}
	}

	return false;
}

void MicroManager::drawOrderText() 
{
	for (auto & unit : _units) 
    {
		if (Config::Debug::DrawUnitTargetInfo) BWAPI::Broodwar->drawTextMap(unit->getPosition().x, unit->getPosition().y, "%s", order.getStatus().c_str());
	}
}
