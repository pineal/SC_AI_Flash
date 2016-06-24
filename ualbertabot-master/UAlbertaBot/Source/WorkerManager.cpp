#include "Common.h"
#include "WorkerManager.h"
#include "Micro.h"

using namespace UAlbertaBot;

WorkerManager::WorkerManager()
{
	numMinerals = 0;
	previousClosestWorker = nullptr;
}

WorkerManager & WorkerManager::Instance()
{
	static WorkerManager instance;
	return instance;
}

void WorkerManager::update()
{
	updateWorkerStatus();
	handleGasWorkers();
	handleIdleWorkers();
	handleMoveWorkers();
	handleCombatWorkers();
	handleRepairWorkers();
	rearrangeWorkersToNewNexus();

	drawResourceDebugInfo();
	drawWorkerInformation(450, 20);
	workerData.drawDepotDebugInfo();
}

void WorkerManager::updateWorkerStatus()
{
	// for each of our Workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker->isCompleted())
		{
			continue;
		}

		// if it's idle
		if (worker->isIdle() &&
			(workerData.getWorkerJob(worker) != WorkerData::Build) &&
			(workerData.getWorkerJob(worker) != WorkerData::Move) &&
			(workerData.getWorkerJob(worker) != WorkerData::Scout))
		{
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}

		// if its job is gas
		if (workerData.getWorkerJob(worker) == WorkerData::Gas)
		{
			BWAPI::Unit refinery = workerData.getWorkerResource(worker);

			// if the refinery doesn't exist anymore
			if (!refinery || !refinery->exists() || refinery->getHitPoints() <= 0)
			{
				setMineralWorker(worker);
			}
		}
	}
}

void WorkerManager::setRepairWorker(BWAPI::Unit worker, BWAPI::Unit unitToRepair)
{
	workerData.setWorkerJob(worker, WorkerData::Repair, unitToRepair);
}

void WorkerManager::stopRepairing(BWAPI::Unit worker)
{
	workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
}

void WorkerManager::handleGasWorkers()
{
	// for each unit we have
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		// if that unit is a refinery
		if (unit->getType().isRefinery() && unit->isCompleted() && isGasMineNearCompletedDepot(unit) && !isGasStealRefinery(unit))
		{
			// get the number of workers currently assigned to it
			int numAssigned = workerData.getNumAssignedWorkers(unit);
			int curMineral = DecisionMaker::Instance().stat.get_cur_mineral();
			int curGas = DecisionMaker::Instance().stat.get_cur_gas();
			// BWAPI::Broodwar->sendText("Gas is %d, Mineral is %d", curGas, curMineral);
			int workersPerRefinery = Config::Macro::WorkersPerRefinery;
			if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
			{
				if (curGas - curMineral > 500)
				{
					workersPerRefinery -= 1;
				}
				else if (curGas - curMineral < 300)
				{
					workersPerRefinery = Config::Macro::WorkersPerRefinery;
				}
				else if (curGas - curMineral > 800 && curGas - curMineral < 1000)
				{
					workersPerRefinery -= 2;
				}
				else if (curGas - curMineral > 1200)
				{
					workersPerRefinery -= 3;
				}
				else
				{
					return;
				}
			}
			else
			{
				if (curGas - curMineral > 200)
				{
					workersPerRefinery -= 1;
				}
				else if (curGas - curMineral < 0)
				{
					workersPerRefinery = Config::Macro::WorkersPerRefinery;
				}
				else if (curGas - curMineral > 400 && curGas - curMineral < 600)
				{
					workersPerRefinery -= 2;
				}
				else if (curGas - curMineral > 800)
				{
					workersPerRefinery -= 3;
				}
				else
				{
					return;
				}
			}
			//if (curGas > 100 && curMineral > 100) {
			
			//}
			
			//if (refineryMap.find(unit) != refineryMap.end() && refineryMap.find(unit)->second == workersPerRefinery && workersPerRefinery == numAssigned)
			//{
				//return;
			//}

			//refineryMap[unit] = workersPerRefinery;

			int numDiff = workersPerRefinery - numAssigned;
			if (numDiff != 0) 
			{
				//BWAPI::Broodwar->sendText("workersPerRefinery - numAssigned is %d", numDiff);

				if (numDiff < 0)
				{
					for (int i = 0; i < -numDiff; i++)
					{
						BWAPI::Unit gasWorker = getGasWorkerBelongto(unit);
						if (gasWorker)
						{
							//BWAPI::Broodwar->sendText("move gas worker to mineral");
							setMineralWorker(gasWorker);
						}
					}
				}
				else
				{
					// if it's less than we want it to be, fill 'er up
					for (int i = 0; i < numDiff; ++i)
					{
						BWAPI::Unit gasWorker = getGasWorker(unit);
						if (gasWorker)
						{
							//BWAPI::Broodwar->sendText("move mineral worker to gas");
							workerData.setWorkerJob(gasWorker, WorkerData::Gas, unit);
						}
					}
				}
			}
		}
	}

}

