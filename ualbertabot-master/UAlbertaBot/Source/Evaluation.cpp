#include "Evaluation.h"
#include "DecisionMaker.h"

using namespace UAlbertaBot;
using namespace std;
using namespace BWAPI;
using namespace BWAPI::UnitTypes::Enum;

Evaluation & Evaluation::Instance()
{
	static Evaluation instance;
	return instance;
}


void Evaluation::single_eval(Player p, UnitType type, int hp)
{
	eval * eval_ptr = NULL;
	if (p == Broodwar->self())
	{
		eval_ptr = &self;
	}
	else if (p == Broodwar->enemy())
	{
		eval_ptr = &enemy;
	}

	assert(eval_ptr != 0);

	//calc hit points
	if (type.isFlyer())
	{
		eval_ptr->air_hp += hp;
	}
	else
	{
		eval_ptr->ground_hp += hp;
	}

	//calc population and unit count
	if (type != Protoss_Interceptor)
	{
		eval_ptr->population += type.supplyRequired() / 2.0;
		eval_ptr->unit += 1;
	}

	if (type == UnitTypes::Protoss_Carrier)
	{
		return;
	}

	//calc damage
	double dps_g = 0;
	double dps_a = 0;

	WeaponType w_g = type.groundWeapon();
	if (w_g != WeaponTypes::None)
	{
		int dmg = p->damage(w_g);
		dmg *= type.maxGroundHits();
		dps_g = dmg * 1.0 / w_g.damageCooldown();
	}

	WeaponType w_a = type.airWeapon();
	if (w_a != WeaponTypes::None)
	{
		int dmg = p->damage(w_a);
		dmg *= type.maxAirHits();
		dps_a = dmg * 1.0 / w_a.damageCooldown();
	}

	double discount = hp / (type.maxHitPoints() + type.maxShields() + 0.0);
	discount = sqrt(discount);
	if (type == Protoss_Interceptor)
	{
		discount = 1.0;
	}


	if (dps_a != 0 && dps_g != 0)
	{
		eval_ptr->max_2a_dps += dps_a * discount;
		eval_ptr->max_2g_dps += dps_g * discount;

		eval_ptr->max_2a += dps_a * hp;
		eval_ptr->max_2g += dps_g * hp;
	}
	else if (dps_a != 0 && dps_g == 0)
	{
		eval_ptr->max_2a_dps += dps_a * discount;
		eval_ptr->min_2a_dps += dps_a * discount;

		eval_ptr->max_2a += dps_a * hp;
		eval_ptr->min_2a += dps_a * hp;
	}
	else if (dps_a == 0 && dps_g != 0)
	{
		eval_ptr->max_2g_dps += dps_g * discount;
		eval_ptr->min_2g_dps += dps_g * discount;

		eval_ptr->max_2g += dps_g * hp;
		eval_ptr->min_2g += dps_g * hp;
	}
}


void Evaluation::calculate_self()
{
	self.clear();
	vector<int> & ret1 =
		DecisionMaker::Instance().getStatistics().get_hp(UnitTypes::Protoss_Zealot);

	for (int iter : ret1)
	{
		single_eval(Broodwar->self(), UnitTypes::Protoss_Zealot, iter);
	}

	vector<int> & ret2 =
		DecisionMaker::Instance().getStatistics().get_hp(UnitTypes::Protoss_Dragoon);

	for (int iter : ret2)
	{
		single_eval(Broodwar->self(), UnitTypes::Protoss_Dragoon, iter);
	}

	vector<int> & ret3 =
		DecisionMaker::Instance().getStatistics().get_hp(UnitTypes::Protoss_Carrier);

	for (int iter : ret3)
	{
		single_eval(Broodwar->self(), UnitTypes::Protoss_Carrier, iter);
	}

	vector<int> & ret4 =
		DecisionMaker::Instance().getStatistics().get_hp(UnitTypes::Protoss_Interceptor);

	for (int iter : ret4)
	{
		single_eval(Broodwar->self(), UnitTypes::Protoss_Interceptor, 0);
	}

	vector<int> & ret5 =
		DecisionMaker::Instance().getStatistics().get_hp(UnitTypes::Protoss_Corsair);

	for (int iter : ret5)
	{
		single_eval(Broodwar->self(), UnitTypes::Protoss_Corsair, iter);
	}
}


