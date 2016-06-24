#pragma once

class MicroDecison
{
private:
	int DragoonMoveBackDist=0;
	//if Dist to enmey Less than DragoonMoveBackDist, pull Dragoon
	int saveZealotLess=0;
	// save zealot without shiled and HP less than this
	double flightOrientatedFactor = 1.0;
	// whether to focus on flight defender
public:
	int getDragoonMoveBack() const
	{
		return DragoonMoveBackDist;
	}
	int getSaveZealotLess() const 
	{
		return saveZealotLess;
	}
	double getFlightOrientatedFactor() const
	{
		return flightOrientatedFactor;
	}
	void setFlightOrientatedFactor(double val)
	{
		flightOrientatedFactor = 1.0;
	}
	void setSaveZealotLess(int val)
	{
		saveZealotLess = val;
	} 
	void setDragoonMoveBack(int val)
	{
		DragoonMoveBackDist = val;
	}
};