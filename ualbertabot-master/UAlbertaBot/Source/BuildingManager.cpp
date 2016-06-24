#include "Common.h"
#include "BuildingManager.h"
#include "Micro.h"
#include "ScoutManager.h"
#include <cmath>

#define MAX_CANNON_NUM 5

using namespace UAlbertaBot;

BuildingManager::BuildingManager()
    : _debugMode(false)
    , _reservedMinerals(0)
    , _reservedGas(0)
	, numNexus(1)
{
	maxResources = 16 * getOcupiedResourses();
	//BWAPI::Broodwar->sendText("Max resource is %d", maxResources);
}

int BuildingManager::getOcupiedResourses()
{
	int sum = 0;
	for (auto & unit : BWAPI::Broodwar->getMinerals())
	{
		if (unit->getDistance(static_cast<BWAPI::Position>(BWAPI::Broodwar->self()->getStartLocation())) < 300)
		{
			sum += unit->getResources();
		}
	}
	return sum;
}

int BuildingManager::getOcupiedResoursesNearDepot(BWAPI::Unit depot)
{
	int sum = 0;
	BWAPI::Unitset minerals = getMineralPatchesNearDepot(depot);
	for (auto mineral : minerals)
	{
		sum += mineral->getResources();
	}
	return sum;
}

// gets called every frame from GameCommander
void BuildingManager::update()
{
    validateWorkersAndBuildings();          // check to see if assigned workers have died en route or while constructing
    assignWorkersToUnassignedBuildings();   // assign workers to the unassigned buildings and label them 'planned'    
    constructAssignedBuildings();           // for each planned building, if the worker isn't constructing, send the command    
    checkForStartedConstruction();          // check to see if any buildings have started construction and update data structures    
    checkForDeadTerranBuilders();           // if we are terran and a building is under construction without a worker, assign a new one    
    checkForCompletedBuildings();           // check to see if any buildings have completed and update data structures
	checkForNewNexusConstruction();
	checkForBuildCannon();
}

bool BuildingManager::isBeingBuilt(BWAPI::UnitType type)
{
    for (auto & b : _buildings)
    {
        if (b.type == type)
        {
            return true;
        }
    }

    return false;
}

void BuildingManager::checkForNewNexusConstruction()
{

	int newNumNexus = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);

	if (newNumNexus == numNexus)
	{
		return;
	}

	numNexus = newNumNexus;

	//if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Forge) == 0)
	//{
		//addBuildingTask(BWAPI::UnitTypes::Protoss_Forge, BWAPI::Broodwar->self()->getStartLocation(), false);
	//}

	BWAPI::Unitset bases = getBases();

	for (auto base : bases)
	{
		int numPylon = getPylonNumNearDepot(base);

		//BWAPI::Broodwar->sendText("NO. of pylon is %d", numPylon);

		if (numPylon == 0)
		{
			//BWAPI::Broodwar->sendText("Desire Pylon position is %d, %d", base->getTilePosition().x, base->getTilePosition().y);
			//addBuildingTask(BWAPI::UnitTypes::Protoss_Pylon, unit->getTilePosition(), false);
			if (base != nullptr)
			{
				addBuildingTask(BWAPI::UnitTypes::Protoss_Pylon, base->getTilePosition(), false);
			}
		}
	}
}

int BuildingManager::getPylonNumNearDepot(BWAPI::Unit depot)
{
	if (!depot) { return 0; }

	int pylonNum = 0;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if ((unit->getType() == BWAPI::UnitTypes::Protoss_Pylon)&& unit->isCompleted() && unit->getDistance(depot) < 300)
		{
			pylonNum++;
		}
	}

	return pylonNum;
}

BWAPI::Unitset BuildingManager::getMineralPatchesNearDepot(BWAPI::Unit depot)
{
	// if there are minerals near the depot, add them to the set
	BWAPI::Unitset mineralsNearDepot;

	int radius = 300;

	for (auto & unit : BWAPI::Broodwar->getAllUnits())
	{
		if ((unit->getType() == BWAPI::UnitTypes::Resource_Mineral_Field) && unit->getDistance(depot) < radius)
		{
			mineralsNearDepot.insert(unit);
		}
	}

	// if we didn't find any, use the whole map
/*	if (mineralsNearDepot.empty())
	{
		for (auto & unit : BWAPI::Broodwar->getAllUnits())
		{
			if ((unit->getType() == BWAPI::UnitTypes::Resource_Mineral_Field))
			{
				mineralsNearDepot.insert(unit);
			}
		}
	}*/

	return mineralsNearDepot;
}

