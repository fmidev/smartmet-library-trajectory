#pragma once

#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiGlobals.h>

#include <boost/shared_ptr.hpp>


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

inline std::ostream& operator<<(std::ostream& os, const NFmiTempBalloonTrajectorSettings& item){item.Write(os); return os;}
inline std::istream& operator>>(std::istream& is, NFmiTempBalloonTrajectorSettings& item){item.Read(is); return is;}

