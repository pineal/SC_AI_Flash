#pragma once

#include <Common.h>
#include <BWAPI.h>
#include "RegionManager.h"

namespace UAlbertaBot
{
	struct PairEdge;
	struct PairEdge
	{
		BWAPI::Unit attacker;
		BWAPI::Unit target;
		double distance;
		bool operator < (const PairEdge& that) const;
	};
namespace Micro
{

    void SmartAttackUnit(BWAPI::Unit attacker, BWAPI::Unit target);
    void SmartAttackMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition);
    void SmartMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition,bool step = false);
    void SmartRightClick(BWAPI::Unit unit, BWAPI::Unit target);
    void SmartLaySpiderMine(BWAPI::Unit unit, BWAPI::Position pos);
    void SmartRepair(BWAPI::Unit unit, BWAPI::Unit target);
	void SmartKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target, BWAPI::Unit dodge, BWAPI::Unitset& targets);
	void SmartKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target);
	void SmarterKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target,BWAPI::Unit closest,const BWAPI::Unitset& targets,BWAPI::Unit attackee);


	void MutaDanceTarget(BWAPI::Unit muta, BWAPI::Unit target);

	BWAPI::Position GetKiteVector(BWAPI::Unit unit, BWAPI::Unit target, int range, const BWAPI::Unitset& targets);
	BWAPI::Position GetKiteVector(BWAPI::Unit unit, BWAPI::Unit target, int range);

	BWAPI::Position Rotate(BWAPI::Position old, double angle);
	void Normalize(double &x, double &y);
	bool ValidPath(BWAPI::Position, BWAPI::Position);
	bool CheckBuilding(BWAPI::Position pos, BWAPI::UnitType tp);
	bool RealWalkable(BWAPI::Position pos, BWAPI::UnitType tp);
    void drawAPM(int x, int y);

	bool DamageEvaluation(const BWAPI::Unitset & attackers, const BWAPI::Unitset & enemies);
	double getGroupDamages(const BWAPI::Unitset & units);
	double getGroupHealth(const BWAPI::Unitset & units);

};
}