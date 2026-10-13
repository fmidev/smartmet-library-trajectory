//© Ilmatieteenlaitos/Marko.
// Original 12.8.2005
//
// Luokka pitää huolta trajektoreihin liittyvistä asioista.
//---------------------------------------------------------- NFmiTrajectorySystem.h

#pragma once

#include "NFmiSingleTrajector.h"
#include "NFmiTempBalloonTrajectorSettings.h"
#include "NFmiTrajectory.h"

#include <smarttools/NFmiRawTempStationInfoSystem.h>

class NFmiInfoOrganizer;
class NFmiFastQueryInfo;
class NFmiProducerSystem;

//_________________________________________________________ NFmiTrajectorySystem
class NFmiTrajectorySystem
{
 public:
  static double itsLatestVersionNumber;
  mutable double itsCurrentVersionNumber;

  NFmiTrajectorySystem(NFmiInfoOrganizer *theInfoOrganizer, NFmiProducerSystem *theProducerSystem);
  ~NFmiTrajectorySystem();

  void Init(const NFmiTrajectorySystem &theOther);
  void InitializeFromSettings();
  void StoreSettings();
  bool SaveXML(const std::string &theFileName);
  void ClearTrajectories();
  void SetSelectedValuesToAllTrajectories();
  void SetSelectedValuesToLastTrajectory();
  void ReCalculateTrajectories();
  bool TrajectoryViewOn() const { return fTrajectoryViewOn; }
  void TrajectoryViewOn(bool newValue) { fTrajectoryViewOn = newValue; }
  const NFmiPoint& SelectedLatLon() const { return itsSelectedLatLon; }
  void SelectedLatLon(const NFmiPoint &newValue) { itsSelectedLatLon = newValue; }
  const NFmiMetTime& SelectedTime() const { return itsSelectedTime; }
  void SelectedTime(const NFmiMetTime &newValue)
  {
    fTrajectoryViewTimeBagDirty = true;
    itsSelectedTime = newValue;
  }
  const NFmiProducer& SelectedProducer() const { return itsSelectedProducer; }
  void SelectedProducer(const NFmiProducer &newValue) { itsSelectedProducer = newValue; }
  int SelectedDataType() const { return itsSelectedDataType; }
  void SelectedDataType(int newValue) { itsSelectedDataType = newValue; }
  const std::vector<std::shared_ptr<NFmiTrajectory> >& Trajectories() const
  {
    return itsTrajectories;
  }
  const NFmiTrajectory &Trajectory(int theIndex) const;
  void AddTrajectory(bool fCalculateData);
  int SelectedTimeStepInMinutes() const { return itsSelectedTimeStepInMinutes; }
  void SelectedTimeStepInMinutes(int newValue) { itsSelectedTimeStepInMinutes = newValue; }
  int SelectedTimeLengthInHours() const { return itsSelectedTimeLengthInHours; }
  void SelectedTimeLengthInHours(int newValue)
  {
    fTrajectoryViewTimeBagDirty = true;
    itsSelectedTimeLengthInHours = newValue;
  }
  double SelectedPlumeProbFactor() const { return itsSelectedPlumeProbFactor; }
  void SelectedPlumeProbFactor(double newValue) { itsSelectedPlumeProbFactor = newValue; }
  int SelectedPlumeParticleCount() const { return itsSelectedPlumeParticleCount; }
  void SelectedPlumeParticleCount(int newValue)
  {
    fTrajectoryViewTimeBagDirty = true;
    itsSelectedPlumeParticleCount = newValue;
  }
  bool PlumesUsed() const { return fPlumesUsed; }
  void PlumesUsed(bool newValue)
  {
    fTrajectoryViewTimeBagDirty = true;
    fPlumesUsed = newValue;
  }
  double SelectedStartLocationRangeInKM() const { return itsSelectedStartLocationRangeInKM; }
  void SelectedStartLocationRangeInKM(double newValue)
  {
    itsSelectedStartLocationRangeInKM = newValue;
  }
  int SelectedStartTimeRangeInMinutes() const { return itsSelectedStartTimeRangeInMinutes; }
  void SelectedStartTimeRangeInMinutes(int newValue)
  {
    fTrajectoryViewTimeBagDirty = true;
    itsSelectedStartTimeRangeInMinutes = newValue;
  }
  const NFmiTimeBag& TrajectoryViewTimeBag();
  void TrajectoryViewTimeBag(const NFmiTimeBag &newTimeBag);
  bool TrajectoryViewTimeBagDirty() const { return fTrajectoryViewTimeBagDirty; }
  void TrajectoryViewTimeBagDirty(bool newValue) { fTrajectoryViewTimeBagDirty = newValue; }
  double SelectedPressureLevel() const { return itsSelectedPressureLevel; }
  void SelectedPressureLevel(double newValue) { itsSelectedPressureLevel = newValue; }
  double SelectedStartPressureLevelRange() const { return itsSelectedStartPressureLevelRange; }
  void SelectedStartPressureLevelRange(double newValue)
  {
    itsSelectedStartPressureLevelRange = newValue;
  }
  FmiDirection SelectedDirection() const { return itsSelectedDirection; }
  void SelectedDirection(FmiDirection newValue) { itsSelectedDirection = newValue; }
  bool ShowTrajectoryArrows() const { return fShowTrajectoryArrows; }
  void ShowTrajectoryArrows(bool newValue) { fShowTrajectoryArrows = newValue; }
  bool ShowTrajectoryAnimationMarkers() const { return fShowTrajectoryAnimationMarkers; }
  void ShowTrajectoryAnimationMarkers(bool newValue) { fShowTrajectoryAnimationMarkers = newValue; }
  bool SelectedTrajectoryIsentropic() const { return fSelectedTrajectoryIsentropic; }
  void SelectedTrajectoryIsentropic(bool newValue) { fSelectedTrajectoryIsentropic = newValue; }
  bool ShowTrajectoriesInCrossSectionView() const { return fShowTrajectoriesInCrossSectionView; }
  void ShowTrajectoriesInCrossSectionView(bool newValue)
  {
    fShowTrajectoriesInCrossSectionView = newValue;
  }
  NFmiTempBalloonTrajectorSettings& TempBalloonTrajectorSettings()
  {
    return itsTempBalloonTrajectorSettings;
  }
  bool CalcTempBalloonTrajectors() const { return fCalcTempBalloonTrajectors; }
  void CalcTempBalloonTrajectors(bool newValue) { fCalcTempBalloonTrajectors = newValue; }
  bool UseMapTime() const { return fUseMapTime; }
  void UseMapTime(bool newValue) { fUseMapTime = newValue; }
  std::shared_ptr<NFmiFastQueryInfo> GetWantedInfo(
      const std::shared_ptr<NFmiTrajectory>& theTrajectory);
  bool TrajectorySaveEnabled() const { return fTrajectorySaveEnabled; }
  void TrajectorySaveEnabled(bool newValue) { fTrajectorySaveEnabled = newValue; }
  const std::string& TrajectorySavePath() const { return itsTrajectorySavePath; }
  void TrajectorySavePath(const std::string &newValue) { itsTrajectorySavePath = newValue; }
  const std::string& TrajectorySaveFilePattern() const { return itsTrajectorySaveFilePattern; }
  void TrajectorySaveFilePattern(const std::string &newValue)
  {
    itsTrajectorySaveFilePattern = newValue;
  }
  const std::string& LastTrajectoryLegendStr() const { return itsLastTrajectoryLegendStr; }
  void LastTrajectoryLegendStr(const std::string &newValue)
  {
    itsLastTrajectoryLegendStr = newValue;
  }
  std::string MakeCurrentTrajectorySaveFileName();
  NFmiSilamStationList& NuclearPlants() { return itsNuclearPlants; }
  NFmiSilamStationList& OtherPlaces() { return itsOtherPlaces; }
  void MakeSureThatTrajectoriesAreCalculated();  // tämä on viewmakro optimointia varten tehty
                                                 // varmistus funktio. Tätä kutsutaan
                                                 // trajektoryView-luokassa ennen varsinaista
                                                 // trajektorien piirtoa
  void SetCaseStudyTimes(const NFmiMetTime &theCaseStudyTime);
  void CalculateTrajectory(const std::shared_ptr<NFmiTrajectory>& theTrajectory);
  static void CalculateTrajectory(const std::shared_ptr<NFmiTrajectory>& theTrajectory,
                                  std::shared_ptr<NFmiFastQueryInfo> &theInfo);

