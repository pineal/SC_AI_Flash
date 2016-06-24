#pragma once

#include "Common.h"
#include "MapGrid.h"
#include "SquadOrder.h"
#include "MapTools.h"
#include "InformationManager.h"
#include "Micro.h"
#include "DecisionMaker.h"
#include "MicroDecision.h"
#include "RegionManager.h"

namespace UAlbertaBot
{
struct AirThreat
{
	BWAPI::Unit	unit;
	double			weight;
};

struct GroundThreat
{
	BWAPI::Unit	unit;
	double			weight;
};

class MicroManager
{
	BWAPI::Unitset  _units;
	BWAPI::Unitset nearbyEnemies;
	bool _formation;

protected:
	
	SquadOrder			order;
	std::string         SquadName;

	virtual void        executeMicro(const BWAPI::Unitset & targets) = 0;
	bool                checkPositionWalkable(BWAPI::Position pos);
	void                drawOrderText();
	bool                unitNearEnemy(BWAPI::Unit unit) const;
	bool                unitNearChokepoint(BWAPI::Unit unit) const;
	void                trainSubUnits(BWAPI::Unit unit) const;
	BWAPI::Position positionShift(const BWAPI::Position &from, const BWAPI::Position &to, int distance);

public:
	bool				formed;
	bool				fighting;

	MicroManager() ;
    virtual				~MicroManager(){}

	const BWAPI::Unitset & getUnits() const;
	bool getFormation();
	bool getFighting();
	bool formSquad(const BWAPI::Unitset & targets, int dist, int radius, double angle, int interval);
	bool regroupForm(const BWAPI::Position & p, int dist, int radius, double angle, int interval);
	BWAPI::Position     calcCenter() const;

	void				setUnits(const BWAPI::Unitset & u);
	void				setFormation(bool f);
	void				execute(const SquadOrder & order, std::string inputSquadName);
	virtual bool		regroup(const BWAPI::Position & regroupPosition, int dist = 32 * 2, int interval = 30, int radius = 32 * 6, double angle = 150);
	void				regroup2(const BWAPI::Position & regroupPosition) const;
	void				regroup1(const BWAPI::Position & regroupPosition) const;
	MicroDecison        * _microDecision;
};
}