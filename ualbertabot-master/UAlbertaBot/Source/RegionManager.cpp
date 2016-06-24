#include "RegionManager.h"
using namespace UAlbertaBot;
std::map<BWAPI::TilePosition, double> RegionManager::distFromHm;
std::map<BWAPI::TilePosition, BWAPI::TilePosition> RegionManager::choke2choke;
std::map<BWAPI::TilePosition, std::vector<BWAPI::TilePosition>> RegionManager::choke2path;
std::map<BWAPI::TilePosition, BWAPI::Position> RegionManager::choke2CloserDefend;

//optional
void RegionManager::calcDistance()
{
	std::set<BWAPI::TilePosition> poss;
	BWAPI::TilePosition hm(BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition());
	for (auto region : BWTA::getRegions())
	{
		for (auto candidate : region->getChokepoints())
			poss.insert(BWAPI::TilePosition(candidate->getCenter()));
	}
	distFromHm = BWTA::getGroundDistances(hm, poss);
	//deal with choke points
	for (auto region : BWTA::getRegions())
	{
		for (auto candidate : region->getChokepoints())
		{
			BWAPI::TilePosition tc(candidate->getCenter());
			choke2choke[tc] = tc;
			double mindist = 1000000;
			if (candidate->getRegions().first == BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion() || 
				candidate->getRegions().second == BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion())
				continue;
			auto neighbors = candidate->getRegions().first->getChokepoints();
			neighbors.insert(candidate->getRegions().second->getChokepoints().begin(), candidate->getRegions().second->getChokepoints().end());
			for (auto candidateA : neighbors)
			{
				auto swaped = candidateA->getRegions();
				std::swap(swaped.first, swaped.second);
				if (candidate->getRegions() == candidateA->getRegions() || candidate->getRegions() == swaped)
					continue;
				BWAPI::TilePosition ta(candidateA->getCenter());
				auto localdist = BWTA::getGroundDistance(ta, tc);
				if (localdist < 0 || distFromHm[ta] < 0)
					continue;
				if (localdist + distFromHm[ta] < mindist)
				{
					mindist = localdist + distFromHm[ta];
					choke2choke[tc] = ta;
				}
			}
		}
	}
	for (auto region : BWTA::getRegions())
	{
		for (auto candidate : region->getChokepoints())
		{
			BWAPI::TilePosition tc(candidate->getCenter());
			auto father = BWTA::getNearestChokepoint(choke2choke[tc]);
			BWTA::Region *defendRegion;
			if (father != candidate)
			{
				if (father->getRegions().first == candidate->getRegions().first || father->getRegions().first == candidate->getRegions().second)
					defendRegion = father->getRegions().first;
				else
					defendRegion = father->getRegions().second;
			}
			else
			{
				defendRegion = BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion();
			}
			choke2path[tc] = BWTA::getShortestPath(tc,choke2choke[tc]);
			
			std::reverse(choke2path[tc].begin(), choke2path[tc].end());
			auto vec = candidate->getSides().first - candidate->getSides().second;
			auto vec1 = vec;
			vec1.x = vec.y;
			vec1.y = -vec.x;
			vec1.x = (int)(64.0 * vec1.x / (sqrt(vec.x*vec.x + vec.y*vec.y)));
			vec1.y = (int)(64.0 * vec1.y / (sqrt(vec.x*vec.x + vec.y*vec.y)));
			
			auto p1 = candidate->getCenter() + vec1;
			auto p2 = candidate->getCenter() - vec1;

			if (BWTA::getRegion(p1) == defendRegion)
				choke2CloserDefend[tc] = p1;
			else
				choke2CloserDefend[tc] = p2;
		}
	}
}
//optional
BWAPI::Position RegionManager::optimalRegroupPosition(BWAPI::Position p)
{
	BWAPI::TilePosition tp(p);
	BWAPI::TilePosition hm(BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition());
	BWAPI::Position res(hm); 
	//if it is already a choke point
	if (choke2choke.find(tp) != choke2choke.end())
		return BWAPI::Position(choke2choke[tp]);
	auto occupied = InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->self());
	auto r = BWTA::getRegion(p);
	if (r == NULL)
		return BWTA::getNearestChokepoint(p)->getCenter();
	try
	{
		if (false)
		{
			double dist = 0;
			for (auto candidate : r->getChokepoints())
			{
				BWAPI::TilePosition tc(candidate->getCenter());
				auto localdist = tp.getDistance(tc);
				if (distFromHm[tc] < 0 || localdist < 0)
				{
					continue;
				}
				if (localdist + distFromHm[tc] > dist)
				{
					dist = localdist + distFromHm[tc];
					res = candidate->getCenter();
				}
			}
		}
		else
		{
			double dist = 1000000;
			for (auto candidate : r->getChokepoints())
			{
				BWAPI::TilePosition tc(candidate->getCenter());
				auto localdist = tp.getDistance(tc);
				if (distFromHm[tc] < 0 || localdist < 0)
				{
					continue;
				}
				if (localdist + distFromHm[tc] < dist)
				{
					dist = localdist + distFromHm[tc];
					res = candidate->getCenter();
				}
			}
		}
	}
	catch (std::bad_function_call e)
	{//only when getGroundDistances or getNearestChokepoint cannot be trusted
		BWAPI::Broodwar << "Error orccurred int optimalRegroupPosition" << std::endl;
		return BWAPI::Position(hm);
	}
	return res;
}
//optional
void RegionManager::onDrawRegroupPosition()
{
	/*
	auto regions = BWTA::getRegions();
	for (auto region : regions)
	{
		BWAPI::Broodwar->drawLineMap(region->getCenter(), 
									 RegionManager::Instance().optimalRegroupPosition(region->getCenter()), 
									 BWAPI::Colors::Yellow);
	}
	*/
	for (auto region : BWTA::getRegions())
	{
		for (auto candidate : region->getChokepoints())
		{

			BWAPI::TilePosition tc(candidate->getCenter());
			BWAPI::Broodwar->drawLineMap(candidate->getCenter(),
				BWAPI::Position(choke2choke[tc]),
				BWAPI::Colors::Yellow);
			BWAPI::Broodwar->drawTextMap(candidate->getCenter(), "%lf", distFromHm[tc]);
			for (auto path : choke2path[tc])
				BWAPI::Broodwar->drawCircleMap(BWAPI::Position(path), 2, BWAPI::Colors::Blue, true);
			auto pos = RegionManager::Instance().getExactRegroupPosition(tc, 350);
			BWAPI::Broodwar->drawCircleMap(BWAPI::Position(pos.first), 10, BWAPI::Colors::White, true);
			BWAPI::Broodwar->drawCircleMap(BWAPI::Position(pos.second), 10, BWAPI::Colors::Teal, true);
		}
	}
}


