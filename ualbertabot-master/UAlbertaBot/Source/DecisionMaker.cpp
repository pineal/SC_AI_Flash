#include "DecisionMaker.h"
#include "Common.h"
#include "UnitUtil.h"
#include "InformationManager.h"
#include "Statistics.h"
#include "Evaluation.h"
#include <cmath>

using namespace UAlbertaBot;
using namespace BWAPI;
using namespace Filter;
using namespace UnitTypes;

DecisionMaker::DecisionMaker() : hasHydralisk(false){
	//update();
	stat = Statistics();
	//stat.draw_info();
}

DecisionMaker & DecisionMaker::Instance(){
	static DecisionMaker instance;

	return instance;
}

Statistics & DecisionMaker::getStatistics(){
	return stat;
}


void DecisionMaker::update(){
	numZealots = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	numPylons = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Pylon);
	numDragoons = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	numProbes = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	numNexusCompleted = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	numNexusAll = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	numCyber = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	numCannon = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	numScout = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Corsair);
	numReaver = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Reaver);
	numDarkTeplar = UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar);
}


bool DecisionMaker::needWorker(){

	update();

	//if we can defence current enemy with ease, then train worker
	//
	//find owned minerals n, set targetworker as  max cur , 2*n+2
	//control generating speed, 3 worker per time
	//
	//
	//getWorkerNumber <- numProbes

	//Broodwar->sendText("%s,,,%d", "needworker1?",numProbes);

	//int ownedMinerals = Statistics::get_cur_mineral ;
	int getOwnedMinerals = stat.get_ctrl_mineral().first;
	//Broodwar->sendText("%d %d", stat.get_ctrl_mineral().first, stat.get_ctrl_mineral().second);

	int idleBaseNum = Broodwar->getUnitsInRadius(0, 0, 999999, IsResourceDepot && IsOwned && IsIdle).size();

	//Broodwar->sendText("")

	if (numProbes >= getOwnedMinerals * 2 + 4 || idleBaseNum ==0)
		return false;

	int getMineralRate = 15,lowRate=6;

	//Broodwar->sendText("%s", "needworker2?");

	if (needArmy() && getMineralRate <= lowRate )
		return false;

	//Broodwar->sendText("%s", "needworker3?");

	return true;
}

double calculateArmyFactor(int num){

	return num;
}

bool DecisionMaker::needArmy(){//need to be redesigned

	update();
	int baseNum = stat.get_num_comp(Protoss_Nexus);

	//caculate enemies' force, if we cannot defence, then train army
	//
	//calculate what kind of army do we need
	//
	//BWAPI::Broodwar->getFrameCount()

	double getEnemyArmyValue = 10, getArmyValue = 10, getMineralRate = 8, workerValue = 0;

	//getEnemyArmyValue = Evaluation::Instance().eval_enemy_in_double()  *  calculateArmyFactor(stat.get_num_comp(Protoss_Nexus)) ) ;
	//getEnemyArmyValue = std::max(1000.0, getEnemyArmyValue);

	getArmyValue = BWAPI::Broodwar->getUnitsInRadius(0, 0, 999999, Filter::CanAttack && !Filter::IsWorker &&Filter::IsOwned).size();

	//Broodwar->sendText("%d %d needArmy?", getEnemyArmyValue, getArmyValue);


	//int availableArmyBuilding = Broodwar->getUnitsInRadius( 0,0,999999, IsBuilding && IsIdle && !Is ).size() ;


	getArmyValue = Evaluation::Instance().eval_self_in_double();


	getEnemyArmyValue = Evaluation::Instance().eval_enemy_in_double();


	getEnemyArmyValue = std::max(20.0, getEnemyArmyValue);

	double factor = 65;

	if (Broodwar->enemy()->getRace() == Races::Protoss){
		factor = 130;
	}
	else if(Broodwar->enemy()->getRace() == Races::Zerg){
		factor=51;
	}

	getEnemyArmyValue = std::max(getEnemyArmyValue * calculateArmyFactor(Broodwar->self()->allUnitCount(Protoss_Nexus)) *1.0, getEnemyArmyValue + baseNum * baseNum * factor);


	//workerValue = stat.get_num_comp(UnitTypes::Protoss_Probe) * 1.5;
	workerValue = 0;

	const int baseMinaerlRate = 7;

	//enough army to defend
	//need to calculate
	//bug? need to fix
	if (getEnemyArmyValue >= getArmyValue + workerValue && stat.get_mineral_speed(10) >= baseMinaerlRate)
		return true;

	//don't waste money
	if (stat.get_cur_mineral() >= 400)
		return true;


	//Broodwar->sendText("%s","dont need army");

	return false;
}

