#pragma once

#include "NFmiSingleTrajector.h"
#include "NFmiTempBalloonTrajectorSettings.h"
#include <newbase/NFmiMetTime.h>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiProducer.h>
#include <boost/shared_ptr.hpp>

// trajektori voi pitää sisällään yhdestä useampaan kpl yksittäisiä
// trajektoreita. Yksi pää trajektory ja ns. satunnais pluumi, jossa useita partikkeleita.
class NFmiTrajectory
{
 public:
  NFmiTrajectory(void);
  NFmiTrajectory(const NFmiPoint& theLatLon,
                 const NFmiMetTime& theTime,
                 const NFmiProducer& theProducer,
                 int theDataType,
                 int theTimeStepInMinutes,
                 int theTimeLengthInHours);
  ~NFmiTrajectory(void);

  void Clear(void);
  std::string ToXMLStr(void);
  const NFmiPoint& LatLon(void) const { return itsLatLon; }
  void LatLon(const NFmiPoint& newValue) { itsLatLon = newValue; }
  const NFmiMetTime& Time(void) const { return itsTime; }
  void Time(const NFmiMetTime& newValue) { itsTime = newValue; }
  const NFmiProducer& Producer(void) const { return itsProducer; }
  void Producer(const NFmiProducer& newValue) { itsProducer = newValue; }
  int DataType(void) const { return itsDataType; }
  void DataType(int newValue) { itsDataType = newValue; }
  int TimeStepInMinutes(void) const { return itsTimeStepInMinutes; }
  void TimeStepInMinutes(int newValue) { itsTimeStepInMinutes = newValue; }
  int TimeLengthInHours(void) const { return itsTimeLengthInHours; }
  void TimeLengthInHours(int newValue) { itsTimeLengthInHours = newValue; }
  const NFmiSingleTrajector& MainTrajector(void) const { return itsMainTrajector; }
  void MainTrajector(const NFmiSingleTrajector& theTrajector) { itsMainTrajector = theTrajector; }
  void AddPlumeTrajector(boost::shared_ptr<NFmiSingleTrajector>& theTrajector);
  const checkedVector<boost::shared_ptr<NFmiSingleTrajector> >& PlumeTrajectories(void) const
  {
    return itsPlumeTrajectories;
  }
  double PlumeProbFactor(void) const { return itsPlumeProbFactor; }
  void PlumeProbFactor(double newValue) { itsPlumeProbFactor = newValue; }
  int PlumeParticleCount(void) const { return itsPlumeParticleCount; }
  void PlumeParticleCount(int newValue) { itsPlumeParticleCount = newValue; }
  bool PlumesUsed(void) const { return fPlumesUsed; }
  void PlumesUsed(bool newValue) { fPlumesUsed = newValue; }
  double StartLocationRangeInKM(void) const { return itsStartLocationRangeInKM; }
  void StartLocationRangeInKM(double newValue) { itsStartLocationRangeInKM = newValue; }
  int StartTimeRangeInMinutes(void) const { return itsStartTimeRangeInMinutes; }
  void StartTimeRangeInMinutes(int newValue) { itsStartTimeRangeInMinutes = newValue; }
  FmiDirection Direction(void) const { return itsDirection; }
  void Direction(FmiDirection newValue) { itsDirection = newValue; }
  NFmiMetTime CalcPossibleFirstTime(void);
  NFmiMetTime CalcPossibleLastTime(void);
  double PressureLevel(void) const { return itsPressureLevel; }
  void PressureLevel(double newValue) { itsPressureLevel = newValue; }
  double StartPressureLevelRange(void) const { return itsStartPressureLevelRange; }
  void StartPressureLevelRange(double newValue) { itsStartPressureLevelRange = newValue; }
  bool Is3DTrajectory(void) const;
  const NFmiMetTime& OriginTime(void) const { return itsOriginTime; }
  void OriginTime(const NFmiMetTime& theOriginTime) { itsOriginTime = theOriginTime; }
  bool Isentropic(void) const { return fIsentropic; }
  void Isentropic(bool newValue) { fIsentropic = newValue; }
  double IsentropicTpotValue(void) const;
  void CalculateCrossSectionTrajectoryHelpData(void);
  const checkedVector<NFmiPoint>& CrossSectionTrajectoryPoints(void) const
  {
    return itsCrossSectionTrajectoryPoints;
  }
  const checkedVector<NFmiMetTime> CrossSectionTrajectoryTimes(void) const
  {
    return itsCrossSectionTrajectoryTimes;
  }
  bool CalcTempBalloonTrajectories(void) const { return fCalcTempBalloonTrajectories; }
  void CalcTempBalloonTrajectories(bool newValue) { fCalcTempBalloonTrajectories = newValue; }
  const NFmiTempBalloonTrajectorSettings& TempBalloonTrajectorSettings(void) const
  {
    return itsTempBalloonTrajectorSettings;
  }
  void TempBalloonTrajectorSettings(const NFmiTempBalloonTrajectorSettings& newValue)
  {
    itsTempBalloonTrajectorSettings = newValue;
  }
  bool Calculated(void) const { return fCalculated; }
  void Calculated(bool newValue) { fCalculated = newValue; }
  void Write(std::ostream& os) const;
  void Read(std::istream& is);

