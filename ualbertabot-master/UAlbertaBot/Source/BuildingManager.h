#pragma once

#include <Common.h>
#include "WorkerManager.h"
#include "BuildingPlacer.h"
#include "InformationManager.h"
#include "MapTools.h"

namespace UAlbertaBot
{
class BuildingManager
{
    BuildingManager();

    std::vector<Building> _buildings;
	std::map<BWAPI::Unit, int> cannonMap;
    bool            _debugMode;
    int             _reservedMinerals;				// minerals reserved for planned buildings
    int             _reservedGas;					// gas reserved for planned buildings
	int				numNexus;
	int				maxResources;

    bool            isEvolvedBuilding(BWAPI::UnitType type);
    bool            isBuildingPositionExplored(const Building & b) const;
    void            removeBuildings(const std::vector<Building> & toRemove);

    void            validateWorkersAndBuildings();		    // STEP 1
    void            assignWorkersToUnassignedBuildings();	// STEP 2
    void            constructAssignedBuildings();			// STEP 3
    void            checkForStartedConstruction();			// STEP 4
    void            checkForDeadTerranBuilders();			// STEP 5
    void            checkForCompletedBuildings();			// STEP 6
	void			checkForNewNexusConstruction();
	void			checkForBuildCannon();
    char            getBuildingWorkerCode(const Building & b) const;
    
	int				getPylonNumNearDepot(BWAPI::Unit depot);
	int				getPylonNumNearPosition(BWAPI::TilePosition position);
	int				getBuildingNumNearDepot(BWAPI::Unit depot);
	int				getOcupiedResourses();
	int				getOcupiedResoursesNearDepot(BWAPI::Unit depot);
	BWAPI::Unitset  getMineralPatchesNearDepot(BWAPI::Unit depot);
	BWAPI::Unitset	getBases();
public:
    
    static BuildingManager &	Instance();

    void                update();
    void                onUnitMorph(BWAPI::Unit unit);
    void                onUnitDestroy(BWAPI::Unit unit);
    void                addBuildingTask(BWAPI::UnitType type,BWAPI::TilePosition desiredLocation,bool isGasSteal);
    void                drawBuildingInformation(int x,int y);
	BWAPI::TilePosition getBuildingLocation(const Building & b);
	BWAPI::TilePosition getBuildingLocationNearDepot(const Building & b, BWAPI::Unit depot);

    int                getReservedMinerals();
    int                 getReservedGas();

    bool                isBeingBuilt(BWAPI::UnitType type);

    std::vector<BWAPI::UnitType> buildingsQueued();
};
}