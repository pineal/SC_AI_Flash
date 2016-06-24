#include "Common.h"
#include "GameCommander.h"
#include "UnitUtil.h"
#include "DecisionMaker.h"

using namespace UAlbertaBot;

GameCommander::GameCommander() 
    : _initialScoutSet(false)
{
	
}

void GameCommander::update()
{
	RegionManager::Instance().drawAllPolygon(BWAPI::Colors::Green);
	RegionManager::Instance().drawPlayerRegion(BWAPI::Broodwar->self(), BWAPI::Colors::Cyan);
	RegionManager::Instance().drawPlayerRegion(BWAPI::Broodwar->enemy(), BWAPI::Colors::Red);
	RegionManager::Instance().drawJointPoints();
	RegionManager::Instance().drawUnwalkableRegion();
	RegionManager::Instance().drawScores();

	_timerManager.startTimer(TimerManager::All);
	// populate the unit vectors we will pass into various managers
	handleUnitAssignments();
	DecisionMaker::Instance().getStatistics().on_frame();
	//Debugger::Instance().draw_info();
	// utility managers
	_timerManager.startTimer(TimerManager::InformationManager);
	InformationManager::Instance().update();
	RegionManager::Instance().valueUpdate();
	_timerManager.stopTimer(TimerManager::InformationManager);

	_timerManager.startTimer(TimerManager::MapGrid);
	MapGrid::Instance().update();
	_timerManager.stopTimer(TimerManager::MapGrid);

	_timerManager.startTimer(TimerManager::MapTools);
	//MapTools::Instance().update();
	_timerManager.stopTimer(TimerManager::MapTools);

	_timerManager.startTimer(TimerManager::Search);
	BOSSManager::Instance().update(35 - _timerManager.getTotalElapsed());
	_timerManager.stopTimer(TimerManager::Search);

	// economy and base managers
	_timerManager.startTimer(TimerManager::Worker);
	WorkerManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Worker);

	_timerManager.startTimer(TimerManager::Production);
	ProductionManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Production);

	_timerManager.startTimer(TimerManager::Building);
	BuildingManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Building);

	// combat and scouting managers
	_timerManager.startTimer(TimerManager::Combat);
	_combatCommander.update(_combatUnits);
	_timerManager.stopTimer(TimerManager::Combat);

	_timerManager.startTimer(TimerManager::Scout);
    ScoutManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Scout);
//	RegionManager::Instance().init();
	_timerManager.stopTimer(TimerManager::All);

	drawDebugInterface();


}


/*
void GameCommander::getPlayerRegion(BWAPI::Player player, BWAPI::Color color){
	BWAPI::Position regroupPos, regroupHom;
	for (BWTA::Region * region : InformationManager::Instance().getOccupiedRegions(player)){
		
				
		BWTA::Polygon p = region->getPolygon();

		for (int j = 0; j<(int)p.size(); j++)
		{
			BWAPI::Position point1 = p[j];
			BWAPI::Position point2 = p[(j + 1) % p.size()];
			BWAPI::Broodwar->drawLineMap(point1, point2, color);
		}
	
		for (auto boundries : region->getChokepoints()){
			//boundries->getCenter()
			BWAPI::Broodwar->drawCircleMap(boundries->getCenter(), 10, BWAPI::Colors::Orange, true);
			BWAPI::Broodwar->drawLineMap(boundries->getSides().first, boundries->getSides().second, BWAPI::Colors::Orange);
		}
		auto startRegion = BWTA::getRegion(BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition());
		if (startRegion->getChokepoints().size() != 1)
			return ;
		else
		{
			for (auto boundries : startRegion->getChokepoints()){
				//boundries->getCenter()
				auto line = boundries->getSides();
				BWAPI::Position pos = line.second - line.first;
				BWAPI::Position hom = startRegion->getCenter() - boundries->getCenter();
				std::swap(pos.x, pos.y);
				pos.x *= -1;
				double length = sqrt(pos.x*pos.x + pos.y * pos.y);
				BWAPI::Position pos1(pos);
				pos.x = 160 * pos.x / length;
				pos.y = 160 * pos.y / length;

				pos1.x = 180 * pos1.x / length;
				pos1.y = 180 * pos1.y / length;

				length = sqrt(hom.x*hom.x + hom.y * hom.y);
				hom.x = 180 * hom.x / length;
				hom.y = 180 * hom.y / length;
				regroupHom = boundries->getCenter() + hom;
				if (BWTA::getRegion(boundries->getCenter() + pos) == startRegion)
					regroupPos =  std::pair<BWAPI::Position, BWAPI::Position>(boundries->getCenter() + pos, boundries->getCenter() + pos1).second;
				else
					regroupPos =  std::pair<BWAPI::Position, BWAPI::Position>(boundries->getCenter() - pos, boundries->getCenter() - pos1).second;

			}			
		}
		BWAPI::Broodwar->drawCircleMap(regroupPos, 10, BWAPI::Colors::Orange, true);
		BWAPI::Broodwar->drawCircleMap(regroupHom, 10, BWAPI::Colors::Yellow, true);
	}
}
*/


