//© Ilmatieteenlaitos/Marko.
//Original 12.8.2005
//
// Luokka pitää huolta trajektoreihin liittyvistä asioista.
//---------------------------------------------------------- NFmiTrajectorySystem.h

#pragma once

#include "NFmiDataMatrix.h"
#include "NFmiTimeBag.h"
#include "NFmiPoint.h"
#include "NFmiProducer.h"
#include "NFmiRawTempStationInfoSystem.h"
#include "boost/shared_ptr.hpp"

class NFmiInfoOrganizer;
class NFmiFastQueryInfo;
class NFmiProducerSystem;

// luokka joka pitää sisällään luotauspallo trajektori asetukset
class NFmiTempBalloonTrajectorSettings
{
 public:
	 NFmiTempBalloonTrajectorSettings(void):itsRisingSpeed(4.34),itsFallSpeed(3.28),itsTopHeightInKM(15),itsFloatingTimeInMinutes(0){};
	 ~NFmiTempBalloonTrajectorSettings(void){};
	 double RisingSpeed(void) const {return itsRisingSpeed;}
	 void RisingSpeed(double newValue) {itsRisingSpeed = newValue;}
	 double FallSpeed(void) const {return itsFallSpeed;}
	 void FallSpeed(double newValue) {itsFallSpeed = newValue;}
	 double TopHeightInKM(void) const {return itsTopHeightInKM;}
	 void TopHeightInKM(double newValue) {itsTopHeightInKM = newValue;}
	 double FloatingTimeInMinutes(void) const {return itsFloatingTimeInMinutes;}
	 void FloatingTimeInMinutes(double newValue) {itsFloatingTimeInMinutes = newValue;}
	 FmiDirection State(void) const {return itsState;}
	 void State(FmiDirection newState) {itsState = newState;}
	 double CurrentFloatTimeInMinutes(void) const {return itsCurrentFloatTimeInMinutes;}
	 void CurrentFloatTimeInMinutes(double newValue) {itsCurrentFloatTimeInMinutes = newValue;} // tällä lähinnä nollataan kellunta aika
	 void AddFloatTime(double theAddedTimeInMinutes) {itsCurrentFloatTimeInMinutes += theAddedTimeInMinutes;}
	 void Reset(void){itsCurrentFloatTimeInMinutes = 0.; itsState = kBase;}
	 // laske currentilla arvoilla nousu/laskunopeus yksikössä hPa/s. Osaa tehdä päätelmiä eri luotaus pallon lennon
	 // vaiheista ja osaa mm. siirtyä seuraavaan vaiheeseen.
	 double CalcOmega(double Z, int theTimeStepInMinutes);
	 double CalcDeltaP(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double theCurrentPressure, unsigned long thePressureParamIndex, double Z, int theTimeStepInMinutes, unsigned long theGroundLevelIndex);

	void Write(std::ostream& os) const;
	void Read(std::istream& is);
private:
	double CalcDeltaPInPhase1(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double theCurrentPressure, unsigned long thePressureParamIndex, double Z, int theTimeStepInMinutes);
	double CalcDeltaPInPhase2(int theTimeStepInMinutes);
	double CalcDeltaPInPhase3(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double theCurrentPressure, unsigned long thePressureParamIndex, double Z, int theTimeStepInMinutes, unsigned long theGroundLevelIndex);
	double CalcOmegaInPhase1(double Z, int theTimeStepInMinutes);
	double CalcOmegaInPhase2(int theTimeStepInMinutes);
	double CalcOmegaInPhase3(double Z, int theTimeStepInMinutes);
	double CalcRisingRate(double Z);
	double CalcFallingRate(double Z);

	 double itsRisingSpeed;
	 double itsFallSpeed;
	 double itsTopHeightInKM;
	 double itsFloatingTimeInMinutes;
	 FmiDirection itsState; // pallolaskuissa on tietyt vaiheet
							// 1. alku (reset tila) eli kBase
							// 2. nousu eli kUp
							// 3. kellunta eli kTop
							// 4. lasku eli kDown
							// 5. loppu (osunut takaisin maahan) eli kNoDirection
	 double itsCurrentFloatTimeInMinutes; // jos 0, ei kellunta ole vielä alkanut, tämän avulla lasketaan milloin kellunta lopetetaan
};

