#include "Common.h"
#include "DecisionMaker.h"
#include "Evaluation.h"
#include "ScoutManager.h"

#ifndef __DEBUGGER_
#define __DEBUGGER_

namespace UAlbertaBot
{
	class Debugger
	{
	public:
		static Debugger & Instance();

		void draw_info();

		void resource_info();

		void people_info();

		void loss_info();

		void eval_info();

	private:
		Debugger();
		Debugger(const Debugger &){}

		int x;
		int y;
	};
}

#endif //__DEBUGGER