bool DecisionMaker::needObserver(){

	//when floaked unit is detected
	//when the enemy is going to train cloaked unit
	//

	update();

	if (InformationManager::Instance().enemyHasCloakedUnits()){
		
		if (Broodwar->self()->allUnitCount(Protoss_Observer) >= 4)
			return false;

		return true;
	}

	//lucker

	return false;
}

bool DecisionMaker::needForge(){

	if (InformationManager::Instance().enemyHasCloakedUnits())
		return true;

	if ((stat.get_num_comp(Protoss_Nexus) == 2 && Broodwar->enemy()->getRace() == BWAPI::Races::Zerg) 
		|| (stat.get_num_comp(Protoss_Nexus)>2)){
		return true;
	}

	return false;
}

bool DecisionMaker::needSupply(){

	update();

	//if maximum supply is not 200 and no supply is building and we have enough budget and almost reach maximum supply
	//some units consume 3 supplies, be aware of deadlock
	

	int supplyTotal = 0;
	int supplyUsed = 0;
	supplyUsed = stat.get_population();//problem


	supplyTotal = numPylons * 8 + numNexusCompleted * 9;

	//Broodwar->sendText("%d %d %d", stat.get_population(), stat.get_population_limit(), supplyTotal);

	//Statistics::get_population();

	//Broodwar->sendText("%d %d %d", supplyTotal, supplyUsed, (supplyTotal <= 200) && (supplyTotal - supplyUsed <= 2));

	const double baseMineralRate = 13;

	if ((supplyTotal <= 200) && ( (supplyTotal - supplyUsed <= 2) || ( stat.get_mineral_speed() >= baseMineralRate && supplyTotal - supplyUsed <= 4) ) )
		return true;

	return false;
}

bool DecisionMaker::needGateway(){

	//depending on mineral rate

	Unit u = Broodwar->getClosestUnit(Position(0, 0));



	//int baseCount = Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	//int gatewayCount = Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Gateway);
	//int needGatewayNumber = std::max((int(BWAPI::Broodwar->getFrameCount() / 24 / 60)), 0);

	int baseCount = stat.get_num_comp(Protoss_Nexus);
	int gatewayCount = stat.get_num_comp(Protoss_Gateway) + stat.get_num_uncomp(Protoss_Gateway);
	int stargateCount = Broodwar->self()->allUnitCount(Protoss_Stargate);

	gatewayCount += stargateCount;

	int needGatewayNumber = 0;

	if (Broodwar->getFrameCount()<=5*24*60)
		needGatewayNumber = int(stat.get_mineral_speed(60) / 6) + 1; // based on zealot
	else
		needGatewayNumber = int(stat.get_mineral_speed(60) / 5) + 1;
	//BroodwarPtr->sendText("%d", baseCount);

	if (gatewayCount < baseCount * 3 && gatewayCount < needGatewayNumber && needArmy()){
		//Broodwar->sendText("%s","needgateway true");
		return true;
	}

	const int leastMoney = 500;
	if (gatewayCount < baseCount * 4 && gatewayCount < 12 && stat.get_cur_mineral() >= 500 && shoudExpand() == false)//train more army when enemy is strong
	{
		//Broodwar->sendText("%s", "should build extra gateway");
		return true;

	}
	return false;
}