bool WorkerManager::isGasMineNearCompletedDepot(BWAPI::Unit gasMine)
{
	if (!gasMine) { return 0; }

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if ((unit->getType() == BWAPI::UnitTypes::Protoss_Nexus) && unit->isCompleted() && unit->getDistance(gasMine) < 200)
		{
			return true;
		}
	}

	return false;
}

bool WorkerManager::isGasStealRefinery(BWAPI::Unit unit)
{
	BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	if (!enemyBaseLocation)
	{
		return false;
	}

	if (enemyBaseLocation->getGeysers().empty())
	{
		return false;
	}

	for (auto & u : enemyBaseLocation->getGeysers())
	{
		if (unit->getTilePosition() == u->getTilePosition())
		{
			return true;
		}
	}

	return false;
}

void WorkerManager::handleIdleWorkers()
{
	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");

		// if it is idle
		if (workerData.getWorkerJob(worker) == WorkerData::Idle)
		{
			// send it to the nearest mineral patch
			setMineralWorker(worker);
		}
	}
}

void WorkerManager::handleRepairWorkers()
{
	if (BWAPI::Broodwar->self()->getRace() != BWAPI::Races::Terran)
	{
		return;
	}

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getType().isBuilding() && (unit->getHitPoints() < unit->getType().maxHitPoints()))
		{
			BWAPI::Unit repairWorker = getClosestMineralWorkerTo(unit);
			setRepairWorker(repairWorker, unit);
			break;
		}
	}
}

// bad micro for combat workers
void WorkerManager::handleCombatWorkers()
{
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");

		if (workerData.getWorkerJob(worker) == WorkerData::Combat)
		{
			BWAPI::Broodwar->drawCircleMap(worker->getPosition().x, worker->getPosition().y, 4, BWAPI::Colors::Yellow, true);
			BWAPI::Unit target = getClosestEnemyUnit(worker);

			if (target)
			{
				Micro::SmartAttackUnit(worker, target);
			}
		}
	}
}

BWAPI::Unit WorkerManager::getClosestEnemyUnit(BWAPI::Unit worker)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 10000;

	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		double dist = unit->getDistance(worker);

		if ((dist < 400) && (!closestUnit || (dist < closestDist)))
		{
			closestUnit = unit;
			closestDist = dist;
		}
	}

	return closestUnit;
}

void WorkerManager::finishedWithCombatWorkers()
{
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");

		if (workerData.getWorkerJob(worker) == WorkerData::Combat)
		{
			setMineralWorker(worker);
		}
	}
}

BWAPI::Unit WorkerManager::getClosestMineralWorkerTo(BWAPI::Unit enemyUnit)
{
	UAB_ASSERT(enemyUnit != nullptr, "enemyUnit was null");

	BWAPI::Unit closestMineralWorker = nullptr;
	double closestDist = 100000;

	if (previousClosestWorker)
	{
		if (previousClosestWorker->getHitPoints() > 0)
		{
			return previousClosestWorker;
		}
	}

	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");
		if (!worker)
		{
			continue;
		}
		// if it is a move worker
		if (workerData.getWorkerJob(worker) == WorkerData::Minerals)
		{
			double dist = worker->getDistance(enemyUnit);

			if (!closestMineralWorker || dist < closestDist)
			{
				closestMineralWorker = worker;
				dist = closestDist;
			}
		}
	}

	previousClosestWorker = closestMineralWorker;
	return closestMineralWorker;
}

BWAPI::Unit WorkerManager::getWorkerScout()
{
	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");
		if (!worker)
		{
			continue;
		}
		// if it is a move worker
		if (workerData.getWorkerJob(worker) == WorkerData::Scout)
		{
			return worker;
		}
	}

	return nullptr;
}

