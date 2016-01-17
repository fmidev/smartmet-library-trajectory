
#include "NFmiSingleTrajector.h"

//_________________________________________________________ NFmiSingleTrajector
NFmiSingleTrajector::NFmiSingleTrajector(void)
    : itsPoints(),
      itsPressures(),
      itsHeightValues(),
      itsStartLatLon(),
      itsStartTime(),
      itsStartPressureLevel(850),
      itsRandWSdiff(0),
      itsRandWDdiff(0),
      itsRandwdiff(0)
{
}

NFmiSingleTrajector::NFmiSingleTrajector(const NFmiPoint &theLatLon,
                                         const NFmiMetTime &theTime,
                                         double thePressureLevel)
    : itsPoints(),
      itsPressures(),
      itsHeightValues(),
      itsStartLatLon(theLatLon),
      itsStartTime(theTime),
      itsStartPressureLevel(thePressureLevel),
      itsRandWSdiff(0),
      itsRandWDdiff(0),
      itsRandwdiff(0)
{
}

NFmiSingleTrajector::~NFmiSingleTrajector(void) {}
void NFmiSingleTrajector::ClearPoints(void)
{
  itsPoints.clear();
  itsPressures.clear();
}

void NFmiSingleTrajector::AddPoint(const NFmiPoint &theLatLon) { itsPoints.push_back(theLatLon); }
void NFmiSingleTrajector::AddPoint(const NFmiPoint &theLatLon,
                                   float thePressure,
                                   float theHeightValue)
{
  itsPoints.push_back(theLatLon);
  itsPressures.push_back(thePressure);
  itsHeightValues.push_back(theHeightValue);
}

bool NFmiSingleTrajector::Is3DTrajectory(void) const
{
  if (!itsPoints.empty())
    if (itsPoints.size() == itsPressures.size()) return true;
  return false;
}

void NFmiSingleTrajector::SetRandomValues(double theWD, double theWS, double thew)
{
  itsRandWSdiff = theWS;
  itsRandWDdiff = theWD;
  itsRandwdiff = thew;
}
