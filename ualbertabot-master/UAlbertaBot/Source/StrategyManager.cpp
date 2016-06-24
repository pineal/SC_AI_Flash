#include "Common.h"
#include "StrategyManager.h"
#include "DecisionMaker.h"
#include "UnitUtil.h"
#include "BWAPI.h"

using namespace BWAPI;
using namespace UAlbertaBot;
using namespace Filter;
using namespace UnitTypes;
// constructor
StrategyManager::StrategyManager() 
	: _selfRace(BWAPI::Broodwar->self()->getRace())
	, _enemyRace(BWAPI::Broodwar->enemy()->getRace())
    , _emptyBuildOrder(BWAPI::Broodwar->self()->getRace())
{
	
}

// get an instance of this
StrategyManager & StrategyManager::Instance() 
{
	static StrategyManager instance;
	return instance;
}

const int StrategyManager::getScore(BWAPI::Player player) const
{
	return player->getBuildingScore() + player->getKillScore() + player->getRazingScore() + player->getUnitScore();
}

const BuildOrder & StrategyManager::getOpeningBookBuildOrder() const
{
    auto buildOrderIt = _strategies.find(Config::Strategy::StrategyName);

    // look for the build order in the build order map
	if (buildOrderIt != std::end(_strategies))
    {
        return (*buildOrderIt).second._buildOrder;
    }
    else
    {
        UAB_ASSERT_WARNING(false, "Strategy not found: %s, returning empty initial build order", Config::Strategy::StrategyName.c_str());
        return _emptyBuildOrder;
    }
}

const bool StrategyManager::shouldExpandNow() const
{
	// if there is no place to expand to, we can't expand
	if (MapTools::Instance().getNextExpansion() == BWAPI::TilePositions::None)
	{
        //BWAPI::Broodwar->printf("No valid expansion location");
		return false;
	}

	size_t numDepots    = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Command_Center)
                        + UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Nexus)
                        + UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Hatchery)
                        + UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Lair)
                        + UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Hive);
	int frame           = BWAPI::Broodwar->getFrameCount();
    int minute          = frame / (24*60);

	// if we have a ton of idle workers then we need a new expansion
	if (WorkerManager::Instance().getNumIdleWorkers() > 10)
	{
		return true;
	}

    // if we have a ridiculous stockpile of minerals, expand
    if (BWAPI::Broodwar->self()->minerals() > 3000)
    {
        return true;
    }

    // we will make expansion N after array[N] minutes have passed
    std::vector<int> expansionTimes = {5, 10, 20, 30, 40 , 50};

    for (size_t i(0); i < expansionTimes.size(); ++i)
    {
        if (numDepots < (i+2) && minute > expansionTimes[i])
        {
            return true;
        }
    }

	return false;
}

void StrategyManager::addStrategy(const std::string & name, Strategy & strategy)
{
    _strategies[name] = strategy;
}

const MetaPairVector StrategyManager::getBuildOrderGoal()
{
    BWAPI::Race myRace = BWAPI::Broodwar->self()->getRace();

    if (myRace == BWAPI::Races::Protoss)
    {
        return getProtossBuildOrderGoal();
    }
    else if (myRace == BWAPI::Races::Terran)
	{
		return getTerranBuildOrderGoal();
	}
    else if (myRace == BWAPI::Races::Zerg)
	{
		return getZergBuildOrderGoal();
	}

    return MetaPairVector();
}

bool flag = true;

static bool rangeFlag = false;
static bool legFlag = false;
static bool capacityFlag = false;