RegionManager & RegionManager::Instance() {
	static RegionManager instance;
	return instance;
}

RegionManager::RegionManager(){
	init();
}

void RegionManager::init(){

	polyRegions.clear();
	jointPoints.clear();
	scores.clear();


	BWAPI::Position myBaseLocation = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	//BWAPI::Position enemyBaseLocation = BWAPI::Position(BWAPI::Broodwar->enemy()->getStartLocation());
	

	for (std::set<BWTA::Region*>::const_iterator r = BWTA::getRegions().begin(); r != BWTA::getRegions().end(); r++)
	{
		polyRegions.insert(*r);
		for (auto boundries : (*r)->getChokepoints()){
			if (jointPoints.find(boundries->getCenter()) == jointPoints.end()){
				jointPoints.insert(boundries->getCenter());
			}
		}
	}

	//double totalDistance = enemyBaseLocation.getDistance(myBaseLocation);
	double totalDistance = MapTools::Instance().getGroundDistance(myBaseLocation, myBaseLocation);

	for (auto p : polyRegions) {
		double thisDistance = MapTools::Instance().getGroundDistance(myBaseLocation, p->getCenter());
		//unwalkable region
		if (thisDistance <= 0){
			scores.emplace(p->getCenter(), MININT);
		}
		else{
			scores.emplace(p->getCenter(), -thisDistance);
		}
	}


	scores.find(BWTA::getRegion(myBaseLocation)->getCenter())->second = 10000;

	for (auto j : jointPoints){
		double thisDistance = MapTools::Instance().getGroundDistance(myBaseLocation, j);
		if (thisDistance <= 0){
			scores.emplace(j, 50 + MININT);
		}
		else{
			scores.emplace(j, 50 - thisDistance);
		}
	}
}

void RegionManager::drawScores() {
	/*
	for (auto & s : scores){
		BWAPI::Broodwar->drawTextMap(s.first, "Score: %.2f", s.second);
	}
	*/
}


