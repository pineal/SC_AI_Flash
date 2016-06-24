#include "Statistics.h"

using namespace std;
using namespace BWAPI;
using namespace UAlbertaBot;



Statistics::Statistics() :
	prev_mineral(1, 50), prev_gas(1, 0),
	rolling_size(3600), frame_counter(0),
	mineral_speed(0), gas_speed(0)
{
	prev_mineral.reserve(rolling_size);
	prev_gas.reserve(rolling_size);
}



int Statistics::get_cur_mineral()
{
	return Broodwar->self()->minerals();
}



int Statistics::get_cur_gas()
{
	return Broodwar->self()->gas();
}



int Statistics::get_population()
{
	return Broodwar->self()->supplyUsed()/2;
}



int Statistics::get_population_limit()
{
	return Broodwar->self()->supplyTotal()/2;
}



void Statistics::update_self()
{
	_people.clear();
	_people_1.clear();
	_people_2.clear();

	_house.clear();
	_house_1.clear();

	_bases.clear();

	Unitset me = BWAPI::Broodwar->self()->getUnits();
	for (auto & iter : me)
	{
		UnitType t = iter->getType();
		switch (t)
		{
		case UnitTypes::Enum::Protoss_Probe:
		case UnitTypes::Enum::Protoss_Zealot:
		case UnitTypes::Enum::Protoss_Dragoon:
		case UnitTypes::Enum::Protoss_Carrier:
		case UnitTypes::Enum::Protoss_Interceptor:
		case UnitTypes::Enum::Protoss_Corsair:
			if (iter->isCompleted())
			{
				_people[t].push_back(iter->getHitPoints() + iter->getShields());
			}
			else if (iter->isBeingConstructed())
			{
				_people_1[t]++;
			}
			else
			{
				_people_2[t]++;
			}
			break;

		case UnitTypes::Enum::Protoss_Nexus:
			if (iter->isCompleted())
			{
				_bases.insert(iter);
			}

		case UnitTypes::Enum::Protoss_Pylon:
		case UnitTypes::Enum::Protoss_Gateway:
		case UnitTypes::Enum::Protoss_Assimilator:
		case UnitTypes::Enum::Protoss_Cybernetics_Core:
			if (iter->isCompleted())
			{
				_house[t].push_back(iter->getHitPoints() + iter->getShields());
			}
			else
			{
				_house_1[t]++;
			}
			break;

		default:
			break;
		}
	}

	
	//current frame count
	int fc = Broodwar->getFrameCount();

	//Store the cumulative amount of minerals and gas.
	//For calculating the gathering speed of minerals and gas .
	if (fc - frame_counter >= 24)
	{
		int sofar_mineral = Broodwar->self()->gatheredMinerals();
		int sofar_gas = Broodwar->self()->gatheredGas();
		
		int s = prev_mineral.size();

		if (s < rolling_size)
		{
			prev_mineral.push_back(sofar_mineral);
			prev_gas.push_back(sofar_gas);
		}
		else
		{
			prev_mineral.erase(prev_mineral.begin());
			prev_mineral.push_back(sofar_mineral);

			prev_gas.erase(prev_gas.begin());
			prev_gas.push_back(sofar_gas);
		}

		frame_counter = fc;
	}

}//end of Statistics::update_self()



void Statistics::update_neutral()
{
	ctrl_mineral.clear();

	Unitset it = Broodwar->neutral()->getUnits();
	for (auto iter : it)
	{
		UnitType i_type = iter->getType();
		if (i_type == UnitTypes::Resource_Mineral_Field ||
			i_type == UnitTypes::Resource_Mineral_Field_Type_2 ||
			i_type == UnitTypes::Resource_Mineral_Field_Type_3)
		{
			bool near = false;
			for (auto base : _bases)
			{
				int dist = iter->getDistance(base->getPosition());
				if (dist <= 300)
				{
					near = true;
					break;
				}
			}
			if (!near)
			{
				continue;
			}
			ctrl_mineral[iter->getRegion()->getID()].push_back(iter->getResources());
		}
	}
}// end of Statistics::update_neutral()



