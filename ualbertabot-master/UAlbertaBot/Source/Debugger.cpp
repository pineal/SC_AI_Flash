#include "Debugger.h"

using namespace UAlbertaBot;
using namespace BWAPI;

Debugger::Debugger():x(0), y(0)
{
}



Debugger & Debugger::Instance()
{
	static Debugger instance;
	return instance;
}



void Debugger::draw_info()
{
	x = 300;
	y = 50;

	resource_info();

	people_info();

	loss_info();

	eval_info();

	auto base_map = ScoutManager::Instance().get_explore();
	for (auto iter : base_map)
	{
		Broodwar->drawCircleMap(iter.first->getPosition(), 50, BWAPI::Colors::Green, true);
		Broodwar->drawTextMap(iter.first->getPosition() - Position(10, 0),
			"priority : %d", iter.second);
	}
}



void Debugger::resource_info()
{
	Broodwar->drawTextScreen(x, y, "Resouce Info:");
	y += 10;

	int c_mineral = DecisionMaker::Instance().getStatistics().get_cur_mineral();
	int c_gas = DecisionMaker::Instance().getStatistics().get_cur_gas();
	Broodwar->drawTextScreen(x, y, "Current :\t%d\t%d", c_mineral, c_gas);
	y += 10;


	double s_gas = DecisionMaker::Instance().getStatistics().get_gas_speed();
	double s_mineral = DecisionMaker::Instance().getStatistics().get_mineral_speed();
	Broodwar->drawTextScreen(x, y, "Speed :\t%.2lf\t%.2lf", s_mineral, s_gas);
	y += 10;


	std::pair<int, int> tmp = DecisionMaker::Instance().getStatistics().get_ctrl_mineral();
	Broodwar->drawTextScreen(x, y, "Controled Mineral: %d\t%d",
		tmp.first, tmp.second);
	y += 10;


	y += 10;
}



void Debugger::people_info()
{
	Broodwar->drawTextScreen(x,y,"People Info:");
	y += 10;

	int supply = DecisionMaker::Instance().getStatistics().get_population();
	int supply_ttl = DecisionMaker::Instance().getStatistics().get_population_limit();
	Broodwar->drawTextScreen(x, y, "supply: %d/%d", supply, supply_ttl);
	y += 10;


	int tmp1 = DecisionMaker::Instance().getStatistics().get_num_comp(UnitTypes::Protoss_Probe);
	int tmp2 = DecisionMaker::Instance().getStatistics().get_num_uncomp(UnitTypes::Protoss_Probe);
	if (tmp1 || tmp2)
	{
		Broodwar->drawTextScreen(x, y, "Probe:\t%d\t%d", tmp1, tmp2);
		y += 10;
	}


	tmp1 = DecisionMaker::Instance().getStatistics().get_num_comp(UnitTypes::Protoss_Zealot);
	tmp2 = DecisionMaker::Instance().getStatistics().get_num_uncomp(UnitTypes::Protoss_Zealot);
	if (tmp1 || tmp2)
	{
		Broodwar->drawTextScreen(x, y, "Zealot :\t%d\t%d", tmp1, tmp2);
		y += 10;
	}


	tmp1 = DecisionMaker::Instance().getStatistics().get_num_comp(UnitTypes::Protoss_Dragoon);
	tmp2 = DecisionMaker::Instance().getStatistics().get_num_uncomp(UnitTypes::Protoss_Dragoon);
	if (tmp1 || tmp2)
	{
		Broodwar->drawTextScreen(x, y, "Dragoon :\t%d\t%d", tmp1, tmp2);
		y += 10;
	}


	tmp1 = DecisionMaker::Instance().getStatistics().get_num_comp(UnitTypes::Protoss_Carrier);
	tmp2 = DecisionMaker::Instance().getStatistics().get_num_uncomp(UnitTypes::Protoss_Carrier);
	if (tmp1 || tmp2)
	{
		Broodwar->drawTextScreen(x, y, "Carrier:\t%d\t%d", tmp1, tmp2);
		y += 10;
	}


	tmp1 = DecisionMaker::Instance().getStatistics().get_num_comp(UnitTypes::Protoss_Interceptor);
	tmp2 = DecisionMaker::Instance().getStatistics().get_num_uncomp(UnitTypes::Protoss_Interceptor);
	if (tmp1 || tmp2)
	{
		Broodwar->drawTextScreen(x, y, "Interceptor:\t%d\t%d", tmp1, tmp2);
		y += 10;
	}


	tmp1 = DecisionMaker::Instance().getStatistics().get_num_comp(UnitTypes::Protoss_Corsair);
	tmp2 = DecisionMaker::Instance().getStatistics().get_num_uncomp(UnitTypes::Protoss_Corsair);
	if (tmp1 || tmp2)
	{
		Broodwar->drawTextScreen(x, y, "Corsair:\t%d\t%d", tmp1, tmp2);
		y += 10;
	}


	y += 10;
}