void Evaluation::calculate_enemy()
{
	enemy.clear();
	map<int, unit_info> enemy_info = DecisionMaker::Instance().getStatistics().enemy();

	for (auto iter : enemy_info)
	{
		UnitType t = iter.second.type;
		int hp = iter.second.hit_points;
		switch (t)
		{
			//invisible unit
		case Protoss_Dark_Templar:
			hp = t.maxHitPoints() + t.maxShields();

			//protoss ground unit
		case Protoss_Zealot:
		case Protoss_Dragoon:
		
		case Protoss_Archon:

			//terran ground unit
		case Terran_Marine:
		case Terran_Firebat:
		case Terran_Ghost:
		case Terran_Vulture:
		case Terran_Siege_Tank_Tank_Mode:
		case Terran_Siege_Tank_Siege_Mode:
		case Terran_Goliath:

			//zerg ground unit
		case Zerg_Zergling:
		case Zerg_Hydralisk:
		case Zerg_Ultralisk:

			//protess air unit
		case Protoss_Scout:
		case Protoss_Arbiter:
		case Protoss_Corsair:

			//terran air unit
		case Terran_Wraith:
		case Terran_Battlecruiser:
		case Terran_Valkyrie:

			//zerg air unit
		case Zerg_Mutalisk:
		case Zerg_Guardian:
		case Zerg_Devourer:
		case Zerg_Scourge:

			single_eval(Broodwar->enemy(), t, hp);
			break;

		default:
			break;
		}
	}
}


void Evaluation::cross_calculate()
{
	int hp_a1 = self.air_hp;
	int hp_g1 = self.ground_hp;

	int hp_a2 = enemy.air_hp;
	int hp_g2 = enemy.ground_hp;

	//if enemy only have ground units or we do not detect [any enemy units]
	if (hp_a2 == 0)
	{
		self_strength = self.max_2g_dps * sqrt(hp_a1 + hp_g1 + 1.0);
		self_wdp = self.max_2g * log(self.unit + 1.0);
	}
	//if enemy only have air units
	else if (hp_g2 == 0)
	{
		self_strength = self.max_2a_dps * sqrt(hp_a1 + hp_g1 + 1.0);
		self_wdp = self.max_2a * log(self.unit + 1.0);
	}
	//if enemy has both air and ground units
	else
	{
		self_strength = (self.min_2g_dps + self.max_2a_dps) * sqrt(hp_a1 + hp_g1 + 1.0);
		self_wdp = (self.min_2g + self.max_2a) * log(self.unit + 1.0);
	}

	//if we only have ground units or we do not have any units
	if (hp_a1 == 0)
	{
		enemy_strength = enemy.max_2g_dps * sqrt(hp_a2 + hp_g2 + 1.0);
		enemy_wdp = enemy.max_2g * log(enemy.unit + 1.0);
	}
	//if we only have air units
	else if (hp_g1 == 0)
	{
		enemy_strength = enemy.max_2a_dps * sqrt(hp_a2 + hp_g2 + 1.0);
		enemy_wdp = enemy.max_2a * log(enemy.unit + 1.0);
	}
	//if we have both air and ground units
	else
	{
		enemy_strength = (enemy.min_2g_dps + enemy.max_2a_dps) * sqrt(hp_a2 + hp_g2 + 1.0);
		enemy_wdp = (enemy.min_2g + enemy.max_2a)*log(enemy.unit + 1.0);
	}
}


void Evaluation::calculate()
{
	int cur_count = Broodwar->getFrameCount();
	if (cur_count - frame_count <= 5)
	{
		return;
	}

	calculate_self();
	calculate_enemy();
	cross_calculate();
	frame_count = cur_count;
}



double Evaluation::eval_self_in_double()
{
	calculate();
	return self_strength;
}


double Evaluation::eval_enemy_in_double()
{
	calculate();
	return enemy_strength;
}

double Evaluation::eval_self_log()
{
	calculate();
	return self_wdp;
}

double Evaluation::eval_enemy_log()
{
	calculate();
	return enemy_wdp;
}

double Evaluation::eval_soldier(UnitType type)
{
	calculate();

	double z_points = 3.0;
	double d_points = 4.0;

	int arg1 = enemy.air_hp;
	int arg2 = enemy.ground_hp;
	double air_bonus = (arg1 + 0.0) / (arg1 + arg2 + 1.0);
	d_points += 5.0 * air_bonus;

	double arg3 = enemy.min_2g_dps;
	double arg4 = enemy.max_2a_dps;
	double range_bonus = (arg4 + 0.0) / (arg3 + arg4 + 1.0);
	d_points += 3.0 * range_bonus;

	int z_num = DecisionMaker::Instance().getStatistics().get_num_comp(Protoss_Zealot);
	z_num += DecisionMaker::Instance().getStatistics().get_num_uncomp(Protoss_Zealot);

	int d_num = DecisionMaker::Instance().getStatistics().get_num_comp(Protoss_Dragoon);
	d_num += DecisionMaker::Instance().getStatistics().get_num_uncomp(Protoss_Dragoon);

	double desire_z = d_num * z_points / d_points - z_num;
	double desire_d = z_num * d_points / z_points - d_num;

	if (type == UnitTypes::Protoss_Zealot)
	{
		return desire_z;
	}
	else if (type == UnitTypes::Protoss_Dragoon)
	{
		return desire_d;
	}
	return 1.0;
}
