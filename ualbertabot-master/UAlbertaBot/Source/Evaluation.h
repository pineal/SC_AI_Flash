#ifndef __EVAL_H__
#define __EVAL_H__

#include "Common.h"

namespace UAlbertaBot
{
	struct eval
	{
		double population;
		int unit;

		int air_hp;
		int ground_hp;

		double max_2a_dps;
		double min_2a_dps;

		double max_2g_dps;
		double min_2g_dps;

		double max_2a;
		double min_2a;

		double max_2g;
		double min_2g;

		eval() :
			population(0), unit(0),
			air_hp(0), ground_hp(0),
			max_2a_dps(0), min_2a_dps(0),
			max_2g_dps(0), min_2g_dps(0),
			max_2a(0), min_2a(0),
			max_2g(0), min_2g(0)
		{}

		void clear()
		{
			population = 0; unit = 0;
			air_hp = 0; ground_hp = 0;
			max_2a_dps = 0; min_2a_dps = 0;
			max_2g_dps = 0; min_2g_dps = 0;
			max_2a = 0; min_2a = 0;
			max_2g = 0; min_2g = 0;
		}
	};

	class Evaluation
	{
	public:
		static Evaluation & Instance();

		double eval_self_in_double();
		double eval_enemy_in_double();

		double eval_self_log();
		double eval_enemy_log();


		double eval_soldier(BWAPI::UnitType t);

	private:
		Evaluation() : frame_count(0) {}
		Evaluation(const Evaluation &){}

		int frame_count;

		double self_strength;
		double enemy_strength;

		double self_wdp;
		double enemy_wdp;

		eval self;
		eval enemy;

		void calculate();

		void calculate_self();
		void calculate_enemy();
		void cross_calculate();

		void single_eval(BWAPI::Player p, BWAPI::UnitType t, int hp);
	};

}

#endif //__EVAL_H__