const MetaPairVector StrategyManager::getProtossBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;
	
	int numZealots          = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
    int numPylons           = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Pylon);
	int numDragoons         = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes           = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted   = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll         = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber            = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon           = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);
    int numScout            = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Corsair);
    int numReaver           = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Reaver);
    int numDarkTeplar       = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar);
	int numHighTemplar		= UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_High_Templar);
	int numStargate			= UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Stargate);
	int numCarrier			= UnitUtil::GetAllUnitCount(Protoss_Carrier);

	int population = numPylons * 8 + numNexusCompleted * 9;


	//
	//we can set different strategy for early, mid, late game
	//
	//if some tech is a good choice, then we develope that tech
	//
	if (flag)
	{

		//goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Pylon, 2));
		//goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe, numProbes + 15));
		//flag = false;
	}

	//default rules
	//whether the number should be determined in DecisionMaker or not?
	//

	//Broodwar->sendText("%s", "needWorker?");
	if ( DecisionMaker::Instance().needWorker() ){
		
		//getDepotNum
		//Broodwar->sendText("%s", "needWorker");
		goal.push_back( MetaPair(BWAPI::UnitTypes::Protoss_Probe, numProbes + BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999, IsResourceDepot && IsOwned && IsCompleted).size() ) );

	}

	bool armyNeedPylons = false;

	if (DecisionMaker::Instance().needArmy()){ //bug  always cancel units, don't build pylons

		//if we are at the early game train more zealots
		//if enemy has a lot of floating units, train more dragoons
		//if we are at the mid game train more dragoons
		//if we are at the late game train more helpful units 
		

		//if floating units is detected, train dragoons

		int hightempNum = Broodwar->self()->completedUnitCount(Protoss_High_Templar);

		

		/*
		if (hightempNum >= 2){// train archon    need to fix

			int numArchon = Broodwar->self()->allUnitCount(Protoss_Archon);

			goal.push_back(MetaPair(Protoss_Archon, numArchon+1) ) ;


			if (population  <= 4+ DecisionMaker::Instance().stat.get_population())
				goal.push_back(MetaPair(Protoss_Pylon, numPylons + 1) );

			
		}
		*/


		if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Pylon) == 0)
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Pylon, numPylons+ 1));

		int supplyTotal = numPylons * 8 + numNexusCompleted * 9;

		//and not block the supply 
		if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Gateway) == 0 && BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Pylon) > 0)//???
		{
			//Broodwar->sendText("%s", "Gateway pushed");
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway, 1));
		}
		

		int idleGateway = 0;

		Unitset myUnits = BWAPI::Broodwar->self()->getUnits();

		for (auto &u : myUnits){

			if (u->getType() == BWAPI::UnitTypes::Protoss_Gateway && u->isIdle())
				idleGateway++;
		}


		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Gateway) > 0 && idleGateway>0){

			int coreNum = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);

			if (coreNum==0)
				goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, numZealots+1 ));//should be changed
			else{

				//int randNum = (rand() * 11)% 2;
				//Broodwar->sendText("%d", randNum);

				//int tempNum = DecisionMaker::Instance().stat.get_num_comp( Protoss_Templar_Archives );
				int tempNum = Broodwar->self()->completedUnitCount( Protoss_Templar_Archives);

				
				/*
				if (tempNum > 0 && DecisionMaker::Instance().needHighTemplar()){
					goal.push_back(MetaPair(Protoss_High_Templar, numHighTemplar + 1));
					Broodwar->sendText("%s %d %d", "need high templar", tempNum, DecisionMaker::Instance().needHighTemplar() );

				} else*/
				if (Evaluation::Instance().eval_soldier(UnitTypes::Protoss_Zealot) > Evaluation::Instance().eval_soldier(UnitTypes::Protoss_Dragoon))
					goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, numZealots + 1));//should be changed
				else
					goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon, numDragoons + 1));//should be changed
				
				if (population <= 2 + DecisionMaker::Instance().stat.get_population())
					armyNeedPylons = true;
				//goal.push_back(MetaPair(Protoss_Pylon, numPylons + 1));
			
			}

		}
	}



	if (DecisionMaker::Instance().stat.get_num_comp(UnitTypes::Protoss_Dragoon) > 0  ){

		if (DecisionMaker::Instance().needUpgradeRange() && rangeFlag==false){

			int coreNum = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
			if (coreNum==0)
				goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));//should be changed
			else {

				int idleCore = 0;
				Unitset myUnits = BWAPI::Broodwar->self()->getUnits();
				for (auto &u : myUnits){

					if (u->getType() == BWAPI::UnitTypes::Protoss_Cybernetics_Core && u->isIdle())
						idleCore++;
				}

				if (DecisionMaker::Instance().stat.get_num_comp(Protoss_Cybernetics_Core) > 0 && rangeFlag == false && idleCore>0){
					goal.push_back(MetaPair(UpgradeTypes::Singularity_Charge, 1));
					rangeFlag = true;
				}

			}
		}
	}

	if (Broodwar->self()->completedUnitCount(Protoss_Citadel_of_Adun) > 0 && legFlag==false){

		if (DecisionMaker::Instance().needZealotSpeed()){

			goal.push_back(MetaPair(UpgradeTypes::Leg_Enhancements, 1));
			legFlag = true;
		}

	}


	//////for carrier

	

	if (DecisionMaker::Instance().needStargate())
		goal.push_back(MetaPair(Protoss_Stargate, numStargate+1));

	if (DecisionMaker::Instance().needCarrier() || DecisionMaker::Instance().needCorsair()){

		if (numStargate==0)
			goal.push_back(MetaPair(Protoss_Stargate, numStargate + 1));

		if (Broodwar->self()->completedUnitCount(Protoss_Stargate) > 0){

			int numFleet = Broodwar->self()->allUnitCount(Protoss_Fleet_Beacon);

			if (numFleet == 0)
				goal.push_back(MetaPair(Protoss_Fleet_Beacon,1));

			if (Broodwar->self()->completedUnitCount(Protoss_Fleet_Beacon) > 0){

				int idleStargate = 0;



				Unitset myUnits = BWAPI::Broodwar->self()->getUnits();

				for (auto &u : myUnits){

					if (u->getType() == BWAPI::UnitTypes::Protoss_Stargate && u->isIdle())
						idleStargate++;
				}

				if (idleStargate > 0){//train army

					goal.push_back(MetaPair(Protoss_Carrier, numCarrier + 1));

					if (population <= 8 + DecisionMaker::Instance().stat.get_population())
						armyNeedPylons = true;

				}

			}
		}
	}
	
	if (Broodwar->self()->completedUnitCount(Protoss_Carrier) > 0)
	{
		//Broodwar->sendText("%s", "adding interceptor");
		//goal.push_back(MetaPair(Protoss_Interceptor, Broodwar->self()->completedUnitCount(Protoss_Carrier) * 8));
	}
	
	//end for training carrier

	//start upgrade for carrier

	if (DecisionMaker::Instance().needCapacity() && capacityFlag==false){//can be more robust
		
		//Broodwar->sendText("%s", "upgrading for capacity");
		

		goal.push_back( MetaPair(UpgradeTypes::Carrier_Capacity,1) );
		capacityFlag = true;
	}

	if (DecisionMaker::Instance().needAirUpgrade()){
		
		int currentLevel = Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Air_Weapons);


		goal.push_back(MetaPair(UpgradeTypes::Protoss_Air_Weapons, currentLevel+1));
	}



	if (DecisionMaker::Instance().needCorsair()){
		goal.push_back(MetaPair(UnitTypes::Protoss_Corsair, 1));
	}


	/////end for carrier


	if (DecisionMaker::Instance().needTemplar()){

		int coreNum = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
		int adunNum = Broodwar->self()->allUnitCount( Protoss_Citadel_of_Adun );
		int tempNum = Broodwar->self()->allUnitCount(Protoss_Templar_Archives);

		if (coreNum == 0)
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));//should be changed
		else if (adunNum == 0)
			goal.push_back(MetaPair(Protoss_Citadel_of_Adun, 1));//need to fix
		/*
		else if (tempNum == 0)
			goal.push_back(MetaPair(Protoss_Templar_Archives,1));
		*/

	}

	if (DecisionMaker::Instance().needObserver()){

		int numFacility = Broodwar->self()->allUnitCount(Protoss_Robotics_Facility);

		if (numFacility==0)
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));


		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}


		int idleFacility = 0;

		Unitset myUnits = BWAPI::Broodwar->self()->getUnits();

		for (auto &u : myUnits){

			if (u->getType() == BWAPI::UnitTypes::Protoss_Robotics_Facility && u->isIdle())
				idleFacility++;
		}


		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0 && idleFacility>0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}

	}



	if (DecisionMaker::Instance().needSupply() || armyNeedPylons){

		//Broodwar->sendText("%s %d", "adding pylon goal", std::max(1, (std::min(int(DecisionMaker::Instance().stat.get_mineral_speed(60) / 7), 3)) ) ) ;

		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Pylon, numPylons + std::max( 1, (std::min( int(DecisionMaker::Instance().stat.get_mineral_speed(60)/7) , 3)) )   ) );
		//

	}

	if (DecisionMaker::Instance().needGateway()){

		//prerequisite
		if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Pylon) == 0)
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Pylon, numPylons + 1));
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Pylon) > 0){
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway, BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Gateway) + 1));
			//Broodwar->sendText("%s", "Gateway pushed");
		}
	}

	if (DecisionMaker::Instance().needDragoon()){

		//prerequisite
		

		int gatewayNum = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Gateway);
		int coreNum = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);

		if(BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Pylon) == 0)
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Pylon, numPylons + 1));
		else if (gatewayNum == 0)
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway,  1));
		else if (coreNum ==0 )
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));

	}

	if (DecisionMaker::Instance().needGas()){
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Assimilator, BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Assimilator) + 1));
	}

	if (DecisionMaker::Instance().shoudExpand()){//should be redesigned
		//Broodwar->sendText("%d", DecisionMaker::Instance().stat.get_num_comp(Protoss_Nexus) + 1);
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, DecisionMaker::Instance().stat.get_num_comp(Protoss_Nexus) + 1));
	}

	/*
    if (Config::Strategy::StrategyName == "Protoss_ZealotRush")
    {
        goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, numZealots + 8));

        // once we have a 2nd nexus start making dragoons
        if (numNexusAll >= 2)
        {
            goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon, numDragoons + 4));
        }
    }
    else if (Config::Strategy::StrategyName == "Protoss_DragoonRush")
    {
        goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon, numDragoons + 6));
    }
    else if (Config::Strategy::StrategyName == "Protoss_Drop")
    {
        if (numZealots == 0)
        {
            goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, numZealots + 4));
            goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Shuttle, 1));
        }
        else
        {
            goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot, numZealots + 8));
        }
    }
    else if (Config::Strategy::StrategyName == "Protoss_DTRush")
    {
        goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dark_Templar, numDarkTeplar + 2));

        // if we have a 2nd nexus then get some goons out
        if (numNexusAll >= 2)
        {
            goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon, numDragoons + 4));
        }
    }
    else
    {
        UAB_ASSERT_WARNING(false, "Unknown Protoss Strategy Name: %s", Config::Strategy::StrategyName.c_str());
    }

    // if we have 3 nexus, make an observer
    if (numNexusCompleted >= 3)
    {
        goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
    }
    
    // add observer to the goal if the enemy has cloaked units
	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
		
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

    // if we want to expand, insert a nexus into the build order
	if (shouldExpandNow())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}
	*/
	return goal;
}

