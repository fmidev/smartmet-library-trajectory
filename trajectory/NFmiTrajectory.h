#pragma once

#include "NFmiSingleTrajector.h"
#include "NFmiTempBalloonTrajectorSettings.h"
#include <memory>
#include <newbase/NFmiMetTime.h>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiProducer.h>

// trajektori voi pitää sisällään yhdestä useampaan kpl yksittäisiä
// trajektoreita. Yksi pää trajektory ja ns. satunnais pluumi, jossa useita partikkeleita.
class NFmiTrajectory
{
 public:
  NFmiTrajectory();
  NFmiTrajectory(const NFmiPoint& theLatLon,
                 const NFmiMetTime& theTime,
                 const NFmiProducer& theProducer,
                 int theDataType,
                 int theTimeStepInMinutes,
                 int theTimeLengthInHours);
  ~NFmiTrajectory();

  void Clear();
  std::string ToXMLStr();
  const NFmiPoint& LatLon() const { return itsLatLon; }
  void LatLon(const NFmiPoint& newValue) { itsLatLon = newValue; }
  const NFmiMetTime& Time() const { return itsTime; }
  void Time(const NFmiMetTime& newValue) { itsTime = newValue; }
  const NFmiProducer& Producer() const { return itsProducer; }
  void Producer(const NFmiProducer& newValue) { itsProducer = newValue; }
  int DataType() const { return itsDataType; }
  void DataType(int newValue) { itsDataType = newValue; }
  int TimeStepInMinutes() const { return itsTimeStepInMinutes; }
  void TimeStepInMinutes(int newValue) { itsTimeStepInMinutes = newValue; }
  int TimeLengthInHours() const { return itsTimeLengthInHours; }
  void TimeLengthInHours(int newValue) { itsTimeLengthInHours = newValue; }
  const NFmiSingleTrajector& MainTrajector() const { return itsMainTrajector; }
  void MainTrajector(const NFmiSingleTrajector& theTrajector) { itsMainTrajector = theTrajector; }
  void AddPlumeTrajector(const std::shared_ptr<NFmiSingleTrajector>& theTrajector);
  const std::vector<std::shared_ptr<NFmiSingleTrajector> >& PlumeTrajectories() const
  {
    return itsPlumeTrajectories;
  }
  double PlumeProbFactor() const { return itsPlumeProbFactor; }
  void PlumeProbFactor(double newValue) { itsPlumeProbFactor = newValue; }
  int PlumeParticleCount() const { return itsPlumeParticleCount; }
  void PlumeParticleCount(int newValue) { itsPlumeParticleCount = newValue; }
  bool PlumesUsed() const { return fPlumesUsed; }
  void PlumesUsed(bool newValue) { fPlumesUsed = newValue; }
  double StartLocationRangeInKM() const { return itsStartLocationRangeInKM; }
  void StartLocationRangeInKM(double newValue) { itsStartLocationRangeInKM = newValue; }
  int StartTimeRangeInMinutes() const { return itsStartTimeRangeInMinutes; }
  void StartTimeRangeInMinutes(int newValue) { itsStartTimeRangeInMinutes = newValue; }
  FmiDirection Direction() const { return itsDirection; }
  void Direction(FmiDirection newValue) { itsDirection = newValue; }
  NFmiMetTime CalcPossibleFirstTime();
  NFmiMetTime CalcPossibleLastTime();
  double PressureLevel() const { return itsPressureLevel; }
  void PressureLevel(double newValue) { itsPressureLevel = newValue; }
  double StartPressureLevelRange() const { return itsStartPressureLevelRange; }
  void StartPressureLevelRange(double newValue) { itsStartPressureLevelRange = newValue; }
  bool Is3DTrajectory() const;
  const NFmiMetTime& OriginTime() const { return itsOriginTime; }
  void OriginTime(const NFmiMetTime& theOriginTime) { itsOriginTime = theOriginTime; }
  bool Isentropic() const { return fIsentropic; }
  void Isentropic(bool newValue) { fIsentropic = newValue; }
  double IsentropicTpotValue() const;
  void CalculateCrossSectionTrajectoryHelpData();
  const std::vector<NFmiPoint>& CrossSectionTrajectoryPoints() const
  {
    return itsCrossSectionTrajectoryPoints;
  }
  const std::vector<NFmiMetTime> CrossSectionTrajectoryTimes() const
  {
    return itsCrossSectionTrajectoryTimes;
  }
  bool CalcTempBalloonTrajectories() const { return fCalcTempBalloonTrajectories; }
  void CalcTempBalloonTrajectories(bool newValue) { fCalcTempBalloonTrajectories = newValue; }
  const NFmiTempBalloonTrajectorSettings& TempBalloonTrajectorSettings() const
  {
    return itsTempBalloonTrajectorSettings;
  }
  void TempBalloonTrajectorSettings(const NFmiTempBalloonTrajectorSettings& newValue)
  {
    itsTempBalloonTrajectorSettings = newValue;
  }
  bool Calculated() const { return fCalculated; }
  void Calculated(bool newValue) { fCalculated = newValue; }
  void Write(std::ostream& os) const;
  void Read(std::istream& is);

 private:
  NFmiSingleTrajector itsMainTrajector;  // 'pää' trajektori eli ei mitää häirintää tämän laskussa
  std::vector<std::shared_ptr<NFmiSingleTrajector> >
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
  std::vector<NFmiPoint> itsCrossSectionTrajectoryPoints;
  std::vector<NFmiMetTime> itsCrossSectionTrajectoryTimes;
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

inline std::ostream& operator<<(std::ostream& os, std::shared_ptr<NFmiTrajectory> item)
{
  (*item).Write(os);
  return os;
}
inline std::istream& operator>>(std::istream& is, std::shared_ptr<NFmiTrajectory> item)
{
  (*item).Read(is);
  return is;
}