void BuildingManager::checkForBuildCannon()
{
	int numForge = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Forge);

	BWAPI::Unitset bases = getBases();

	for (auto base : bases)
	{
		if (cannonMap.find(base) == cannonMap.end())
		{
			cannonMap[base] = 0;
		}
	}

	int frame = BWAPI::Broodwar->getFrameCount();
	int minute = frame / (24 * 60);

	if (numForge != 0)
	{
		for (auto it : cannonMap)
		{
			int rs = getOcupiedResoursesNearDepot(it.first);
			int cannonNumShould = rs*minute*MAX_CANNON_NUM / maxResources;
			int count = cannonNumShould - it.second;
			if (count > 0 && getPylonNumNearDepot(it.first) != 0)
			{
				//BWAPI::Broodwar->sendText("We should have %d cannon, still need %d cannon with evaluation %d", cannonNumShould, count, rs*minute);
				for (int i = 0; i < count; i++)
				{
					addBuildingTask(BWAPI::UnitTypes::Protoss_Photon_Cannon, it.first->getTilePosition(), false);
				}
				cannonMap[it.first] = cannonNumShould;
			}
		}
	}
}

int BuildingManager::getBuildingNumNearDepot(BWAPI::Unit depot)
{
	if (!depot) { return 0; }

	int buildingNum = 0;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getDistance(depot) < 300)
		{
			buildingNum++;
		}
	}

	return buildingNum;
}

// STEP 1: DO BOOK KEEPING ON WORKERS WHICH MAY HAVE DIED
void BuildingManager::validateWorkersAndBuildings()
{
    // TODO: if a terran worker dies while constructing and its building
    //       is under construction, place unit back into buildingsNeedingBuilders

    std::vector<Building> toRemove;
    
    // find any buildings which have become obsolete
    for (auto & b : _buildings)
    {
        if (b.status != BuildingStatus::UnderConstruction)
        {
            continue;
        }

        if (b.buildingUnit == nullptr || !b.buildingUnit->getType().isBuilding() || b.buildingUnit->getHitPoints() <= 0)
        {
            toRemove.push_back(b);
        }
    }

    removeBuildings(toRemove);
}

// STEP 2: ASSIGN WORKERS TO BUILDINGS WITHOUT THEM
void BuildingManager::assignWorkersToUnassignedBuildings()
{
    // for each building that doesn't have a builder, assign one
    for (Building & b : _buildings)
    {
        if (b.status != BuildingStatus::Unassigned)
        {
            continue;
        }

		if (_debugMode) BWAPI::Broodwar->printf("Assigning Worker To: %s", b.type.getName().c_str());

        // grab a worker unit from WorkerManager which is closest to this final position
        BWAPI::Unit workerToAssign = WorkerManager::Instance().getBuilder(b);

        if (workerToAssign)
        {
            //BWAPI::Broodwar->printf("VALID WORKER BEING ASSIGNED: %d", workerToAssign->getID());

            // TODO: special case of terran building whose worker died mid construction
            //       send the right click command to the buildingUnit to resume construction
            //		 skip the buildingsAssigned step and push it back into buildingsUnderConstruction

            b.builderUnit = workerToAssign;
			b.status = BuildingStatus::Assigned;

            BWAPI::TilePosition testLocation = getBuildingLocation(b);

			if (b.type == BWAPI::UnitTypes::Protoss_Pylon)
			{
				//BWAPI::Broodwar->sendText("Pylon position is %d, %d", testLocation.x, testLocation.y);
			}

			//b.type.getName

			//for building localtion test

			/*
			static bool testflag;


			if (testflag == false){

				testLocation = BWAPI::Broodwar->self()->getStartLocation();

				testLocation.x += 10;
				testLocation.y += 10;

				testflag = true;
			}
			*/
			

            if (!testLocation.isValid())
            {
                continue;
            }

            b.finalPosition = testLocation;

            // reserve this building's space
            BuildingPlacer::Instance().reserveTiles(b.finalPosition,b.type.tileWidth(),b.type.tileHeight());

			//b.status = BuildingStatus::Assigned;
        }
    }
}