// pluumi eli yksi yksittäisen partikkelin eteneminen
// tähän talletetaan vain trajectorin pisteet ja alku aika ja paikka.
// Muut tiedot ovat ylempänä eli NFmiTrajectory-luokassa
class NFmiSingleTrajector
{
 public:
	NFmiSingleTrajector(void);
	NFmiSingleTrajector(const NFmiPoint &theLatLon,	const NFmiMetTime &theTime, double thePressureLevel);
	~NFmiSingleTrajector(void);

	std::string ToXMLStr(int TimeStepInMinutes, NFmiProducer &Producer, FmiDirection Direction);	
	const checkedVector<NFmiPoint>& Points(void) const {return itsPoints;}
	const checkedVector<float>& Pressures(void) const {return itsPressures;}
	const checkedVector<float>& HeightValues(void) const {return itsHeightValues;}
	const NFmiPoint& StartLatLon(void) const {return itsStartLatLon;}
	void StartLatLon(const NFmiPoint &newValue) {itsStartLatLon = newValue;}
	const NFmiMetTime& StartTime(void) const {return itsStartTime;}
	void StartTime(const NFmiMetTime &newValue) {itsStartTime = newValue;}
	void ClearPoints(void);
	void AddPoint(const NFmiPoint &theLatLon);
	void AddPoint(const NFmiPoint &theLatLon, float thePressure, float theHeightValue);
	double StartPressureLevel(void) const {return itsStartPressureLevel;}
	void StartPressureLevel(double newValue) {itsStartPressureLevel = newValue;}
	bool Is3DTrajectory(void) const;
	double IsentropicTpotValue(void) const {return itsIsentropicTpotValue;}
	void IsentropicTpotValue(double newValue) {itsIsentropicTpotValue = newValue;}
	void SetRandomValues(double theWD, double theWS, double thew);
	double RandWSdiff(void) const {return itsRandWSdiff;}
	double RandWDdiff(void) const {return itsRandWDdiff;}
	double Randwdiff(void) const {return itsRandwdiff;}
 private:
	checkedVector<NFmiPoint> itsPoints;
	checkedVector<float> itsPressures;
	checkedVector<float> itsHeightValues;
	NFmiPoint itsStartLatLon;
	NFmiMetTime itsStartTime;
	double itsStartPressureLevel; // yks. hPa
	double itsIsentropicTpotValue; // kun lasketaan trajektoreita, tähän talletetaan haluttu potentiaali lämpötila, mitä on käytetty

	double itsRandWSdiff; // satunnainen WS muutos
	double itsRandWDdiff; // satunnainen WD muutos
	double itsRandwdiff; // satunnainen w muutos
};

// trajektori voi pitää sisällään yhdestä useampaan kpl yksittäisiä
// trajektoreita. Yksi pää trajektory ja ns. satunnais pluumi, jossa useita partikkeleita.
class NFmiTrajectory
{
 public:
	NFmiTrajectory(void);
	NFmiTrajectory(const NFmiPoint &theLatLon, const NFmiMetTime &theTime, const NFmiProducer &theProducer, int theDataType, int theTimeStepInMinutes,	int theTimeLengthInHours);
	~NFmiTrajectory(void);