void WorkerManager::handleMoveWorkers()
{
	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");

		// if it is a move worker
		if (workerData.getWorkerJob(worker) == WorkerData::Move)
		{
			WorkerMoveData data = workerData.getWorkerMoveData(worker);

			Micro::SmartMove(worker, data.position);
		}
	}
}

// set a worker to mine minerals
void WorkerManager::setMineralWorker(BWAPI::Unit unit)
{
	UAB_ASSERT(unit != nullptr, "Unit was null");

	// check if there is a mineral available to send the worker to
	BWAPI::Unit depot = getClosestDepot(unit);

	// if there is a valid mineral
	if (depot)
	{
		// update workerData with the new job
		workerData.setWorkerJob(unit, WorkerData::Minerals, depot);
	}
	else
	{
		// BWAPI::Broodwar->printf("No valid depot for mineral worker");
	}
}

void WorkerManager::setMineralWorker(BWAPI::Unit unit, BWAPI::Unit depot)
{
	UAB_ASSERT(unit != nullptr && depot != nullptr, "Unit or depot was null");
	// BWAPI::Broodwar->sendText("Hello world!");
	workerData.setWorkerJob(unit, WorkerData::Minerals, depot);
}

void WorkerManager::rearrangeWorkersToNewNexus()
{
	numMinerals++;
	if (numMinerals%120 != 1)
	{
		return;
	}

	BWAPI::Unitset bases = getBases();

	std::set<BWAPI::Unit> workers;

	for (auto base : bases)
	{
		int numWorkers = workerData.getNumAssignedWorkers(base);
		int numMinerals = workerData.getMineralsNearDepot(base);

		if (numWorkers < numMinerals)
		{
			int count = numMinerals - numWorkers;

			int idleWorkerNum = workerData.getNumIdleWorkers();

			bool useMineralWorkers = (idleWorkerNum >= count) ? false : true;

			// first arrange the idle workers
			for (auto & worker : workerData.getWorkers())
			{
				if (count > 0 && workerData.getWorkerJob(worker) == WorkerData::Idle)
				{
					setMineralWorker(worker, base);
					workers.insert(worker);
					count--;
				}
			}

			// then arrange the overfitting workers
			if (count > 0)
			{
				for (auto otherbase : bases)
				{
					int nw = workerData.getNumAssignedWorkers(otherbase);
					int nm = workerData.getMineralsNearDepot(otherbase);

					if (nm > 0 && nw/nm > 2)
					{
						int mcount = nw%nm;
						for (auto & worker : workerData.getWorkers())
						{
							if (count > 0 && mcount > 0 && workerData.getWorkerJob(worker) == WorkerData::Minerals && getClosestDepot(worker) == otherbase && workers.find(worker) == workers.end())
							{
								setMineralWorker(worker, base);
								workers.insert(worker);
								count--;
								mcount--;
							}
						}
					}
				}
			}

			// last other worker
			if (count > 0)
			{
				for (auto & worker : workerData.getWorkers())
				{
					if (count > 0 && workerData.getWorkerJob(worker) == WorkerData::Minerals && getClosestDepot(worker) != base && workers.find(worker) == workers.end())
					{
						setMineralWorker(worker, base);
						workers.insert(worker);
						count--;
					}
				}
			}
		}
	}

	// when all mineral for one nexus is done
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");
		if (workerData.getWorkerJob(worker) == WorkerData::Idle) {
			BWAPI::Unit depot = getClosestDepot(worker);
			setMineralWorker(worker, depot);
		}
	}