bool DecisionMaker::needDragoon(){
	//
	int firstTiming = int(3.5 * 60);
	int leastZealots = 4;
	int mynumZealots = numZealots;
	if (Broodwar->enemy()->getRace() != Races::Zerg){
		mynumZealots = stat.get_num_comp(Protoss_Zealot);
		leastZealots = 6;
	}

	if (Broodwar->getFrameCount() / 24 >= firstTiming && numZealots >= leastZealots)
	{
		return true;
	}
	return false;
}


bool DecisionMaker::needGas(){


	int baseCount = Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int gasCount = Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Assimilator);
	//if gas is required or we have enough money

	const int baseWorkerNum = 17;

	if (gasCount < baseCount && (numProbes>baseWorkerNum || needDragoon()))
		return true;
	return false;
}


bool DecisionMaker::shoudExpand(){

	if (MapTools::Instance().getNextExpansion() == BWAPI::TilePositions::None){
		//BWAPI::Broodwar->printf("No valid expansion location");
		return false;
	}

	if (stat.get_num_uncomp(Protoss_Nexus) > 0){
		//Broodwar->sendText("%s", "already exist");
		return false;
	}


	const int mostMineral = 22;

	if (stat.get_ctrl_mineral().first >= mostMineral)// not too many bases
		return false;

	const int leatMineral = 3000;

	if (stat.get_ctrl_mineral().second <= leatMineral)
		return true;

	//if enemy's second base detected

	/*
	if (needArmy() == false && stat.get_cur_mineral() >= 500)
	return true;
	*/
	// should be discussed
	//Broodwar->sendText("%.4f %.4f", Evaluation::Instance().eval_self_in_double(), Evaluation::Instance().eval_enemy_in_double());
	/*
	if (Evaluation::Instance().eval_self_in_double() - Evaluation::Instance().eval_enemy_in_double() >=50+ stat.get_num_comp(Protoss_Nexus)*50  && stat.get_cur_mineral()>=300 ){
	//Broodwar->sendText("%s","need Base!");
	return true;
	}
	*/

	double self = Evaluation::Instance().eval_self_in_double();
	double enemy = Evaluation::Instance().eval_enemy_in_double();
	int baseNum = stat.get_num_comp(Protoss_Nexus);
	const double eps = 0.00000001;
	const double leastRatio = 1.2;

	static int cnt = 480;

	if (self - enemy >= baseNum * baseNum * 50.0
		&& (fabs(enemy) < eps || (fabs(enemy) > eps && self / enemy > leastRatio))){
		cnt--;
	}
	else
	{
		cnt = 480;
	}

	if (cnt <= 0 && stat.get_cur_mineral() >= 300)
		return true;

	//Broodwar->sendText("%d", stat.get_cur_mineral());


	return false;
}

bool DecisionMaker::needUpgradeRange(){

	const int baseDragoon = 6;

	int idleCore = 0;
	Unitset myUnits = BWAPI::Broodwar->self()->getUnits();
	for (auto &u : myUnits){

		if (u->getType() == BWAPI::UnitTypes::Protoss_Cybernetics_Core && u->isIdle())
			idleCore++;
	}
	if (idleCore == 0)
		return false;

	if (Broodwar->self()->allUnitCount(Protoss_Dragoon)>= baseDragoon
		&& Broodwar->self()->getUpgradeLevel( UpgradeTypes::Singularity_Charge )==0 )
		return true;

	return false;
}

bool DecisionMaker::needTemplar(){

	//not use
	return false;
	int basePop = 60, baseMineral = 300;// for test


	if (stat.get_population() >= basePop && stat.get_cur_mineral()>=baseMineral){
		return true;
	}

	return false;
}

bool DecisionMaker::needHighTemplar(){

	int baseGas = 500;
	if (stat.get_cur_gas() >= baseGas)
		return true;

	return false;
}