// STEP 3: ISSUE CONSTRUCTION ORDERS TO ASSIGN BUILDINGS AS NEEDED
void BuildingManager::constructAssignedBuildings()
{
    for (auto & b : _buildings)
    {
        if (b.status != BuildingStatus::Assigned)
        {
            continue;
        }

        // if that worker is not currently constructing
        if (!b.builderUnit->isConstructing())
        {
            // if we haven't explored the build position, go there
            if (!isBuildingPositionExplored(b))
            {
                Micro::SmartMove(b.builderUnit,BWAPI::Position(b.finalPosition));
            }
            // if this is not the first time we've sent this guy to build this
            // it must be the case that something was in the way of building
            else if (b.buildCommandGiven)
            {
                // tell worker manager the unit we had is not needed now, since we might not be able
                // to get a valid location soon enough
                WorkerManager::Instance().finishedWithWorker(b.builderUnit);

                // free the previous location in reserved
                BuildingPlacer::Instance().freeTiles(b.finalPosition,b.type.tileWidth(),b.type.tileHeight());

                // nullify its current builder unit
                b.builderUnit = nullptr;

                // reset the build command given flag
                b.buildCommandGiven = false;

                // add the building back to be assigned
                b.status = BuildingStatus::Unassigned;
            }
            else
            {
                // issue the build order!
                b.builderUnit->build(b.type,b.finalPosition);

                // set the flag to true
                b.buildCommandGiven = true;
            }
        }
    }
}

// STEP 4: UPDATE DATA STRUCTURES FOR BUILDINGS STARTING CONSTRUCTION
void BuildingManager::checkForStartedConstruction()
{
    // for each building unit which is being constructed
    for (auto & buildingStarted : BWAPI::Broodwar->self()->getUnits())
    {
        // filter out units which aren't buildings under construction
        if (!buildingStarted->getType().isBuilding() || !buildingStarted->isBeingConstructed())
        {
            continue;
        }

        // check all our building status objects to see if we have a match and if we do, update it
        for (auto & b : _buildings)
        {
            if (b.status != BuildingStatus::Assigned)
            {
                continue;
            }
        
            // check if the positions match
            if (b.finalPosition == buildingStarted->getTilePosition())
            {
                // the resources should now be spent, so unreserve them
                _reservedMinerals -= buildingStarted->getType().mineralPrice();
                _reservedGas      -= buildingStarted->getType().gasPrice();

                // flag it as started and set the buildingUnit
                b.underConstruction = true;
                b.buildingUnit = buildingStarted;

                // if we are zerg, the buildingUnit now becomes nullptr since it's destroyed
                if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg)
                {
                    b.builderUnit = nullptr;
                    // if we are protoss, give the worker back to worker manager
                }
                else if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss)
                {
                    // if this was the gas steal unit then it's the scout worker so give it back to the scout manager
                    if (b.isGasSteal)
                    {
                        ScoutManager::Instance().setWorkerScout(b.builderUnit);
                    }
                    // otherwise tell the worker manager we're finished with this unit
                    else
                    {
                        WorkerManager::Instance().finishedWithWorker(b.builderUnit);
                    }

                    b.builderUnit = nullptr;
                }

                // put it in the under construction vector
                b.status = BuildingStatus::UnderConstruction;

                // free this space
                BuildingPlacer::Instance().freeTiles(b.finalPosition,b.type.tileWidth(),b.type.tileHeight());

                // only one building will match
                break;
            }
        }
    }
}

// STEP 5: IF WE ARE TERRAN, THIS MATTERS, SO: LOL
void BuildingManager::checkForDeadTerranBuilders() {}

// STEP 6: CHECK FOR COMPLETED BUILDINGS
void BuildingManager::checkForCompletedBuildings()
{
    std::vector<Building> toRemove;

    // for each of our buildings under construction
    for (auto & b : _buildings)
    {
        if (b.status != BuildingStatus::UnderConstruction)
        {
            continue;       
        }

        // if the unit has completed
        if (b.buildingUnit->isCompleted())
        {
            // if we are terran, give the worker back to worker manager
            if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran)
            {
                if (b.isGasSteal)
                {
                    ScoutManager::Instance().setWorkerScout(b.builderUnit);
                }
                // otherwise tell the worker manager we're finished with this unit
                else
                {
                    WorkerManager::Instance().finishedWithWorker(b.builderUnit);
                }
            }

            // remove this unit from the under construction vector
            toRemove.push_back(b);
        }
    }

    removeBuildings(toRemove);
}

// COMPLETED
bool BuildingManager::isEvolvedBuilding(BWAPI::UnitType type) 
{
    if (type == BWAPI::UnitTypes::Zerg_Sunken_Colony ||
        type == BWAPI::UnitTypes::Zerg_Spore_Colony ||
        type == BWAPI::UnitTypes::Zerg_Lair ||
        type == BWAPI::UnitTypes::Zerg_Hive ||
        type == BWAPI::UnitTypes::Zerg_Greater_Spire)
    {
        return true;
    }

    return false;
}