	void Clear(void);
	std::string ToXMLStr(void);
	const NFmiPoint& LatLon(void) const {return itsLatLon;}
	void LatLon(const NFmiPoint &newValue) {itsLatLon = newValue;}
	const NFmiMetTime& Time(void) const {return itsTime;}
	void Time(const NFmiMetTime &newValue) {itsTime = newValue;}
	const NFmiProducer& Producer(void) const {return itsProducer;}
	void Producer(const NFmiProducer &newValue) {itsProducer = newValue;}
	int DataType(void) const {return itsDataType;}
	void DataType(int newValue) {itsDataType = newValue;}
	int TimeStepInMinutes(void) const {return itsTimeStepInMinutes;}
	void TimeStepInMinutes(int newValue) {itsTimeStepInMinutes = newValue;}
	int TimeLengthInHours(void) const {return itsTimeLengthInHours;}
	void TimeLengthInHours(int newValue) {itsTimeLengthInHours = newValue;}
	const NFmiSingleTrajector& MainTrajector(void) const {return itsMainTrajector;}
	void MainTrajector(const NFmiSingleTrajector &theTrajector) {itsMainTrajector = theTrajector;}
	void AddPlumeTrajector(boost::shared_ptr<NFmiSingleTrajector> &theTrajector);
	const checkedVector<boost::shared_ptr<NFmiSingleTrajector> >& PlumeTrajectories(void) const {return itsPlumeTrajectories;}
	double PlumeProbFactor(void) const {return itsPlumeProbFactor;}
	void PlumeProbFactor(double newValue) {itsPlumeProbFactor = newValue;}
	int PlumeParticleCount(void) const {return itsPlumeParticleCount;}
	void PlumeParticleCount(int newValue) {itsPlumeParticleCount = newValue;}
	bool PlumesUsed(void) const {return fPlumesUsed;}
	void PlumesUsed(bool newValue) {fPlumesUsed = newValue;}
	double StartLocationRangeInKM(void) const {return itsStartLocationRangeInKM;}
	void StartLocationRangeInKM(double newValue) {itsStartLocationRangeInKM = newValue;}
	int StartTimeRangeInMinutes(void) const {return itsStartTimeRangeInMinutes;}
	void StartTimeRangeInMinutes(int newValue) {itsStartTimeRangeInMinutes = newValue;}
	FmiDirection Direction(void) const {return itsDirection;}
	void Direction(FmiDirection newValue) {itsDirection = newValue;}
	NFmiMetTime CalcPossibleFirstTime(void);
	NFmiMetTime CalcPossibleLastTime(void);
	double PressureLevel(void) const {return itsPressureLevel;}
	void PressureLevel(double newValue) {itsPressureLevel = newValue;}
	double StartPressureLevelRange(void) const {return itsStartPressureLevelRange;}
	void StartPressureLevelRange(double newValue) {itsStartPressureLevelRange = newValue;}
	bool Is3DTrajectory(void) const;
	const NFmiMetTime& OriginTime(void) const {return itsOriginTime;}
	void OriginTime(const NFmiMetTime &theOriginTime) {itsOriginTime = theOriginTime;}
	bool Isentropic(void) const {return fIsentropic;}
	void Isentropic(bool newValue) {fIsentropic = newValue;}
	double IsentropicTpotValue(void) const;
	void CalculateCrossSectionTrajectoryHelpData(void);
	const checkedVector<NFmiPoint>& CrossSectionTrajectoryPoints(void) const {return itsCrossSectionTrajectoryPoints;}
	const checkedVector<NFmiMetTime> CrossSectionTrajectoryTimes(void) const {return itsCrossSectionTrajectoryTimes;}
	bool CalcTempBalloonTrajectories(void) const {return fCalcTempBalloonTrajectories;}
	void CalcTempBalloonTrajectories(bool newValue) {fCalcTempBalloonTrajectories = newValue;}
	const NFmiTempBalloonTrajectorSettings& TempBalloonTrajectorSettings(void) const {return itsTempBalloonTrajectorSettings;}
	void TempBalloonTrajectorSettings(const NFmiTempBalloonTrajectorSettings &newValue) {itsTempBalloonTrajectorSettings = newValue;}
	bool Calculated(void) const {return fCalculated;}
	void Calculated(bool newValue) {fCalculated = newValue;}

	void Write(std::ostream& os) const;
	void Read(std::istream& is);
private:
	NFmiSingleTrajector itsMainTrajector; // 'pää' trajektori eli ei mitää häirintää tämän laskussa
	checkedVector<boost::shared_ptr<NFmiSingleTrajector> > itsPlumeTrajectories; // jos haluttu parvi trajektoreita, ne on talletettu tänne
	NFmiPoint itsLatLon; // alku piste ns. pääpiste, koska tästä voidaan laskea myös "häirityt" alkupisteet pluumiin
	NFmiMetTime itsTime; // alku aika ns. pääaika, koska tästä voidaan laskea myös "häirityt" alkuajat pluumiin
	NFmiMetTime itsOriginTime; // käytetyn datan origin time
	NFmiProducer itsProducer;
	int itsDataType; // 0=pinta, 1=painepinta, 2=mallipinta ja 3=historia dataa
	int itsTimeStepInMinutes;
	int itsTimeLengthInHours;
	double itsPlumeProbFactor;
	int itsPlumeParticleCount;
	double itsStartLocationRangeInKM;
	double itsPressureLevel; // yks. hPa
	double itsStartPressureLevelRange; // yks. hPa
	double itsIsentropicTpotValue; // kun lasketaan trajektoreita, tähän talletetaan aina viimeisin haluttu potentiaali lämpötila, mitä on käytetty
	int itsStartTimeRangeInMinutes;
	FmiDirection itsDirection;
	checkedVector<NFmiPoint> itsCrossSectionTrajectoryPoints;
	checkedVector<NFmiMetTime> itsCrossSectionTrajectoryTimes;
	bool fPlumesUsed;
	bool fIsentropic;
	bool fCalcTempBalloonTrajectories;
	NFmiTempBalloonTrajectorSettings itsTempBalloonTrajectorSettings;
	bool fCalculated; // Tämä on optimointia tämän avulla piirto koodi voi tarkistaa, onko trajektori jo laskettu 
					  // vai pitääkö trajektorin arvot laskea.
					  // Tämä on tehty erityisesti viewMakro optimointia varten. Kun viewMacro ladataan, ei lasketa trajektoreita heti
					  // koska niiden laskut voivat olla aikaa vieviä, varsinkin jos kyseisessä viewMakrossa niitä ei edes näytetä.
};