 private:
  NFmiSingleTrajector itsMainTrajector;  // 'pää' trajektori eli ei mitää häirintää tämän laskussa
  checkedVector<boost::shared_ptr<NFmiSingleTrajector> >
      itsPlumeTrajectories;  // jos haluttu parvi trajektoreita, ne on talletettu tänne
  NFmiPoint itsLatLon;  // alku piste ns. pääpiste, koska tästä voidaan laskea myös "häirityt"
                        // alkupisteet pluumiin
  NFmiMetTime itsTime;  // alku aika ns. pääaika, koska tästä voidaan laskea myös "häirityt"
                        // alkuajat pluumiin
  NFmiMetTime itsOriginTime;  // käytetyn datan origin time
  NFmiProducer itsProducer;
  int itsDataType;  // 0=pinta, 1=painepinta, 2=mallipinta ja 3=historia dataa
  int itsTimeStepInMinutes;
  int itsTimeLengthInHours;
  double itsPlumeProbFactor;
  int itsPlumeParticleCount;
  double itsStartLocationRangeInKM;
  double itsPressureLevel;            // yks. hPa
  double itsStartPressureLevelRange;  // yks. hPa
  double itsIsentropicTpotValue;  // kun lasketaan trajektoreita, tähän talletetaan aina viimeisin
                                  // haluttu potentiaali lämpötila, mitä on käytetty
  int itsStartTimeRangeInMinutes;
  FmiDirection itsDirection;
  checkedVector<NFmiPoint> itsCrossSectionTrajectoryPoints;
  checkedVector<NFmiMetTime> itsCrossSectionTrajectoryTimes;
  bool fPlumesUsed;
  bool fIsentropic;
  bool fCalcTempBalloonTrajectories;
  NFmiTempBalloonTrajectorSettings itsTempBalloonTrajectorSettings;
  bool fCalculated;  // Tämä on optimointia tämän avulla piirto koodi voi tarkistaa, onko trajektori
                     // jo laskettu
  // vai pitääkö trajektorin arvot laskea.
  // Tämä on tehty erityisesti viewMakro optimointia varten. Kun viewMacro ladataan, ei lasketa
  // trajektoreita heti
  // koska niiden laskut voivat olla aikaa vieviä, varsinkin jos kyseisessä viewMakrossa niitä ei
  // edes näytetä.
};

inline std::ostream& operator<<(std::ostream& os, const NFmiTrajectory& item)
{
  item.Write(os);
  return os;
}
inline std::istream& operator>>(std::istream& is, NFmiTrajectory& item)
{
  item.Read(is);
  return is;
}

inline std::ostream& operator<<(std::ostream& os, boost::shared_ptr<NFmiTrajectory> item)
{
  (*item).Write(os);
  return os;
}
inline std::istream& operator>>(std::istream& is, boost::shared_ptr<NFmiTrajectory> item)
{
  (*item).Read(is);
  return is;
}
