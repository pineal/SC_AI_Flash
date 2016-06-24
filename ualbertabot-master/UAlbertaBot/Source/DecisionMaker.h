#pragma once

#include "BWAPI.h"
#include "Common.h"
#include "BWTA.h"
#include "BuildOrderQueue.h"
#include "InformationManager.h"
#include "WorkerManager.h"
#include "BuildOrder.h"
#include "Statistics.h"
#include "Evaluation.h"


namespace UAlbertaBot
{
	class DecisionMaker{
	private:
		int numZealots;
		int numPylons;
		int numDragoons;
		int numProbes;
		int numNexusCompleted;
		int numNexusAll;
		int numCyber;
		int numCannon;
		int numScout;
		int numReaver;
		int numDarkTeplar;
	    bool hasHydralisk;
		bool hasTank;
		

	public:

		DecisionMaker();
		

		static	DecisionMaker &	    Instance();


		//TODO:
		bool needWorker();
		bool needArmy();
		bool needObserver();
		bool shoudExpand();
		bool needSupply();
		bool needUpgrade();
		bool needTech();
		bool needGateway();
		bool needGas();
		bool needDragoon();
		bool needUpgradeRange();
		bool needTemplar();
		bool needHighTemplar();
		bool needZealotSpeed();
		bool needStargate();
		bool needFleetBeacon();
		bool needCorsair();
		bool needCarrier();
		bool needAirUpgrade();
		bool needCapacity();
		bool needForge();

		Statistics stat;
		//Evaluation eva;


		//Evaluation & getEvaluation();
		Statistics & getStatistics();


		void update();

	};
}