// add a new building to be constructed
void BuildingManager::addBuildingTask(BWAPI::UnitType type, BWAPI::TilePosition desiredLocation, bool isGasSteal)
{
    _reservedMinerals += type.mineralPrice();
    _reservedGas	     += type.gasPrice();

    Building b(type, desiredLocation);
    b.isGasSteal = isGasSteal;
    b.status = BuildingStatus::Unassigned;

    _buildings.push_back(b);
}

bool BuildingManager::isBuildingPositionExplored(const Building & b) const
{
    BWAPI::TilePosition tile = b.finalPosition;

    // for each tile where the building will be built
    for (int x=0; x<b.type.tileWidth(); ++x)
    {
        for (int y=0; y<b.type.tileHeight(); ++y)
        {
            if (!BWAPI::Broodwar->isExplored(tile.x + x,tile.y + y))
            {
                return false;
            }
        }
    }

    return true;
}


char BuildingManager::getBuildingWorkerCode(const Building & b) const
{
    return b.builderUnit == nullptr ? 'X' : 'W';
}

int BuildingManager::getReservedMinerals() 
{
    return _reservedMinerals;
}

int BuildingManager::getReservedGas() 
{
    return _reservedGas;
}

void BuildingManager::drawBuildingInformation(int x,int y)
{
    if (!Config::Debug::DrawBuildingInfo)
    {
        return;
    }

    for (auto & unit : BWAPI::Broodwar->self()->getUnits())
    {
        BWAPI::Broodwar->drawTextMap(unit->getPosition().x,unit->getPosition().y+5,"\x07%d",unit->getID());
    }

    BWAPI::Broodwar->drawTextScreen(x,y,"\x04 Building Information:");
    BWAPI::Broodwar->drawTextScreen(x,y+20,"\x04 Name");
    BWAPI::Broodwar->drawTextScreen(x+150,y+20,"\x04 State");

    int yspace = 0;

    for (const auto & b : _buildings)
    {
        if (b.status == BuildingStatus::Unassigned)
        {
            BWAPI::Broodwar->drawTextScreen(x,y+40+((yspace)*10),"\x03 %s",b.type.getName().c_str());
            BWAPI::Broodwar->drawTextScreen(x+150,y+40+((yspace++)*10),"\x03 Need %c",getBuildingWorkerCode(b));
        }
        else if (b.status == BuildingStatus::Assigned)
        {
            BWAPI::Broodwar->drawTextScreen(x,y+40+((yspace)*10),"\x03 %s %d",b.type.getName().c_str(),b.builderUnit->getID());
            BWAPI::Broodwar->drawTextScreen(x+150,y+40+((yspace++)*10),"\x03 A %c (%d,%d)",getBuildingWorkerCode(b),b.finalPosition.x,b.finalPosition.y);

            int x1 = b.finalPosition.x*32;
            int y1 = b.finalPosition.y*32;
            int x2 = (b.finalPosition.x + b.type.tileWidth())*32;
            int y2 = (b.finalPosition.y + b.type.tileHeight())*32;

            BWAPI::Broodwar->drawLineMap(b.builderUnit->getPosition().x,b.builderUnit->getPosition().y,(x1+x2)/2,(y1+y2)/2,BWAPI::Colors::Orange);
            BWAPI::Broodwar->drawBoxMap(x1,y1,x2,y2,BWAPI::Colors::Red,false);
        }
        else if (b.status == BuildingStatus::UnderConstruction)
        {
            BWAPI::Broodwar->drawTextScreen(x,y+40+((yspace)*10),"\x03 %s %d",b.type.getName().c_str(),b.buildingUnit->getID());
            BWAPI::Broodwar->drawTextScreen(x+150,y+40+((yspace++)*10),"\x03 Const %c",getBuildingWorkerCode(b));
        }
    }
}

BuildingManager & BuildingManager::Instance()
{
    static BuildingManager instance;
    return instance;
}

std::vector<BWAPI::UnitType> BuildingManager::buildingsQueued()
{
    std::vector<BWAPI::UnitType> buildingsQueued;

    for (const auto & b : _buildings)
    {
        if (b.status == BuildingStatus::Unassigned || b.status == BuildingStatus::Assigned)
        {
            buildingsQueued.push_back(b.type);
        }
    }

    return buildingsQueued;
}