void GameCommander::drawDebugInterface()
{
	InformationManager::Instance().drawExtendedInterface();
	InformationManager::Instance().drawUnitInformation(425,30);
	InformationManager::Instance().drawMapInformation();
	BuildingManager::Instance().drawBuildingInformation(200,50);
	BuildingPlacer::Instance().drawReservedTiles();
	ProductionManager::Instance().drawProductionInformation(30, 50);
	BOSSManager::Instance().drawSearchInformation(490, 100);
    BOSSManager::Instance().drawStateInformation(250, 0);
    
	_combatCommander.drawSquadInformation(200, 30);
    _timerManager.displayTimers(490, 225);
    drawGameInformation(4, 1);

	// draw position of mouse cursor
	/*
	if (Config::Debug::DrawMouseCursorInfo)
	{
		int mouseX = BWAPI::Broodwar->getMousePosition().x + BWAPI::Broodwar->getScreenPosition().x;
		int mouseY = BWAPI::Broodwar->getMousePosition().y + BWAPI::Broodwar->getScreenPosition().y;
		BWAPI::Broodwar->drawTextMap(mouseX + 20, mouseY, " %d %d", mouseX, mouseY);
	}
	*/
}

void GameCommander::drawGameInformation(int x, int y)
{
    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Players:");
	BWAPI::Broodwar->drawTextScreen(x+50, y, "%c%s \x04vs. %c%s", BWAPI::Broodwar->self()->getTextColor(), BWAPI::Broodwar->self()->getName().c_str(), 
                                                                  BWAPI::Broodwar->enemy()->getTextColor(), BWAPI::Broodwar->enemy()->getName().c_str());
	y += 12;
		
    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Strategy:");
	BWAPI::Broodwar->drawTextScreen(x+50, y, "\x03%s %s", Config::Strategy::StrategyName.c_str(), Config::Strategy::FoundEnemySpecificStrategy ? "(enemy specific)" : "");
	BWAPI::Broodwar->setTextSize();
	y += 12;

    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Map:");
	BWAPI::Broodwar->drawTextScreen(x+50, y, "\x03%s", BWAPI::Broodwar->mapFileName().c_str());
	BWAPI::Broodwar->setTextSize();
	y += 12;

    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Time:");
    BWAPI::Broodwar->drawTextScreen(x+50, y, "\x04%d %4dm %3ds", BWAPI::Broodwar->getFrameCount(), (int)(BWAPI::Broodwar->getFrameCount()/(23.8*60)), (int)((int)(BWAPI::Broodwar->getFrameCount()/23.8)%60));
}

// assigns units to various managers
void GameCommander::handleUnitAssignments()
{
	_validUnits.clear();
    _combatUnits.clear();

	// filter our units for those which are valid and usable
	setValidUnits();

	// set each type of unit
	setScoutUnits();
	setCombatUnits();
}

bool GameCommander::isAssigned(BWAPI::Unit unit) const
{
	return _combatUnits.contains(unit) || _scoutUnits.contains(unit);
}

// validates units as usable for distribution to various managers
void GameCommander::setValidUnits()
{
	// make sure the unit is completed and alive and usable
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (UnitUtil::IsValidUnit(unit))
		{	
			_validUnits.insert(unit);
		}
	}
}