/*	int numNexusCompleted = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);

	int numMineralHave = 0;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");
		if (unit->getType().isResourceDepot() && unit->isCompleted())
		{
			int numMinerals = workerData.getMineralsNearDepot(unit);
			numMineralHave += numMinerals;
		}
	}

	if (numMinerals == 0)
	{
		numMinerals = numMineralHave;
	}

	if (numNexusCompleted == 1 || numMineralHave == numMinerals)
	{
		return;
	}

	numMinerals = numMineralHave;

	//BWAPI::Broodwar->sendText("NO. of Minerals is %d", numMinerals);

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");
		if (unit->getType().isResourceDepot() && unit->isCompleted())
		{
			int numWorkers = workerData.getNumAssignedWorkers(unit);
			int numMinerals = workerData.getMineralsNearDepot(unit);

			//BWAPI::Broodwar->sendText("%s%d", "NO. of workers is ", numWorkers);
			//BWAPI::Broodwar->sendText("%s%d", "NO. of minerals is ", numMinerals);

			if (numWorkers < numMinerals)
			{
				int count = numMinerals - numWorkers;

				int idleWorkerNum = workerData.getNumIdleWorkers();

				bool useMineralWorkers = (idleWorkerNum >= count) ? false : true;

				for (auto & worker : workerData.getWorkers())
				{
					UAB_ASSERT(worker != nullptr, "Worker was null");
					if (workerData.getWorkerJob(worker) == WorkerData::Idle || (useMineralWorkers && workerData.getWorkerJob(worker) == WorkerData::Minerals && getClosestDepot(worker) != unit))
					{
						setMineralWorker(worker, unit);
						if (--count < 0)
						{
							break;
						}
					}
				}
			}
		}
	}*/
}

BWAPI::Unit WorkerManager::getClosestDepot(BWAPI::Unit worker)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	BWAPI::Unit closestDepot = nullptr;
	double closestDistance = 0;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");

		if (unit->getType().isResourceDepot() && (unit->isCompleted() || unit->getType() == BWAPI::UnitTypes::Zerg_Lair) && !workerData.depotIsFull(unit) && workerData.getMineralsNearDepot(unit) > 0)
		{
			double distance = unit->getDistance(worker);
			if (!closestDepot || distance < closestDistance)
			{
				closestDepot = unit;
				closestDistance = distance;
			}
		}
	}

	return closestDepot;
}


// other managers that need workers call this when they're done with a unit
void WorkerManager::finishedWithWorker(BWAPI::Unit unit)
{
	UAB_ASSERT(unit != nullptr, "Unit was null");

	//BWAPI::Broodwar->printf("BuildingManager finished with worker %d", unit->getID());
	if (workerData.getWorkerJob(unit) != WorkerData::Scout)
	{
		workerData.setWorkerJob(unit, WorkerData::Idle, nullptr);
	}
}

BWAPI::Unit WorkerManager::getGasWorker(BWAPI::Unit refinery)
{
	UAB_ASSERT(refinery != nullptr, "Refinery was null");

	BWAPI::Unit closestWorker = nullptr;
	double closestDistance = 0;

	for (auto & unit : workerData.getWorkers())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");

		if (workerData.getWorkerJob(unit) == WorkerData::Minerals)
		{
			double distance = unit->getDistance(refinery);
			if (!closestWorker || distance < closestDistance)
			{
				closestWorker = unit;
				closestDistance = distance;
			}
		}
	}

	return closestWorker;
}

BWAPI::Unit WorkerManager::getGasWorkerBelongto(BWAPI::Unit refinery)
{
	UAB_ASSERT(refinery != nullptr, "Refinery was null");

	BWAPI::Unit closestWorker = nullptr;
	double closestDistance = 0;

	for (auto & unit : workerData.getWorkers())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");

		if (workerData.getWorkerJob(unit) == WorkerData::Gas)
		{
			double distance = unit->getDistance(refinery);
			if (!closestWorker || distance < closestDistance)
			{
				closestWorker = unit;
				closestDistance = distance;
			}
		}
	}

	return closestWorker;
}

void WorkerManager::setBuildingWorker(BWAPI::Unit worker, Building & b)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	workerData.setWorkerJob(worker, WorkerData::Build, b.type);
}

// gets a builder for BuildingManager to use
// if setJobAsBuilder is true (default), it will be flagged as a builder unit
// set 'setJobAsBuilder' to false if we just want to see which worker will build a building
BWAPI::Unit WorkerManager::getBuilder(Building & b, bool setJobAsBuilder)
{
	// variables to hold the closest worker of each type to the building
	BWAPI::Unit closestMovingWorker = nullptr;
	BWAPI::Unit closestMiningWorker = nullptr;
	double closestMovingWorkerDistance = 0;
	double closestMiningWorkerDistance = 0;

	// look through each worker that had moved there first
	for (auto & unit : workerData.getWorkers())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");

		// gas steal building uses scout worker
		if (b.isGasSteal && (workerData.getWorkerJob(unit) == WorkerData::Scout))
		{
			if (setJobAsBuilder)
			{
				workerData.setWorkerJob(unit, WorkerData::Build, b.type);
			}
			return unit;
		}

		// mining worker check
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Minerals))
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(BWAPI::Position(b.finalPosition));
			if (!closestMiningWorker || distance < closestMiningWorkerDistance)
			{
				closestMiningWorker = unit;
				closestMiningWorkerDistance = distance;
			}
		}

		// moving worker check
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Move))
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(BWAPI::Position(b.finalPosition));
			if (!closestMovingWorker || distance < closestMovingWorkerDistance)
			{
				closestMovingWorker = unit;
				closestMovingWorkerDistance = distance;
			}
		}
	}

	// if we found a moving worker, use it, otherwise using a mining worker
	BWAPI::Unit chosenWorker = closestMovingWorker ? closestMovingWorker : closestMiningWorker;

	// if the worker exists (one may not have been found in rare cases)
	if (chosenWorker && setJobAsBuilder)
	{
		workerData.setWorkerJob(chosenWorker, WorkerData::Build, b.type);
	}

	// return the worker
	return chosenWorker;
}