  void Write(std::ostream &os) const;
  void Read(std::istream &is);

 private:
  static void Make3DRandomizing(double &WS,
                                double &WD,
                                double &w,
                                int theRandStep,
                                int theCounter,
                                double theRandomFactor,
                                NFmiSingleTrajector &theTrajector);
  void CalculateTrajectoryViewTimeBag();
  void SetSelectedValuesToTrajectory(const std::shared_ptr<NFmiTrajectory>& theTrajectory,
                                     bool fInitialize,
                                     bool fKeepLevelSettings);
  static void CalculateSingleTrajectory(
      std::shared_ptr<NFmiFastQueryInfo> &theInfo,
      NFmiSingleTrajector &theTrajector,
      int theTimeStepInMinutes,
      int theTimeLengthInHours,
      double theRandFactor,
      int theRandStep,
      FmiDirection theDirection,
      bool fIsentropic,
      bool fCalcBalloonTrajectory,
      NFmiTempBalloonTrajectorSettings &theTempBalloonTrajectorSettings);
  static void CalculateSingle3DTrajectory(
      const std::shared_ptr<NFmiFastQueryInfo>& theInfo,
      NFmiSingleTrajector &theTrajector,
      int theTimeStepInMinutes,
      int theTimeLengthInHours,
      double theRandFactor,
      int theRandStep,
      FmiDirection theDirection,
      bool fIsentropic,
      bool fCalcBalloonTrajectory,
      NFmiTempBalloonTrajectorSettings &theTempBalloonTrajectorSettings);