void GameCommander::setScoutUnits()
{
	//BWAPI::Broodwar->drawTextScreen(200, 300, "%d", int(_scoutUnits.size()));
	if (!_scoutUnits.empty())
	{
		BWAPI::Unit scout = *(_scoutUnits.begin());
		if (!scout->exists())
		{
			_scoutUnits.clear();
		}
	}

    // if we haven't set a scout unit, do it
    if (_scoutUnits.empty())// && !_initialScoutSet)
    {
        BWAPI::Unit supplyProvider = getFirstSupplyProvider();

		// if it exists
		if (supplyProvider)
		{
			// grab the closest worker to the supply provider to send to scout
			BWAPI::Unit workerScout = getClosestWorkerToTarget(supplyProvider->getPosition());

			// if we find a worker (which we should) add it to the scout units
			if (workerScout)
			{
                ScoutManager::Instance().setWorkerScout(workerScout);
				assignUnit(workerScout, _scoutUnits);
                //_initialScoutSet = true;
			}
		}
    }
}

// sets combat units to be passed to CombatCommander
void GameCommander::setCombatUnits()
{
	for (auto & unit : _validUnits)
	{
		if (!isAssigned(unit) && (UnitUtil::IsCombatUnit(unit) || unit->getType().isWorker()))		
		{	
			assignUnit(unit, _combatUnits);
		}
	}
}

BWAPI::Unit GameCommander::getFirstSupplyProvider()
{
	BWAPI::Unit supplyProvider = nullptr;

	if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg)
	{
		for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		{
			if (unit->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool)
			{
				supplyProvider = unit;
			}
		}
	}
	else
	{
		
		for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		{
			if (unit->getType() == BWAPI::Broodwar->self()->getRace().getSupplyProvider())
			{
				supplyProvider = unit;
			}
		}
	}

	return supplyProvider;
}

void GameCommander::onUnitShow(BWAPI::Unit unit)			
{ 
	InformationManager::Instance().onUnitShow(unit); 
	WorkerManager::Instance().onUnitShow(unit);
}

void GameCommander::onUnitHide(BWAPI::Unit unit)			
{ 
	InformationManager::Instance().onUnitHide(unit); 
}

void GameCommander::onUnitCreate(BWAPI::Unit unit)		
{ 
	InformationManager::Instance().onUnitCreate(unit); 
}

void GameCommander::onUnitComplete(BWAPI::Unit unit)
{
	InformationManager::Instance().onUnitComplete(unit);
}

void GameCommander::onUnitRenegade(BWAPI::Unit unit)		
{ 
	InformationManager::Instance().onUnitRenegade(unit); 
}

void GameCommander::onUnitDestroy(BWAPI::Unit unit)		
{ 	
	ProductionManager::Instance().onUnitDestroy(unit);
	WorkerManager::Instance().onUnitDestroy(unit);
	InformationManager::Instance().onUnitDestroy(unit);
	BuildingManager::Instance().onUnitDestroy(unit);
	DecisionMaker::Instance().getStatistics().on_unit_destroy(unit);
}

void GameCommander::onUnitMorph(BWAPI::Unit unit)		
{ 
	InformationManager::Instance().onUnitMorph(unit);
	WorkerManager::Instance().onUnitMorph(unit);
}

BWAPI::Unit GameCommander::getClosestUnitToTarget(BWAPI::UnitType type, BWAPI::Position target)
{
	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 100000;

	for (auto & unit : _validUnits)
	{
		if (unit->getType() == type)
		{
			double dist = unit->getDistance(target);
			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}

BWAPI::Unit GameCommander::getClosestWorkerToTarget(BWAPI::Position target)
{
	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 100000;

	for (auto & unit : _validUnits)
	{
		if (!isAssigned(unit) && unit->getType().isWorker() && WorkerManager::Instance().isFree(unit))
		{
			double dist = unit->getDistance(target);
			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}

void GameCommander::assignUnit(BWAPI::Unit unit, BWAPI::Unitset & set)
{
    if (_scoutUnits.contains(unit)) { _scoutUnits.erase(unit); }
    else if (_combatUnits.contains(unit)) { _combatUnits.erase(unit); }

    set.insert(unit);
}
