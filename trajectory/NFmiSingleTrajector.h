#pragma once

#include <newbase/NFmiDataMatrix.h>
#include <newbase/NFmiMetTime.h>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiProducer.h>

// pluumi eli yksi yksittäisen partikkelin eteneminen
// tähän talletetaan vain trajectorin pisteet ja alku aika ja paikka.
// Muut tiedot ovat ylempänä eli NFmiTrajectory-luokassa
class NFmiSingleTrajector
{
 public:
  NFmiSingleTrajector();
  NFmiSingleTrajector(const NFmiPoint &theLatLon,
                      const NFmiMetTime &theTime,
                      double thePressureLevel);
  ~NFmiSingleTrajector();

  std::string ToXMLStr(int TimeStepInMinutes, const NFmiProducer& Producer, FmiDirection Direction);
  const std::vector<NFmiPoint>& Points() const { return itsPoints; }
  const std::vector<float>& Pressures() const { return itsPressures; }
  const std::vector<float>& HeightValues() const { return itsHeightValues; }
  const NFmiPoint& StartLatLon() const { return itsStartLatLon; }
  void StartLatLon(const NFmiPoint &newValue) { itsStartLatLon = newValue; }
  const NFmiMetTime& StartTime() const { return itsStartTime; }
  void StartTime(const NFmiMetTime &newValue) { itsStartTime = newValue; }
  void ClearPoints();
  void AddPoint(const NFmiPoint &theLatLon);
  void AddPoint(const NFmiPoint &theLatLon, float thePressure, float theHeightValue);
  double StartPressureLevel() const { return itsStartPressureLevel; }
  void StartPressureLevel(double newValue) { itsStartPressureLevel = newValue; }
  bool Is3DTrajectory() const;
  double IsentropicTpotValue() const { return itsIsentropicTpotValue; }
  void IsentropicTpotValue(double newValue) { itsIsentropicTpotValue = newValue; }
  void SetRandomValues(double theWD, double theWS, double thew);
  double RandWSdiff() const { return itsRandWSdiff; }
  double RandWDdiff() const { return itsRandWDdiff; }
  double Randwdiff() const { return itsRandwdiff; }

 private:
  std::vector<NFmiPoint> itsPoints;
  std::vector<float> itsPressures;
  std::vector<float> itsHeightValues;
  NFmiPoint itsStartLatLon;
  NFmiMetTime itsStartTime;
  double itsStartPressureLevel;   // yks. hPa
  double itsIsentropicTpotValue;  // kun lasketaan trajektoreita, tähän talletetaan haluttu
                                  // potentiaali lämpötila, mitä on käytetty

  double itsRandWSdiff;  // satunnainen WS muutos
  double itsRandWDdiff;  // satunnainen WD muutos
  double itsRandwdiff;   // satunnainen w muutos
};