const MetaPairVector StrategyManager::getTerranBuildOrderGoal() const
{
	// the goal to return
	std::vector<MetaPair> goal;

    int numWorkers      = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_SCV);
    int numCC           = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Command_Center);            
    int numMarines      = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Marine);
	int numMedics       = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Medic);
	int numWraith       = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Wraith);
    int numVultures     = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Vulture);
    int numGoliath      = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Goliath);
    int numTanks        = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode)
                        + UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode);
    int numBay          = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Terran_Engineering_Bay);

    if (Config::Strategy::StrategyName == "Terran_MarineRush")
    {
	    goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine, numMarines + 8));

        if (numMarines > 5)
        {
            goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Engineering_Bay, 1));
        }
    }
    else if (Config::Strategy::StrategyName == "Terran_4RaxMarines")
    {
	    goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine, numMarines + 8));
    }
    else if (Config::Strategy::StrategyName == "Terran_VultureRush")
    {
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Vulture, numVultures + 8));

        if (numVultures > 8)
        {
            goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Tank_Siege_Mode, 1));
            goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, 4));
        }
    }
    else if (Config::Strategy::StrategyName == "Terran_TankPush")
    {
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, 6));
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Goliath, numGoliath + 6));
        goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Tank_Siege_Mode, 1));
    }
    else
    {
        BWAPI::Broodwar->printf("Warning: No build order goal for Terran Strategy: %s", Config::Strategy::StrategyName.c_str());
    }



    if (shouldExpandNow())
    {
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Command_Center, numCC + 1));
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_SCV, numWorkers + 10));
    }

	return goal;
}

