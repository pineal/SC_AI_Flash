#pragma once

#include "MapGrid.h"
#include <BWAPI.h>
#include <set>
#include <unordered_map>
#include <BWTA/Polygon.h>
#include <BWTA/Region.h>
#include <map>
#include "UnitUtil.h"
#include "Common.h"
#include "InformationManager.h"
#include "MapTools.h"
namespace UAlbertaBot
{
	class Region{
	private:
		BWTA::Region* region;
		int				  ID;
		double		 ditance;
	public:
	};

	//enum playerType	
	class RegionManager{
	private:
		std::vector<Region> myRegions;
		std::set<BWTA::Region*> polyRegions;
		std::set<BWAPI::Position> jointPoints;
		std::unordered_map<BWAPI::Position, double,UnitUtil::PositionHasher> scores;
		BWAPI::Position myCenter;
		BWAPI::Position enemyCenter;
		static std::map<BWAPI::TilePosition, double> distFromHm;
		static std::map<BWAPI::TilePosition, BWAPI::TilePosition> choke2choke;
		static std::map<BWAPI::TilePosition, std::vector<BWAPI::TilePosition>> choke2path;
		static std::map<BWAPI::TilePosition, BWAPI::Position> choke2CloserDefend;
	public:
		static RegionManager & Instance();
		RegionManager();
		~RegionManager(){};
		void init();
		void valueUpdate();
		BWTA::Polygon getPlayerRegion(BWAPI::Player player);
		void drawPolygon(BWTA::Polygon p, BWAPI::Color color);
		void drawAllPolygon(BWAPI::Color color);
		void drawPlayerRegion(BWAPI::Player player, BWAPI::Color color);
		void drawUnwalkableRegion();
		void drawJointPoints();
		std::vector<std::vector<BWAPI::Region>> getPaths(BWAPI::Region start, BWAPI::Region end);
		BWAPI::Position findOptimalPoint(std::set<BWAPI::Position> positions);
		void drawScores();
		BWAPI::Position optimalRegroupPosition1(BWAPI::Position p);
		BWAPI::Position optimalRegroupPosition2(BWAPI::Position p);

		BWAPI::Position optimalRegroupPosition(BWAPI::Position p);
		static void onDrawRegroupPosition();
		BWTA::Chokepoint* optimalChokePoint(BWAPI::Position p);
		static void calcDistance();
		std::pair<BWAPI::TilePosition, BWAPI::TilePosition> getExactRegroupPosition(BWAPI::TilePosition tp, double radius);
		BWAPI::Position getNextPointToBase(BWAPI::TilePosition tp);
	};
}