//_________________________________________________________ NFmiTrajectorySystem
class NFmiTrajectorySystem
{
 public:
	static double itsLatestVersionNumber;
	mutable double itsCurrentVersionNumber;

	NFmiTrajectorySystem(NFmiInfoOrganizer *theInfoOrganizer, NFmiProducerSystem *theProducerSystem);
	~NFmiTrajectorySystem(void);

	void Init(const NFmiTrajectorySystem &theOther);
	void InitializeFromSettings(void);
	void StoreSettings(void);
	bool SaveXML(const std::string &theFileName);
	void ClearTrajectories(void);
	void SetSelectedValuesToAllTrajectories(void);
	void SetSelectedValuesToLastTrajectory(void);
	void ReCalculateTrajectories(void);
	bool TrajectoryViewOn(void) const {return fTrajectoryViewOn;}
	void TrajectoryViewOn(bool newValue) {fTrajectoryViewOn = newValue;}
	const NFmiPoint& SelectedLatLon(void) const {return itsSelectedLatLon;}
	void SelectedLatLon(const NFmiPoint &newValue) {itsSelectedLatLon = newValue;}
	const NFmiMetTime& SelectedTime(void) const {return itsSelectedTime;}
	void SelectedTime(const NFmiMetTime &newValue) {fTrajectoryViewTimeBagDirty = true; itsSelectedTime = newValue;}
	const NFmiProducer& SelectedProducer(void) const {return itsSelectedProducer;}
	void SelectedProducer(const NFmiProducer &newValue) {itsSelectedProducer = newValue;}
	int SelectedDataType(void) const {return itsSelectedDataType;}
	void SelectedDataType(int newValue) {itsSelectedDataType = newValue;}
	const checkedVector<boost::shared_ptr<NFmiTrajectory> >& Trajectories(void) const {return itsTrajectories;}
	const NFmiTrajectory& Trajectory(int theIndex) const;
	void AddTrajectory(bool fCalculateData);
	int SelectedTimeStepInMinutes(void) const {return itsSelectedTimeStepInMinutes;}
	void SelectedTimeStepInMinutes(int newValue) {itsSelectedTimeStepInMinutes = newValue;}
	int SelectedTimeLengthInHours(void) const {return itsSelectedTimeLengthInHours;}
	void SelectedTimeLengthInHours(int newValue) {fTrajectoryViewTimeBagDirty = true; itsSelectedTimeLengthInHours = newValue;}
	double SelectedPlumeProbFactor(void) const {return itsSelectedPlumeProbFactor;}
	void SelectedPlumeProbFactor(double newValue) {itsSelectedPlumeProbFactor = newValue;}
	int SelectedPlumeParticleCount(void) const {return itsSelectedPlumeParticleCount;}
	void SelectedPlumeParticleCount(int newValue) {fTrajectoryViewTimeBagDirty = true; itsSelectedPlumeParticleCount = newValue;}
	bool PlumesUsed(void) const {return fPlumesUsed;}
	void PlumesUsed(bool newValue) {fTrajectoryViewTimeBagDirty = true; fPlumesUsed = newValue;}
	double SelectedStartLocationRangeInKM(void) const {return itsSelectedStartLocationRangeInKM;}
	void SelectedStartLocationRangeInKM(double newValue) {itsSelectedStartLocationRangeInKM = newValue;}
	int SelectedStartTimeRangeInMinutes(void) const {return itsSelectedStartTimeRangeInMinutes;}
	void SelectedStartTimeRangeInMinutes(int newValue) {fTrajectoryViewTimeBagDirty = true; itsSelectedStartTimeRangeInMinutes = newValue;}
	const NFmiTimeBag& TrajectoryViewTimeBag(void);
	void TrajectoryViewTimeBag(const NFmiTimeBag &newTimeBag);
	bool TrajectoryViewTimeBagDirty(void) const {return fTrajectoryViewTimeBagDirty;}
	void TrajectoryViewTimeBagDirty(bool newValue) {fTrajectoryViewTimeBagDirty = newValue;}
	double SelectedPressureLevel(void) const {return itsSelectedPressureLevel;}
	void SelectedPressureLevel(double newValue) {itsSelectedPressureLevel = newValue;}
	double SelectedStartPressureLevelRange(void) const {return itsSelectedStartPressureLevelRange;}
	void SelectedStartPressureLevelRange(double newValue) {itsSelectedStartPressureLevelRange = newValue;}
	FmiDirection SelectedDirection(void) const {return itsSelectedDirection;}
	void SelectedDirection(FmiDirection newValue) {itsSelectedDirection = newValue;}
	bool ShowTrajectoryArrows(void) const {return fShowTrajectoryArrows;}
	void ShowTrajectoryArrows(bool newValue) {fShowTrajectoryArrows = newValue;}
	bool ShowTrajectoryAnimationMarkers(void) const {return fShowTrajectoryAnimationMarkers;}
	void ShowTrajectoryAnimationMarkers(bool newValue) {fShowTrajectoryAnimationMarkers = newValue;}
	bool SelectedTrajectoryIsentropic(void) const {return fSelectedTrajectoryIsentropic;}
	void SelectedTrajectoryIsentropic(bool newValue) {fSelectedTrajectoryIsentropic = newValue;}
	bool ShowTrajectoriesInCrossSectionView(void) const {return fShowTrajectoriesInCrossSectionView;}
	void ShowTrajectoriesInCrossSectionView(bool newValue) {fShowTrajectoriesInCrossSectionView = newValue;}
	NFmiTempBalloonTrajectorSettings& TempBalloonTrajectorSettings(void) {return itsTempBalloonTrajectorSettings;}
	bool CalcTempBalloonTrajectors(void) const {return fCalcTempBalloonTrajectors;}
	void CalcTempBalloonTrajectors(bool newValue) {fCalcTempBalloonTrajectors = newValue;}
	bool UseMapTime(void) {return fUseMapTime;}
	void UseMapTime(bool newValue) {fUseMapTime = newValue;}
	boost::shared_ptr<NFmiFastQueryInfo> GetWantedInfo(boost::shared_ptr<NFmiTrajectory> &theTrajectory);
	bool TrajectorySaveEnabled(void) const {return fTrajectorySaveEnabled;}
	void TrajectorySaveEnabled(bool newValue) {fTrajectorySaveEnabled = newValue;}
	const std::string& TrajectorySavePath(void) const {return itsTrajectorySavePath;}
	void TrajectorySavePath(const std::string &newValue) {itsTrajectorySavePath = newValue;}
	const std::string& TrajectorySaveFilePattern(void) const {return itsTrajectorySaveFilePattern;}
	void TrajectorySaveFilePattern(const std::string &newValue){itsTrajectorySaveFilePattern = newValue;}
	const std::string& LastTrajectoryLegendStr(void) const {return itsLastTrajectoryLegendStr;}
	void LastTrajectoryLegendStr(const std::string &newValue) {itsLastTrajectoryLegendStr = newValue;}
	std::string MakeCurrentTrajectorySaveFileName(void);
	NFmiSilamStationList& NuclearPlants(void) {return itsNuclearPlants;}
	NFmiSilamStationList& OtherPlaces(void) {return itsOtherPlaces;}
	void MakeSureThatTrajectoriesAreCalculated(void); // tämä on viewmakro optimointia varten tehty varmistus funktio. Tätä kutsutaan trajektoryView-luokassa ennen varsinaista trajektorien piirtoa
	void SetCaseStudyTimes(const NFmiMetTime &theCaseStudyTime);
	void CalculateTrajectory(boost::shared_ptr<NFmiTrajectory> &theTrajectory);
	void CalculateTrajectory(boost::shared_ptr<NFmiTrajectory> &theTrajectory, boost::shared_ptr<NFmiFastQueryInfo> &theInfo);

