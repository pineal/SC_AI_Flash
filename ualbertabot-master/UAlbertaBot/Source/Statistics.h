#include "Common.h"
#pragma once
namespace UAlbertaBot
{
	struct loss
	{
		int mineral_price;
		int gas_price;
		double population;
		int unit;
		int building;

		loss() :mineral_price(0), gas_price(0),
			population(0), unit(0), building(0)
		{}
	};

	struct unit_info
	{
		int hit_points;
		BWAPI::UnitType type;
	};

	class Statistics
	{
		std::map<BWAPI::UnitType, std::vector<int> > _people;
		std::map<BWAPI::UnitType, int> _people_1;
		std::map<BWAPI::UnitType, int> _people_2;

		std::map<BWAPI::UnitType, std::vector<int> > _house;
		std::map<BWAPI::UnitType, int> _house_1;

		std::vector<int> prev_mineral;
		std::vector<int> prev_gas;
		double mineral_speed;
		double gas_speed;

		int rolling_size;
		int frame_counter;

		std::map<int, std::vector<int> > ctrl_mineral;
		BWAPI::Unitset _bases;

		loss my_loss;
		loss his_loss;

		std::map<int, unit_info> enemy_info;

		Statistics(const Statistics &){}
		
		
		void update_self();

		void update_neutral();

		void update_enemy();

	public:
		Statistics();

		//get current mineral
		int get_cur_mineral();

		//get current gas
		int get_cur_gas();

		//get amount of people
		int get_population();

		//get limitation for amount of people
		int get_population_limit();

		//update information per frame
		void on_frame();

		//update loss
		void on_unit_destroy(BWAPI::Unit u);

		//get completed number of a protoss type
		int get_num_comp(BWAPI::UnitType type);

		//get hit points of a protoss non-building type
		std::vector<int> & get_hp(BWAPI::UnitType type);

		//get uncompleted number of a protoss type
		int get_num_uncomp(BWAPI::UnitType type);


		//return mineral gathering speed for last N seconds.
		//N is default to 30
		double get_mineral_speed(int last_n_seconds = 30);


		//return gas gathering speed for last N seconds.
		//N is default to 30
		double get_gas_speed(int last_n_seconds = 30);

		
		//get info of minerals that are under control
		//first int : the number of mineral
		//second int : the remaining amount of mineral in total
		std::pair<int, int> get_ctrl_mineral();

		//get info of minerals that are under control
		//grouped by region
		//vector stores the remaining amount of each mineral in that region
		std::map<int,std::vector<int> > get_ctrl_mineral_by_region();


		//calculate the loss of player
		//return struct loss
		loss self_loss();


		//calculate the loss of enemy
		//return struct loss
		loss enemy_loss();


		//get enemy info
		std::map<int, unit_info> enemy();

		BWAPI::Unitset get_bases();

	};
}