bool DecisionMaker::needZealotSpeed(){


	return false;
	//stop use this
	int baseZealot = 9;

	if (stat.get_num_comp(Protoss_Zealot) + stat.get_num_uncomp(Protoss_Zealot) >= baseZealot && Broodwar->self()->completedUnitCount( Protoss_Citadel_of_Adun) >0 ){

		return true;

	}

	return false;
}

bool DecisionMaker::needStargate(){//balance between gateway and stargate

	update();

	

	int numStargate = Broodwar->self()->allUnitCount( Protoss_Stargate );
	int numCarrier = Broodwar->self()->allUnitCount(Protoss_Carrier);

	const int maxStargate = 4;

	if (numStargate >= maxStargate)
		return false;

	if ((needCorsair() || needCarrier() || numCarrier>0) && numStargate < numNexusCompleted*1.1)
		return true;


	return false;
}


bool DecisionMaker::needFleetBeacon(){

	
	
	return false;
}

bool DecisionMaker::needCorsair(){
	//for Z
	return false;
	int idleStargate =0;

	Unitset myUnits = BWAPI::Broodwar->self()->getUnits();

	for (auto &u : myUnits){

		if (u->getType() == BWAPI::UnitTypes::Protoss_Stargate && u->isIdle())
			idleStargate++;
	}

	if (idleStargate > 0){

		Unitset allUnits = Broodwar->getAllUnits();

		

		for (auto &unit : allUnits){
			if (unit->getType() == Zerg_Hydralisk)
				hasHydralisk = true;

		}

		if (Broodwar->enemy()->getRace() == BWAPI::Races::Zerg && hasHydralisk==false){

			if (Broodwar->self()->allUnitCount(Protoss_Corsair) < 4)
				return true;
		}

		if (Broodwar->self()->allUnitCount(Protoss_Carrier)*0.49 > Broodwar->self()->allUnitCount(Protoss_Corsair))
			return true;

	}

	return false;
}

bool DecisionMaker::needCarrier(){
	//for Z T

	Unitset allUnits = Broodwar->getAllUnits();



	for (auto &unit : allUnits){
		if (unit->getType() == Terran_Siege_Tank_Tank_Mode || unit->getType()==Terran_Siege_Tank_Siege_Mode)
			hasTank = true;

	}

	/*
	if (Broodwar->enemy()->getRace() == BWAPI::Races::Terran && hasTank==true){
		if (stat.get_cur_mineral() >= 250)
			return true;
	}
	*/

	if (numNexusCompleted >= 2 && stat.get_cur_mineral()>=250)
		return true;

	return false;
}

bool DecisionMaker::needAirUpgrade(){
	//once

	int numCarrier = Broodwar->self()->allUnitCount(Protoss_Carrier) ;
	int currentLevel = Broodwar->self()->getUpgradeLevel( UpgradeTypes::Protoss_Air_Weapons ); 
	
	//Broodwar->sendText("%d", currentLevel);

	int idleCore = 0;
	Unitset myUnits = BWAPI::Broodwar->self()->getUnits();
	for (auto &u : myUnits){

		if (u->getType() == BWAPI::UnitTypes::Protoss_Cybernetics_Core && u->isIdle())
			idleCore++;
	}

	if (idleCore == 0)
		return false;

	if (numCarrier > 0 && currentLevel == 0)
		return true;

	if (numCarrier > 1 && currentLevel == 1){
		//Broodwar->sendText("%s", "need air weapon");
		return true;
	}

	if (numCarrier > 5 && currentLevel == 2)
		return true;

	return false;
}

bool DecisionMaker::needCapacity(){

	//int numCarrier = stat.get_num_uncomp(Protoss_Carrier);

	int numCompleteFleet = Broodwar->self()->completedUnitCount(Protoss_Fleet_Beacon);

	//once
	if (numCompleteFleet > 0)
		return true;

	return false;
}