	void Write(std::ostream& os) const;
	void Read(std::istream& is);
private:
	void Make3DRandomizing(double &WS, double &WD, double &w, int theRandStep, int theCounter, double theRandomFactor, NFmiSingleTrajector &theTrajector);
	void CalculateTrajectoryViewTimeBag(void);
	void SetSelectedValuesToTrajectory(boost::shared_ptr<NFmiTrajectory> &theTrajectory, bool fInitialize, bool fKeepLevelSettings);
	void CalculateSingleTrajectory(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, NFmiSingleTrajector &theTrajector, int theTimeStepInMinutes, int theTimeLengthInHours, double theRandFactor, int theRandStep, FmiDirection theDirection, bool fIsentropic, bool fCalcBalloonTrajectory, NFmiTempBalloonTrajectorSettings &theTempBalloonTrajectorSettings);
	void CalculateSingle3DTrajectory(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, NFmiSingleTrajector &theTrajector, int theTimeStepInMinutes, int theTimeLengthInHours, double theRandFactor, int theRandStep, FmiDirection theDirection, bool fIsentropic, bool fCalcBalloonTrajectory, NFmiTempBalloonTrajectorSettings &theTempBalloonTrajectorSettings);

	NFmiInfoOrganizer *itsInfoOrganizer; // täältä saadaan data, ei omista/tuhoa!
	NFmiProducerSystem *itsProducerSystem;
	NFmiSilamStationList itsNuclearPlants;
	NFmiSilamStationList itsOtherPlaces;
	checkedVector<boost::shared_ptr<NFmiTrajectory> > itsTrajectories;
	NFmiPoint itsSelectedLatLon;
	NFmiMetTime itsSelectedTime;
	NFmiProducer itsSelectedProducer;
	int itsSelectedDataType; // 0=pinta, 1=painepinta, 2=mallipinta ja 3=historia dataa
	int itsSelectedTimeStepInMinutes;
	int itsSelectedTimeLengthInHours;
	double itsSelectedPlumeProbFactor;
	int itsSelectedPlumeParticleCount;
	double itsSelectedStartLocationRangeInKM;
	double itsSelectedPressureLevel; // yks. hPa
	double itsSelectedStartPressureLevelRange; // yks. hPa
	FmiDirection itsSelectedDirection;
	int itsSelectedStartTimeRangeInMinutes;
	NFmiTimeBag itsTrajectoryViewTimeBag; // laske tähän kaikkien trajektorien yli 1. ja viimeinen aika
	bool fTrajectoryViewTimeBagDirty; // pitääkö timebagi laskea uudestaan