const MetaPairVector StrategyManager::getZergBuildOrderGoal() const
{
	// the goal to return
	std::vector<MetaPair> goal;
	
    int numWorkers      = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Drone);
    int numCC           = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Hatchery)
                        + UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Lair)
                        + UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Hive);
	int numMutas        = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Mutalisk);
    int numDrones       = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Drone);
    int zerglings       = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Zergling);
	int numHydras       = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Hydralisk);
    int numScourge      = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Scourge);
    int numGuardians    = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Guardian);

	int mutasWanted = numMutas + 6;
	int hydrasWanted = numHydras + 6;

    if (Config::Strategy::StrategyName == "Zerg_ZerglingRush")
    {
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Zergling, zerglings + 6));
    }
    else if (Config::Strategy::StrategyName == "Zerg_2HatchHydra")
    {
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Hydralisk, numHydras + 8));
        goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Grooved_Spines, 1));
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Drone, numDrones + 4));
    }
    else if (Config::Strategy::StrategyName == "Zerg_3HatchMuta")
    {
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Hydralisk, numHydras + 12));
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Drone, numDrones + 4));
    }
    else if (Config::Strategy::StrategyName == "Zerg_3HatchScourge")
    {
        if (numScourge > 40)
        {
            goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Hydralisk, numHydras + 12));
        }
        else
        {
            goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Scourge, numScourge + 12));
        }

        
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Drone, numDrones + 4));
    }

    if (shouldExpandNow())
    {
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Hatchery, numCC + 1));
        goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Drone, numWorkers + 10));
    }

	return goal;
}