// sets a worker as a scout
void WorkerManager::setScoutWorker(BWAPI::Unit worker)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	workerData.setWorkerJob(worker, WorkerData::Scout, nullptr);
}

// gets a worker which will move to a current location
BWAPI::Unit WorkerManager::getMoveWorker(BWAPI::Position p)
{
	// set up the pointer
	BWAPI::Unit closestWorker = nullptr;
	double closestDistance = 0;

	// for each worker we currently have
	for (auto & unit : workerData.getWorkers())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");

		// only consider it if it's a mineral worker
		if (unit->isCompleted() && workerData.getWorkerJob(unit) == WorkerData::Minerals)
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(p);
			if (!closestWorker || distance < closestDistance)
			{
				closestWorker = unit;
				closestDistance = distance;
			}
		}
	}

	// return the worker
	return closestWorker;
}

// sets a worker to move to a given location
void WorkerManager::setMoveWorker(int mineralsNeeded, int gasNeeded, BWAPI::Position p)
{
	// set up the pointer
	BWAPI::Unit closestWorker = nullptr;
	double closestDistance = 0;

	// for each worker we currently have
	for (auto & unit : workerData.getWorkers())
	{
		UAB_ASSERT(unit != nullptr, "Unit was null");

		// only consider it if it's a mineral worker
		if (unit->isCompleted() && workerData.getWorkerJob(unit) == WorkerData::Minerals)
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(p);
			if (!closestWorker || distance < closestDistance)
			{
				closestWorker = unit;
				closestDistance = distance;
			}
		}
	}

	if (closestWorker)
	{
		//BWAPI::Broodwar->printf("Setting worker job Move for worker %d", closestWorker->getID());
		workerData.setWorkerJob(closestWorker, WorkerData::Move, WorkerMoveData(mineralsNeeded, gasNeeded, p));
	}
	else
	{
		//BWAPI::Broodwar->printf("Error, no worker found");
	}
}

// will we have the required resources by the time a worker can travel a certain distance
bool WorkerManager::willHaveResources(int mineralsRequired, int gasRequired, double distance)
{
	// if we don't require anything, we will have it
	if (mineralsRequired <= 0 && gasRequired <= 0)
	{
		return true;
	}

	// the speed of the worker unit
	double speed = BWAPI::Broodwar->self()->getRace().getWorker().topSpeed();

	UAB_ASSERT(speed > 0, "Speed is negative");

	// how many frames it will take us to move to the building location
	// add a second to account for worker getting stuck. better early than late
	double framesToMove = (distance / speed) + 50;

	// magic numbers to predict income rates
	double mineralRate = getNumMineralWorkers() * 0.045;
	double gasRate = getNumGasWorkers() * 0.07;

	// calculate if we will have enough by the time the worker gets there
	if (mineralRate * framesToMove >= mineralsRequired &&
		gasRate * framesToMove >= gasRequired)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void WorkerManager::setCombatWorker(BWAPI::Unit worker)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	workerData.setWorkerJob(worker, WorkerData::Combat, nullptr);
}

void WorkerManager::onUnitMorph(BWAPI::Unit unit)
{
	UAB_ASSERT(unit != nullptr, "Unit was null");

	// if something morphs into a worker, add it
	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getHitPoints() >= 0)
	{
		workerData.addWorker(unit);
	}

	// if something morphs into a building, it was a worker?
	if (unit->getType().isBuilding() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getPlayer()->getRace() == BWAPI::Races::Zerg)
	{
		//BWAPI::Broodwar->printf("A Drone started building");
		workerData.workerDestroyed(unit);
	}
}