BWAPI::TilePosition getBuildingLocationNearDepot(const Building & b, BWAPI::Unit depot)
{
	int numPylons = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Pylon);

	if (b.type.requiresPsi() && numPylons == 0)
	{
		return BWAPI::TilePositions::None;
	}

	if (b.type.isRefinery())
	{
		return BuildingPlacer::Instance().getRefineryPosition();
	}

	if (b.type.isResourceDepot())
	{
		// get the location 
		BWAPI::TilePosition tile = MapTools::Instance().getNextExpansion();

		return tile;
	}

	// set the building padding specifically
	int distance = b.type == BWAPI::UnitTypes::Protoss_Photon_Cannon ? 0 : Config::Macro::BuildingSpacing;
	if (b.type == BWAPI::UnitTypes::Protoss_Pylon && (numPylons < 3))
	{
		distance = Config::Macro::PylonSpacing;
	}

	// get a position within our region
	return BuildingPlacer::Instance().getBuildLocationNear(b, distance, false);
}

BWAPI::TilePosition BuildingManager::getBuildingLocation(const Building & b)
{
	int numPylons = 0;
	if (b.desiredPosition == BWAPI::Broodwar->self()->getStartLocation())
	{
		numPylons = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Pylon);
	}
	else
	{
		numPylons = getPylonNumNearPosition(b.desiredPosition);
	}

    if (b.isGasSteal)
    {
        BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
        UAB_ASSERT(enemyBaseLocation,"Should have enemy base location before attempting gas steal");
        UAB_ASSERT(enemyBaseLocation->getGeysers().size() > 0,"Should have spotted an enemy geyser");

        for (auto & unit : enemyBaseLocation->getGeysers())
        {
            BWAPI::TilePosition tp(unit->getInitialTilePosition());
            return tp;
        }
    }

    if (b.type.requiresPsi() && numPylons == 0)
    {
        return BWAPI::TilePositions::None;
    }

    if (b.type.isRefinery())
    {
        return BuildingPlacer::Instance().getRefineryPosition();
    }

    if (b.type.isResourceDepot())
    {
        // get the location 
        BWAPI::TilePosition tile = MapTools::Instance().getNextExpansion();

        return tile;
    }

    // set the building padding specifically
    int distance = b.type == BWAPI::UnitTypes::Protoss_Photon_Cannon ? 0 : Config::Macro::BuildingSpacing;
    if (b.type == BWAPI::UnitTypes::Protoss_Pylon && (numPylons < 3))
    {
        distance = Config::Macro::PylonSpacing;
    }

    // get a position within our region
    return BuildingPlacer::Instance().getBuildLocationNear(b, distance,false);
}

int BuildingManager::getPylonNumNearPosition(BWAPI::TilePosition position)
{
	int pylonNum = 0;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if ((unit->getType() == BWAPI::UnitTypes::Protoss_Pylon) && unit->isCompleted())
		{
			double distance = sqrt(pow(unit->getTilePosition().x-position.x, 2) + pow(unit->getTilePosition().y-position.y, 2));

			if (distance < 300) {
				// BWAPI::Broodwar->sendText("Near Pylon distance is %f", distance);
				pylonNum++;
			}
		}
	}

	return pylonNum;
}

void BuildingManager::removeBuildings(const std::vector<Building> & toRemove)
{
    for (auto & b : toRemove)
    {
        auto & it = std::find(_buildings.begin(), _buildings.end(), b);

        if (it != _buildings.end())
        {
            _buildings.erase(it);
        }
    }
}

void BuildingManager::onUnitDestroy(BWAPI::Unit unit)
{
	BWAPI::Unitset bases = getBases();
	if (unit != nullptr && unit->getType() == BWAPI::UnitTypes::Protoss_Pylon && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		BWAPI::Unit nearestBase = nullptr;
		int distance = 1000000;
		for (auto base : bases)
		{
			int d = unit->getDistance(base);
			if (d < distance)
			{
				distance = d;
				nearestBase = base;
			}
		}
		if (nearestBase != nullptr)
		{
			addBuildingTask(BWAPI::UnitTypes::Protoss_Pylon, nearestBase->getTilePosition(), false);
		}
	}

	if (unit != nullptr && unit->getType() == BWAPI::UnitTypes::Protoss_Photon_Cannon && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		BWAPI::Unit nearestBase = nullptr;
		int distance = 1000000;
		for (auto base : bases)
		{
			int d = unit->getDistance(base);
			if (d < distance)
			{
				distance = d;
				nearestBase = base;
			}
		}
		if (nearestBase != nullptr)
		{
			cannonMap[nearestBase] = cannonMap[nearestBase] - 1;
		}
	}

	if (unit != nullptr && unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		cannonMap.erase(unit);
	}
}

BWAPI::Unitset BuildingManager::getBases()
{
	BWAPI::Unitset bases;
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit != nullptr && unit->getType().isResourceDepot() && unit->isCompleted())
		{
			bases.insert(unit);
		}
	}
	return bases;
}