	bool fPlumesUsed;
	bool fTrajectoryViewOn;
	bool fShowTrajectoryArrows;
	bool fShowTrajectoryAnimationMarkers;
	bool fSelectedTrajectoryIsentropic;
	bool fShowTrajectoriesInCrossSectionView;
	bool fUseMapTime; // käytetäänkö karttanäytön aikaa vai trajektori dialogissa olevaa aika säädintä

	NFmiTempBalloonTrajectorSettings itsTempBalloonTrajectorSettings;
	bool fCalcTempBalloonTrajectors;

	// Trajektorien talletukseen liittyvia muuttujia
	// HUOM nämä luetaan settingeistä, mutta niitä ei vielä talleteta takaisin.
	// HUOM2 Näitä ei ole tarkoituskaan viedä viewMakroihin mitenkään.
	bool fTrajectorySaveEnabled; // onko talletus systeemi käytössä vai ei
	std::string itsTrajectorySavePath; // vain polku, ei file filtteriä
	std::string itsTrajectorySaveFilePattern; // pelkkä lopullisen nimen patterni, missä aikaleiman kohdalla esim. "traj_stuk_YYYYMMDDHHmm.txt"
	std::string itsLastTrajectoryLegendStr; // talletetaan aina viimeisin legenda talteen, että sitä ei tarvitse alusta lähtien keksiä
};

inline std::ostream& operator<<(std::ostream& os, const NFmiTrajectorySystem& item){item.Write(os); return os;}
inline std::istream& operator>>(std::istream& is, NFmiTrajectorySystem& item){item.Read(is); return is;}

inline std::ostream& operator<<(std::ostream& os, const NFmiTempBalloonTrajectorSettings& item){item.Write(os); return os;}
inline std::istream& operator>>(std::istream& is, NFmiTempBalloonTrajectorSettings& item){item.Read(is); return is;}

inline std::ostream& operator<<(std::ostream& os, const NFmiTrajectory& item){item.Write(os); return os;}
inline std::istream& operator>>(std::istream& is, NFmiTrajectory& item){item.Read(is); return is;}

inline std::ostream& operator<<(std::ostream& os, boost::shared_ptr<NFmiTrajectory> item){(*item).Write(os); return os;}
inline std::istream& operator>>(std::istream& is, boost::shared_ptr<NFmiTrajectory> item){(*item).Read(is); return is;}