void Debugger::loss_info()
{
	Broodwar->drawTextScreen(x, y, "Loss Info:");
	y += 10;

	loss tmp = DecisionMaker::Instance().getStatistics().self_loss();

	Broodwar->drawTextScreen(x, y,
		"My loss :\t%d mineral\t%d gas\t%.1lf ppl\t%d unit\t%d building",
		tmp.mineral_price,
		tmp.gas_price,
		tmp.population,
		tmp.unit,
		tmp.building);
	y += 10;


	tmp = DecisionMaker::Instance().getStatistics().enemy_loss();
	Broodwar->drawTextScreen(x, y,
		"His loss:\t%d mineral\t%d gas\t%.1lf ppl\t%d unit\t%d building",
		tmp.mineral_price,
		tmp.gas_price,
		tmp.population,
		tmp.unit,
		tmp.building);
	y += 10;

	y += 10;
}


void Debugger::eval_info()
{
	Broodwar->drawTextScreen(x, y, "Evaluation Info:");
	y += 10;

	double my_str = Evaluation::Instance().eval_self_in_double();
	double my_log_str = Evaluation::Instance().eval_self_log();
	Broodwar->drawTextScreen(x, y,
		"Self army strength: ori %.2lf\tlog %.2lf", my_str, my_log_str);
	y += 10;

	double his_str = Evaluation::Instance().eval_enemy_in_double();
	double his_log_str = Evaluation::Instance().eval_enemy_log();
	Broodwar->drawTextScreen(x, y,
		"Enemy army strength: ori %.2lf\tlog %.2lf", his_str, his_log_str);
	y += 10;

	double s_zealot = Evaluation::Instance().eval_soldier(UnitTypes::Protoss_Zealot);
	double s_dragoon = Evaluation::Instance().eval_soldier(UnitTypes::Protoss_Dragoon);
	Broodwar->drawTextScreen(x,y,"Score for zealot: %.2lf \t score for dragoon: %.2lf",
		s_zealot, s_dragoon);
	y += 10;
	/*
	UnitType z = UnitTypes::Protoss_Zealot;
	UnitType d = UnitTypes::Protoss_Dragoon;
	WeaponType w = d.airWeapon();
	WeaponType e = d.groundWeapon();
	WeaponType q = z.groundWeapon();

	double dps1 = d.maxAirHits() * w.damageAmount() * 1.0 / w.damageCooldown();
	double dps2 = d.maxGroundHits() * e.damageAmount() * 1.0 / e.damageCooldown();
	double dps3 = z.maxGroundHits() * q.damageAmount() * 1.0 / q.damageCooldown();

	//0.67 0.67 0.73
	Broodwar->drawTextScreen(x,y,"%.2lf\t%.2lf\t%.2lf",dps1,dps2,dps3);
	y += 10;
	*/
}