  NFmiInfoOrganizer *itsInfoOrganizer;  // täältä saadaan data, ei omista/tuhoa!
  NFmiProducerSystem *itsProducerSystem;
  NFmiSilamStationList itsNuclearPlants;
  NFmiSilamStationList itsOtherPlaces;
  std::vector<std::shared_ptr<NFmiTrajectory> > itsTrajectories;
  NFmiPoint itsSelectedLatLon;
  NFmiMetTime itsSelectedTime;
  NFmiProducer itsSelectedProducer;
  int itsSelectedDataType;  // 0=pinta, 1=painepinta, 2=mallipinta ja 3=historia dataa
  int itsSelectedTimeStepInMinutes;
  int itsSelectedTimeLengthInHours;
  double itsSelectedPlumeProbFactor;
  int itsSelectedPlumeParticleCount;
  double itsSelectedStartLocationRangeInKM;
  double itsSelectedPressureLevel;            // yks. hPa
  double itsSelectedStartPressureLevelRange;  // yks. hPa
  FmiDirection itsSelectedDirection;
  int itsSelectedStartTimeRangeInMinutes;
  NFmiTimeBag
      itsTrajectoryViewTimeBag;      // laske tähän kaikkien trajektorien yli 1. ja viimeinen aika
  bool fTrajectoryViewTimeBagDirty;  // pitääkö timebagi laskea uudestaan

  bool fPlumesUsed;
  bool fTrajectoryViewOn;
  bool fShowTrajectoryArrows;
  bool fShowTrajectoryAnimationMarkers;
  bool fSelectedTrajectoryIsentropic;
  bool fShowTrajectoriesInCrossSectionView;
  bool
      fUseMapTime;  // käytetäänkö karttanäytön aikaa vai trajektori dialogissa olevaa aika säädintä

  NFmiTempBalloonTrajectorSettings itsTempBalloonTrajectorSettings;
  bool fCalcTempBalloonTrajectors;

  // Trajektorien talletukseen liittyvia muuttujia
  // HUOM nämä luetaan settingeistä, mutta niitä ei vielä talleteta takaisin.
  // HUOM2 Näitä ei ole tarkoituskaan viedä viewMakroihin mitenkään.
  bool fTrajectorySaveEnabled;               // onko talletus systeemi käytössä vai ei
  std::string itsTrajectorySavePath;         // vain polku, ei file filtteriä
  std::string itsTrajectorySaveFilePattern;  // pelkkä lopullisen nimen patterni, missä aikaleiman
                                             // kohdalla esim. "traj_stuk_YYYYMMDDHHmm.txt"
  std::string itsLastTrajectoryLegendStr;  // talletetaan aina viimeisin legenda talteen, että sitä
                                           // ei tarvitse alusta lähtien keksiä
};

inline std::ostream &operator<<(std::ostream &os, const NFmiTrajectorySystem &item)
{
  item.Write(os);
  return os;
}
inline std::istream &operator>>(std::istream &is, NFmiTrajectorySystem &item)
{
  item.Read(is);
  return is;
}