void StrategyManager::readResults()
{
    if (!Config::Modules::UsingStrategyIO)
    {
        return;
    }

    std::string enemyName = BWAPI::Broodwar->enemy()->getName();
    std::replace(enemyName.begin(), enemyName.end(), ' ', '_');

    std::string enemyResultsFile = Config::Strategy::ReadDir + enemyName + ".txt";
    
    std::string strategyName;
    int wins = 0;
    int losses = 0;

    FILE *file = fopen ( enemyResultsFile.c_str(), "r" );
    if ( file != nullptr )
    {
        char line [ 4096 ]; /* or other suitable maximum line size */
        while ( fgets ( line, sizeof line, file ) != nullptr ) /* read a line */
        {
            std::stringstream ss(line);

            ss >> strategyName;
            ss >> wins;
            ss >> losses;

            //BWAPI::Broodwar->printf("Results Found: %s %d %d", strategyName.c_str(), wins, losses);

            if (_strategies.find(strategyName) == _strategies.end())
            {
                //BWAPI::Broodwar->printf("Warning: Results file has unknown Strategy: %s", strategyName.c_str());
            }
            else
            {
                _strategies[strategyName]._wins = wins;
                _strategies[strategyName]._losses = losses;
            }
        }

        fclose ( file );
    }
    else
    {
        //BWAPI::Broodwar->printf("No results file found: %s", enemyResultsFile.c_str());
    }
}

void StrategyManager::writeResults()
{
    if (!Config::Modules::UsingStrategyIO)
    {
        return;
    }

    std::string enemyName = BWAPI::Broodwar->enemy()->getName();
    std::replace(enemyName.begin(), enemyName.end(), ' ', '_');

    std::string enemyResultsFile = Config::Strategy::WriteDir + enemyName + ".txt";

    std::stringstream ss;

    for (auto & kv : _strategies)
    {
        const Strategy & strategy = kv.second;

        ss << strategy._name << " " << strategy._wins << " " << strategy._losses << "\n";
    }

    Logger::LogOverwriteToFile(enemyResultsFile, ss.str());
}

void StrategyManager::onEnd(const bool isWinner)
{
    if (!Config::Modules::UsingStrategyIO)
    {
        return;
    }

    if (isWinner)
    {
        _strategies[Config::Strategy::StrategyName]._wins++;
    }
    else
    {
        _strategies[Config::Strategy::StrategyName]._losses++;
    }

    writeResults();
}

void StrategyManager::setLearnedStrategy()
{
    // we are currently not using this functionality for the competition so turn it off 
    return;

    if (!Config::Modules::UsingStrategyIO)
    {
        return;
    }

    const std::string & strategyName = Config::Strategy::StrategyName;
    Strategy & currentStrategy = _strategies[strategyName];

    int totalGamesPlayed = 0;
    int strategyGamesPlayed = currentStrategy._wins + currentStrategy._losses;
    double winRate = strategyGamesPlayed > 0 ? currentStrategy._wins / static_cast<double>(strategyGamesPlayed) : 0;

    // if we are using an enemy specific strategy
    if (Config::Strategy::FoundEnemySpecificStrategy)
    {        
        return;
    }

    // if our win rate with the current strategy is super high don't explore at all
    // also we're pretty confident in our base strategies so don't change if insufficient games have been played
    if (strategyGamesPlayed < 5 || (strategyGamesPlayed > 0 && winRate > 0.49))
    {
        BWAPI::Broodwar->printf("Still using default strategy");
        return;
    }

    // get the total number of games played so far with this race
    for (auto & kv : _strategies)
    {
        Strategy & strategy = kv.second;
        if (strategy._race == BWAPI::Broodwar->self()->getRace())
        {
            totalGamesPlayed += strategy._wins + strategy._losses;
        }
    }

    // calculate the UCB value and store the highest
    double C = 0.5;
    std::string bestUCBStrategy;
    double bestUCBStrategyVal = std::numeric_limits<double>::lowest();
    for (auto & kv : _strategies)
    {
        Strategy & strategy = kv.second;
        if (strategy._race != BWAPI::Broodwar->self()->getRace())
        {
            continue;
        }

        int sGamesPlayed = strategy._wins + strategy._losses;
        double sWinRate = sGamesPlayed > 0 ? currentStrategy._wins / static_cast<double>(strategyGamesPlayed) : 0;
        double ucbVal = C * sqrt( log( (double)totalGamesPlayed / sGamesPlayed ) );
        double val = sWinRate + ucbVal;

        if (val > bestUCBStrategyVal)
        {
            bestUCBStrategy = strategy._name;
            bestUCBStrategyVal = val;
        }
    }

    Config::Strategy::StrategyName = bestUCBStrategy;
}