void RegionManager::valueUpdate() {

	//TODO: HighLand factor and distance percentage
	BWAPI::Position myBaseLocation = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	BWAPI::Position enemyBaseLocation = BWAPI::Position(BWAPI::Broodwar->enemy()->getStartLocation());
	double totalDistance = enemyBaseLocation.getDistance(myBaseLocation);

	for (auto u : BWAPI::Broodwar->getAllUnits()) {
		if (u == nullptr) {
			continue;
		}

		if (u->isAttackFrame()) {
			continue; 
		}

		if (u->isUnderAttack()){						
			//if scount keep the score, otherwise clear to initial
			if (BWTA::getRegion(u->getPosition()) == nullptr) {
				continue;
			}

			if (!BWTA::getRegion(u->getPosition())->isReachable(BWTA::getRegion(myBaseLocation))) {
				continue;
			}

			if (scores.find(BWTA::getRegion(u->getPosition())->getCenter()) != scores.end()) {
				double regionScore = scores.find(BWTA::getRegion(u->getPosition())->getCenter())->second;
				if (abs(regionScore) <= MAXINT) {
					//scores.find(BWTA::getRegion(u->getPosition())->getCenter())->second -= 4;
				}
			}
			
			for (auto nearbyChokepoints : BWTA::getRegion(u->getPosition())->getChokepoints()) {
				if (abs(scores.find(nearbyChokepoints->getCenter())->second) <= MAXINT) {
					//scores.find(nearbyChokepoints->getCenter())->second -= 1;
				}
			}
		}

		else{
			if (BWTA::getRegion(u->getPosition()) == nullptr) {
				continue;
			}

			if (!BWTA::getRegion(u->getPosition())->isReachable(BWTA::getRegion(myBaseLocation))) {
				continue;
			}

			if (scores.find(BWTA::getRegion(u->getPosition())->getCenter()) != scores.end()) {
				double regionScore = scores.find(BWTA::getRegion(u->getPosition())->getCenter())->second;
				double thisDistance = MapTools::Instance().getGroundDistance(myBaseLocation, BWTA::getRegion(u->getPosition())->getCenter());
				if (regionScore <= -thisDistance) {
					//scores.find(BWTA::getRegion(u->getPosition())->getCenter())->second += 2;
				}
			}

			for (auto nearbyChokepoints : BWTA::getRegion(u->getPosition())->getChokepoints()) {
				double thisDistance = MapTools::Instance().getGroundDistance(myBaseLocation, nearbyChokepoints->getCenter());
				if (scores.find(nearbyChokepoints->getCenter())->second <= -thisDistance + 50) {
					//scores.find(nearbyChokepoints->getCenter())->second += 0.5;
				}
			}
		}
	}
}


void RegionManager::drawPolygon(BWTA::Polygon p, BWAPI::Color color){
	for (int j = 0; j < (int)p.size(); j++) {
		BWAPI::Position point1 = p[j];
		BWAPI::Position point2 = p[(j + 1) % p.size()];
		BWAPI::Broodwar->drawLineMap(point1, point2, color);
	}

}


void RegionManager::drawAllPolygon(BWAPI::Color color){
	for (auto r = polyRegions.begin(); r != polyRegions.end(); r++) {
		BWTA::Polygon p = (*r)->getPolygon();
		drawPolygon(p, color);
	}
}

void RegionManager::drawUnwalkableRegion(){
	for (auto up : BWTA::getUnwalkablePolygons()){
		drawPolygon(*up, BWAPI::Colors::Yellow);
	}

}

void RegionManager::drawPlayerRegion(BWAPI::Player player, BWAPI::Color color){
	for (BWTA::Region * region : InformationManager::Instance().getOccupiedRegions(player)) {
		BWTA::Polygon p = region->getPolygon();
		drawPolygon(p, color);
	}
	
}

void RegionManager::drawJointPoints(){
	for (auto p : jointPoints) {
		BWAPI::Broodwar->drawCircleMap(p, 10, BWAPI::Colors::Orange, true);
	}
}

void findPolyRegions(BWAPI::Region region, std::set<BWTA::Region*> & polyRegionSet) {
	for (auto neighbor : region->getNeighbors()) {
		auto polyRegion = BWTA::getRegion(region->getCenter());
		if (polyRegion != nullptr && polyRegionSet.find(polyRegion) == polyRegionSet.end()) {
			polyRegionSet.emplace(polyRegion);
		}
	}
}

BWAPI::Position RegionManager::findOptimalPoint(std::set<BWAPI::Position> positions) {
	BWAPI::Position result;
	double maxscore = MININT;
	for (auto p : positions) {
		auto temp = scores.find(p);
		if (temp != scores.end()) {
			if (temp->second > maxscore) {
				maxscore = temp->second;
				result = temp->first;
			}
		}
	}
	return result;
}

BWAPI::Position RegionManager::getNextPointToBase(BWAPI::TilePosition tp) {
	return choke2CloserDefend[tp];
}

//std::map<BWAPI::TilePosition, double> RegionManager::distFromHm;
//std::map<BWAPI::TilePosition, BWAPI::TilePosition> RegionManager::choke2choke;
//std::map<BWAPI::TilePosition, std::vector<BWAPI::TilePosition>> RegionManager::choke2path;
std::pair<BWAPI::TilePosition, BWAPI::TilePosition> RegionManager::getExactRegroupPosition(BWAPI::TilePosition tp, double radius) {
//	auto nextChokePoint = choke2choke.find(tp)->second;
	std::pair<BWAPI::TilePosition, BWAPI::TilePosition> result(BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition(), BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition());
	if (choke2CloserDefend.find(tp) == choke2CloserDefend.end())
		return result;
	else
	{
		result.first = BWAPI::TilePosition(choke2CloserDefend[tp]);
		result.second = result.first;
		return result;
	}
}