void Statistics::update_enemy()
{
	//if there is no visible enemy
	//return
	int new_count = Broodwar->enemy()->allUnitCount();
	if (new_count == 0)
	{
		return;
	}

	Unitset he = Broodwar->enemy()->getUnits();

	for (auto iter : he)
	{
		if (!iter->exists())
		{
			continue;
		}
		if (iter->getType().isCloakable())
		{
			//Broodwar->sendText("find invisible unit");
			enemy_info[iter->getID()].hit_points = iter->getType().maxHitPoints() + iter->getType().maxShields();
		}
		else
		{
			enemy_info[iter->getID()].hit_points = iter->getHitPoints() + iter->getShields();
		}
		enemy_info[iter->getID()].type = iter->getType();
	}
		
}//end of Statistics::update_enemy()



void Statistics::on_frame()
{
	update_self();

	update_neutral();
	
	update_enemy();

}



int Statistics::get_num_comp(UnitType type)
{
	if (type.isBuilding())
	{
		return _house[type].size();
	}
	else
	{
		return _people[type].size();
	}
}



vector<int> & Statistics::get_hp(UnitType type)
{
	if (type.isBuilding())
	{
		return _house[type];
	}
	else
	{
		return _people[type];
	}
}



int Statistics::get_num_uncomp(UnitType type)
{
	if (type.isBuilding())
	{
		return _house_1[type];
	}
	else
	{
		return _people_1[type] + _people_2[type];
	}
}



double Statistics::get_mineral_speed(int last_n_sec)
{
	int cur = prev_mineral.size() - 1;
	int window = min(last_n_sec, cur);
	double mineral = prev_mineral[cur] - prev_mineral[cur - window];
	return mineral / window;
}



double Statistics::get_gas_speed(int last_n_sec)
{
	int cur = prev_gas.size() - 1;
	int window = min(last_n_sec,  cur);
	double gas = prev_gas[cur] - prev_gas[cur - window];
	return gas/window;
}



map<int, vector<int> > Statistics::get_ctrl_mineral_by_region()
{
	return ctrl_mineral;
}



pair<int, int> Statistics::get_ctrl_mineral()
{
	int first = 0;
	int second = 0;
	for (auto iter : ctrl_mineral)
	{
		for (auto iiter : iter.second)
		{
			first++;
			second += iiter;
		}
	}
	return pair<int, int>(first,second);
}



void Statistics::on_unit_destroy(Unit u)
{
	if (u->getPlayer() == Broodwar->self())
	{
		my_loss.mineral_price += u->getType().mineralPrice();
		my_loss.gas_price += u->getType().gasPrice();
		if (u->getType().isBuilding())
		{
			my_loss.building++;
		}
		else
		{
			my_loss.population += u->getType().supplyRequired() / 2;
			my_loss.unit += 1;
		}
	}
	else if (u->getPlayer() == Broodwar->enemy())
	{
		his_loss.mineral_price += u->getType().mineralPrice();
		his_loss.gas_price += u->getType().gasPrice();
		if (u->getType().isBuilding())
		{
			his_loss.building++;
		}
		else
		{
			his_loss.population += u->getType().supplyRequired() / 2.0;
			his_loss.unit += 1;
		}

		auto pos = enemy_info.find(u->getID());
		if (pos != enemy_info.end())
		{
			enemy_info.erase(pos);
			//Broodwar->sendText("Found and erased");
		}
		else
		{
			//Broodwar->sendText("Not found");
		}
	}
}



loss Statistics::self_loss()
{
	return my_loss;
}



loss Statistics::enemy_loss()
{
	return his_loss;
}



map<int, unit_info> Statistics::enemy()
{
	return enemy_info;
}


BWAPI::Unitset Statistics::get_bases()
{
	return _bases;
}