void WorkerManager::onUnitShow(BWAPI::Unit unit)
{
	UAB_ASSERT(unit != nullptr, "Unit was null");

	// add the depot if it exists
	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		workerData.addDepot(unit);
	}

	// if something morphs into a worker, add it
	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getHitPoints() >= 0)
	{
		//BWAPI::Broodwar->printf("A worker was shown %d", unit->getID());
		workerData.addWorker(unit);
	}
}


void WorkerManager::rebalanceWorkers()
{
	// for each worker
	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");

		if (!workerData.getWorkerJob(worker) == WorkerData::Minerals)
		{
			continue;
		}

		BWAPI::Unit depot = workerData.getWorkerDepot(worker);

		if (depot && workerData.depotIsFull(depot))
		{
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}
		else if (!depot)
		{
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}
	}
}

void WorkerManager::onUnitDestroy(BWAPI::Unit unit)
{
	UAB_ASSERT(unit != nullptr, "Unit was null");

	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		workerData.removeDepot(unit);
	}

	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		workerData.workerDestroyed(unit);
	}

	if (unit->getType() == BWAPI::UnitTypes::Resource_Mineral_Field)
	{
		rebalanceWorkers();
	}
}

void WorkerManager::drawResourceDebugInfo()
{
	if (!Config::Debug::DrawResourceInfo)
	{
		return;
	}

	for (auto & worker : workerData.getWorkers())
	{
		UAB_ASSERT(worker != nullptr, "Worker was null");

		char job = workerData.getJobCode(worker);

		BWAPI::Position pos = worker->getTargetPosition();

		BWAPI::Broodwar->drawTextMap(worker->getPosition().x, worker->getPosition().y - 5, "\x07%c", job);

		BWAPI::Broodwar->drawLineMap(worker->getPosition().x, worker->getPosition().y, pos.x, pos.y, BWAPI::Colors::Cyan);

		BWAPI::Unit depot = workerData.getWorkerDepot(worker);
		if (depot)
		{
			BWAPI::Broodwar->drawLineMap(worker->getPosition().x, worker->getPosition().y, depot->getPosition().x, depot->getPosition().y, BWAPI::Colors::Orange);
		}
	}
}

void WorkerManager::drawWorkerInformation(int x, int y)
{
	if (!Config::Debug::DrawWorkerInfo)
	{
		return;
	}

	BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Workers %d", workerData.getNumMineralWorkers());
	BWAPI::Broodwar->drawTextScreen(x, y + 20, "\x04 UnitID");
	BWAPI::Broodwar->drawTextScreen(x + 50, y + 20, "\x04 State");

	int yspace = 0;

	for (auto & unit : workerData.getWorkers())
	{
		UAB_ASSERT(unit != nullptr, "Worker was null");

		BWAPI::Broodwar->drawTextScreen(x, y + 40 + ((yspace)* 10), "\x03 %d", unit->getID());
		BWAPI::Broodwar->drawTextScreen(x + 50, y + 40 + ((yspace++) * 10), "\x03 %c", workerData.getJobCode(unit));
	}
}

bool WorkerManager::isFree(BWAPI::Unit worker)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	return workerData.getWorkerJob(worker) == WorkerData::Minerals || workerData.getWorkerJob(worker) == WorkerData::Idle;
}

bool WorkerManager::isWorkerScout(BWAPI::Unit worker)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	return (workerData.getWorkerJob(worker) == WorkerData::Scout);
}

bool WorkerManager::isBuilder(BWAPI::Unit worker)
{
	UAB_ASSERT(worker != nullptr, "Worker was null");

	return (workerData.getWorkerJob(worker) == WorkerData::Build);
}

int WorkerManager::getNumMineralWorkers()
{
	return workerData.getNumMineralWorkers();
}

int WorkerManager::getNumIdleWorkers()
{
	return workerData.getNumIdleWorkers();
}

int WorkerManager::getNumGasWorkers()
{
	return workerData.getNumGasWorkers();
}

BWAPI::Unitset WorkerManager::getBases()
{
	BWAPI::Unitset bases;
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getType().isResourceDepot() && unit->isCompleted())
		{
			bases.insert(unit);
		}
	}
	return bases;
}