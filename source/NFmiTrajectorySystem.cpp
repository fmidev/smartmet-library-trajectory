//© Ilmatieteenlaitos/Marko.
//Original 12.8.2005
//

#include "NFmiTrajectorySystem.h"
#include <newbase/NFmiDataModifierAvg.h>
#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiProducerName.h>
#include <newbase/NFmiSettings.h>
#include <newbase/NFmiValueString.h>
#include <smarttools/NFmiDataStoringHelpers.h>
#include <smarttools/NFmiInfoOrganizer.h>
#include <smarttools/NFmiProducerSystem.h>
#include <fstream>


double NFmiTrajectorySystem::itsLatestVersionNumber = 1.0;

static const NFmiMetTime gMissingTime(1900, 0, 0, 0);

// *****************************************************************************
// ***** NFmiTempBalloonTrajectorSettings **************************************
// *****************************************************************************

// laske currentilla arvoilla nousu/laskunopeus yksikˆss‰ hPa/s. Osaa tehd‰ p‰‰telmi‰ eri luotaus pallon lennon
// vaiheista ja osaa mm. siirty‰ seuraavaan vaiheeseen.
double NFmiTempBalloonTrajectorSettings::CalcOmega(double Z, int theTimeStepInMinutes)
{
	if(theTimeStepInMinutes <= 0 || Z == kFloatMissing)
		itsState = kNoDirection;

	switch(itsState)
	{
	case kBase:
		itsState = kUp; // HUOM! t‰st‰ on siis tarkoitus jatkaa suoraan kUp caseen!
	case kUp:
		return CalcOmegaInPhase1(Z, theTimeStepInMinutes);
	case kTop:
		return CalcOmegaInPhase2(theTimeStepInMinutes);
	case kDown:
		return CalcOmegaInPhase3(Z, theTimeStepInMinutes);
	case kNoDirection: // eli pallo on tˆrm‰nnyt jo maahan tai data on ollut puutteellista
		return kFloatMissing;
	default:
		itsState = kNoDirection;
		return kFloatMissing;
	}
}

double NFmiTempBalloonTrajectorSettings::CalcDeltaP(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double theCurrentPressure, unsigned long thePressureParamIndex, double Z, int theTimeStepInMinutes, unsigned long theGroundLevelIndex)
{
	if(theTimeStepInMinutes <= 0 || Z == kFloatMissing)
		itsState = kNoDirection;

	switch(itsState)
	{
	case kBase:
		itsState = kUp; // HUOM! t‰st‰ on siis tarkoitus jatkaa suoraan kUp caseen!
	case kUp:
		return CalcDeltaPInPhase1(theInfo, theLatlon, theTime, theCurrentPressure, thePressureParamIndex, Z, theTimeStepInMinutes);
	case kTop:
		return CalcDeltaPInPhase2(theTimeStepInMinutes);
	case kDown:
		return CalcDeltaPInPhase3(theInfo, theLatlon, theTime, theCurrentPressure, thePressureParamIndex, Z, theTimeStepInMinutes, theGroundLevelIndex);
	case kNoDirection: // eli pallo on tˆrm‰nnyt jo maahan tai data on ollut puutteellista
		return kFloatMissing;
	default:
		itsState = kNoDirection;
		return kFloatMissing;
	}
}

double NFmiTempBalloonTrajectorSettings::CalcDeltaPInPhase1(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double theCurrentPressure, unsigned long thePressureParamIndex, double Z, int theTimeStepInMinutes)
{
	// 1.a.1 Laske nousunopeus (w)
	double w = CalcRisingRate(Z);
	// 1.a.1b korkeuden muutos (dz = w*timestep)
	double dz = w * theTimeStepInMinutes * 60.;
	// 1.a.2 laske uusi korkeus (z = z+dz)
	double zNew = Z + dz;
	// 1.a.5 jos uusi korkeus yli top-korkeuden, "palauta" pallo top korkeuteen ja aloita kellutus
	if(zNew >= itsTopHeightInKM * 1000.)
	{
		zNew = itsTopHeightInKM * 1000.;
		itsState = kTop;
		if(itsFloatingTimeInMinutes <= 0)
			itsState = kDown;
	}
	// 1.b.1 jatka...  laske deltaP ja palauta se [hPa/s]
	theInfo->ParamIndex(thePressureParamIndex);
	double newP = theInfo->HeightValue(static_cast<float>(zNew), theLatlon, theTime);
	if(newP != kFloatMissing)
		return newP - theCurrentPressure;
	return kFloatMissing;
}

double NFmiTempBalloonTrajectorSettings::CalcDeltaPInPhase2(int theTimeStepInMinutes)
{
	// 2.1 lis‰‰ aikasteppi kellutus aikaan
	itsCurrentFloatTimeInMinutes += theTimeStepInMinutes;
	// 2.2 ollaanko jo kelluttu tarpeeksi
	// 2.3 jos kellutus lopussa, siirry seuraavaan vaiheeseen
	if(itsCurrentFloatTimeInMinutes >= itsFloatingTimeInMinutes)
		itsState = kDown;
	// 2.4 palauta 0 omega
	return 0;
}

// phase3 eli pallon lasku/putoamis vaihe
double NFmiTempBalloonTrajectorSettings::CalcDeltaPInPhase3(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double theCurrentPressure, unsigned long thePressureParamIndex, double Z, int theTimeStepInMinutes, unsigned long theGroundLevelIndex)
{
	// 3.1 Laske laskunopeus (w)
	double w = CalcFallingRate(Z);
	// 3.2 korkeuden muutos (dz = w*timestep)
	double dz = w * theTimeStepInMinutes * 60.;
	// 3.3 laske uusi korkeus (z = z-dz)
	double zNew = Z + dz;
	// 3.4 ollaanko jo tultu pinnan alle?
	theInfo->ParamIndex(theInfo->HeightParamIndex());
	theInfo->LevelIndex(theGroundLevelIndex);
	const double surfaceZ = theInfo->InterpolatedValue(theLatlon, theTime);
	if(zNew <= surfaceZ)
	{
		// 3.4.1 jos ollaan, laske pinta korkeuteen omega ja laita state:ksi loppu ja palauta omega
		zNew = surfaceZ;
		itsState = kNoDirection;
	}
	// 3.4.2 laske deltaP ja palauta se
	theInfo->ParamIndex(thePressureParamIndex);
	// HUOM! jos ollaan jo pinnassa, pyyd‰ pinta painetta, muuten painetta saadusta korkeudesta
	double newP = (itsState == kNoDirection) ? theInfo->InterpolatedValue(theLatlon, theTime) : theInfo->HeightValue(static_cast<float>(zNew), theLatlon, theTime);
	if(newP != kFloatMissing)
		return newP - theCurrentPressure;
	return kFloatMissing;
}

// Sodankyl‰n observatoriosta saatu pallon 'vakio' nousunopeus kaava. Riippuvainen korkeudesta
double NFmiTempBalloonTrajectorSettings::CalcRisingRate(double Z)
{
	double w = ((1.19 * itsRisingSpeed) - 4.42) * 1./10000. * Z + 3.42;
	return w;
}

// Sodankyl‰n observatoriosta saatu pallon 'vakio' lasku/putoamisnopeus kaava. Riippuvainen korkeudesta
double NFmiTempBalloonTrajectorSettings::CalcFallingRate(double Z)
{
	double w = (-3.44*1./100000000.*Z*Z) + (1.24*1./10000.*Z) - 5.17;
	return w;
}

// HUOM! T‰m‰n pit‰isi olla Z-riippuvainen (g ja roo muuttuvat korkeuden mukaan).
// Laskee paineen muutosnopeuden (omega [hPa/s]) vertikaalinopeuden (w) ja korkeuden (z)
// avulla.
static double CalcOmegaValue(double w, double  /* Z */ )
{
	const double g = 9.81; // putoamis kiihtyvyys (pit‰isi vaihtua korkeuden mukana)
	const double rooAir = 1; // ilman tiheys rooAir (olisi pinnalla n. 1.2 ja pit‰isi muuttua korkeuden mukaan)
	double omega = -g*rooAir*w;  // k‰ytetty kaavaa omega = -g*roo*w eli -kiihtyvyys*ilman tiheys*vertikaali nopeus
	return omega;
}

// phase1 eli pallon nousuvaihe
double NFmiTempBalloonTrajectorSettings::CalcOmegaInPhase1(double Z, int theTimeStepInMinutes)
{
	// 1.a.1 Laske nousunopeus (w)
	double w = CalcRisingRate(Z);
	// 1.a.1b korkeuden muutos (dz = w*timestep)
	double dz = w * theTimeStepInMinutes * 60.;
	// 1.a.2 laske uusi korkeus (z = z+dz)
	double zNew = Z + dz;
	// 1.a.5 jos uusi korkeus yli top-korkeuden, "palauta" pallo top korkeuteen ja aloita kellutus
	if(zNew >= itsTopHeightInKM * 1000.)
	{
		zNew = itsTopHeightInKM * 1000.;
		itsState = kTop;
		if(itsFloatingTimeInMinutes <= 0)
			itsState = kDown;
	}
	// 1.b.1 jatka...  laske omega ja palauta se [hPa/s]
	double realDZ = zNew - Z;
	double realW = realDZ/(theTimeStepInMinutes * 60.);
	return CalcOmegaValue(realW, Z);
}

// phase2 eli pallon kellutus vaihe (kelluu yl‰ rajalla)
double NFmiTempBalloonTrajectorSettings::CalcOmegaInPhase2(int theTimeStepInMinutes)
{
	// 2.1 lis‰‰ aikasteppi kellutus aikaan
	itsCurrentFloatTimeInMinutes += theTimeStepInMinutes;
	// 2.2 ollaanko jo kelluttu tarpeeksi
	// 2.3 jos kellutus lopussa, siirry seuraavaan vaiheeseen
	if(itsCurrentFloatTimeInMinutes >= itsFloatingTimeInMinutes)
		itsState = kDown;
	// 2.4 palauta 0 omega
	return 0;
}

// phase3 eli pallon lasku/putoamis vaihe
double NFmiTempBalloonTrajectorSettings::CalcOmegaInPhase3(double Z, int theTimeStepInMinutes)
{
	// 3.1 Laske laskunopeus (w)
	double w = CalcFallingRate(Z);
	// 3.2 korkeuden muutos (dz = w*timestep)
	double dz = w * theTimeStepInMinutes * 60.;
	// 3.3 laske uusi korkeus (z = z-dz)
	double zNew = Z + dz;
	// 3.4 ollaanko jo tultu pinnan alle?
	const double surfaceZ = 0;
	if(zNew <= surfaceZ)
	{
		// 3.4.1 jos ollaan, laske pinta korkeuteen omega ja laita state:ksi loppu ja palauta omega
		zNew = surfaceZ;
		itsState = kNoDirection;
	}
	// 3.4.2 laske omega ja palauta se
	double realDZ = zNew - Z;
	double realW = realDZ/(theTimeStepInMinutes * 60.);
	return CalcOmegaValue(realW, Z);
}

void NFmiTempBalloonTrajectorSettings::Write(std::ostream& os) const
{
	os << "// NFmiTempBalloonTrajectorSettings::Write..." << std::endl;

	os << "// RisingSpeed + FallSpeed + TopHeightInKM + FloatingTimeInMinutes" << std::endl;
	os << itsRisingSpeed << " " << itsFallSpeed << " " << itsTopHeightInKM << " " << itsFloatingTimeInMinutes << std::endl;

	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi viel‰ mahdollinen extra data
	// Kun tulee uusia muuttujia, tee t‰h‰n extradatan t‰yttˆ‰, jotta se saadaan talteen tiedopstoon siten ett‰
	// edelliset versiot eiv‰t mene solmuun vaikka on tullut uutta dataa.
	os << "// possible extra data" << std::endl;
	os << extraData;

	if(os.fail())
		throw std::runtime_error("NFmiTempBalloonTrajectorSettings::Write failed");
}

void NFmiTempBalloonTrajectorSettings::Read(std::istream& is)
{ // toivottavasti olet poistanut kommentit luettavasta streamista!!

	is >> itsRisingSpeed >> itsFallSpeed >> itsTopHeightInKM >> itsFloatingTimeInMinutes;
	itsState = kBase;
	itsCurrentFloatTimeInMinutes = 0;

	if(is.fail())
		throw std::runtime_error("NFmiTempBalloonTrajectorSettings::Read failed");
	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi viel‰ mahdollinen extra data
	is >> extraData;
	// T‰ss‰ sitten otetaaan extradatasta talteen uudet muuttujat, mit‰ on mahdollisesti tullut
	// eli jos uusia muutujia tai arvoja, k‰sittele t‰ss‰.

	if(is.fail())
		throw std::runtime_error("NFmiTempBalloonTrajectorSettings::Read failed");
}

//_________________________________________________________ NFmiSingleTrajector
NFmiSingleTrajector::NFmiSingleTrajector(void)
:itsPoints()
,itsPressures()
,itsHeightValues()
,itsStartLatLon()
,itsStartTime()
,itsStartPressureLevel(850)
,itsRandWSdiff(0)
,itsRandWDdiff(0)
,itsRandwdiff(0)
{
}

NFmiSingleTrajector::NFmiSingleTrajector(const NFmiPoint &theLatLon, const NFmiMetTime &theTime, double thePressureLevel)
:itsPoints()
,itsPressures()
,itsHeightValues()
,itsStartLatLon(theLatLon)
,itsStartTime(theTime)
,itsStartPressureLevel(thePressureLevel)
,itsRandWSdiff(0)
,itsRandWDdiff(0)
,itsRandwdiff(0)
{
}

NFmiSingleTrajector::~NFmiSingleTrajector(void)
{
}

void NFmiSingleTrajector::ClearPoints(void)
{
	itsPoints.clear();
	itsPressures.clear();
}

void NFmiSingleTrajector::AddPoint(const NFmiPoint &theLatLon)
{
	itsPoints.push_back(theLatLon);
}

void NFmiSingleTrajector::AddPoint(const NFmiPoint &theLatLon, float thePressure, float theHeightValue)
{
	itsPoints.push_back(theLatLon);
	itsPressures.push_back(thePressure);
	itsHeightValues.push_back(theHeightValue);
}

bool NFmiSingleTrajector::Is3DTrajectory(void) const
{
	if(!itsPoints.empty())
		if(itsPoints.size() == itsPressures.size())
			return true;
	return false;
}

void NFmiSingleTrajector::SetRandomValues(double theWD, double theWS, double thew)
{
	itsRandWSdiff = theWS;
	itsRandWDdiff = theWD;
	itsRandwdiff = thew;
}

//_________________________________________________________ NFmiTrajectory
NFmiTrajectory::NFmiTrajectory(void)
:itsMainTrajector()
,itsPlumeTrajectories()
,itsLatLon()
,itsTime()
,itsOriginTime()
,itsProducer()
,itsDataType(2)
,itsTimeStepInMinutes(10)
,itsTimeLengthInHours(48)
,itsPlumeProbFactor(25)
,itsPlumeParticleCount(50)
,itsStartLocationRangeInKM(0)
,itsPressureLevel(850)
,itsIsentropicTpotValue(kFloatMissing)
,itsStartPressureLevelRange(0)
,itsStartTimeRangeInMinutes(0)
,itsDirection(kForward)
,itsCrossSectionTrajectoryPoints()
,itsCrossSectionTrajectoryTimes()
,fPlumesUsed(true)
,fIsentropic(false)
,fCalcTempBalloonTrajectories(false)
,itsTempBalloonTrajectorSettings()
,fCalculated(false)
{
}

NFmiTrajectory::NFmiTrajectory(const NFmiPoint &theLatLon, const NFmiMetTime &theTime, const NFmiProducer &theProducer, int theDataType, int theTimeStepInMinutes,	int theTimeLengthInHours)
:itsMainTrajector()
,itsPlumeTrajectories()
,itsLatLon(theLatLon)
,itsTime(theTime)
,itsProducer(theProducer)
,itsDataType(theDataType)
,itsTimeStepInMinutes(theTimeStepInMinutes)
,itsTimeLengthInHours(theTimeLengthInHours)
,itsPlumeProbFactor(25)
,itsPlumeParticleCount(50)
,itsStartLocationRangeInKM(0)
,itsPressureLevel(850)
,itsIsentropicTpotValue(kFloatMissing)
,itsStartPressureLevelRange(0)
,itsStartTimeRangeInMinutes(0)
,itsDirection(kForward)
,itsCrossSectionTrajectoryPoints()
,itsCrossSectionTrajectoryTimes()
,fPlumesUsed(true)
,fIsentropic(false)
,fCalcTempBalloonTrajectories(false)
,itsTempBalloonTrajectorSettings()
,fCalculated(false)
{
}

NFmiTrajectory::~NFmiTrajectory(void)
{
}

void NFmiTrajectory::Clear(void)
{
	itsMainTrajector.ClearPoints();
	itsPlumeTrajectories.clear(); // kaikki roskiin vain
	fCalculated = false;
}

void NFmiTrajectory::AddPlumeTrajector(boost::shared_ptr<NFmiSingleTrajector> &theTrajector)
{
	itsPlumeTrajectories.push_back(theTrajector);
}

NFmiMetTime NFmiTrajectory::CalcPossibleFirstTime(void)
{
	if(itsDirection == kForward)
	{
		NFmiMetTime aTime(itsTime);
		aTime.ChangeByMinutes(-itsStartTimeRangeInMinutes);
		return aTime;
	}
	else
	{ // backward trajectory
		NFmiMetTime aTime(itsTime);
		aTime.ChangeByMinutes(-itsStartTimeRangeInMinutes);
		aTime.ChangeByHours(-itsTimeLengthInHours);
		return aTime;
	}
}

NFmiMetTime NFmiTrajectory::CalcPossibleLastTime(void)
{
	if(itsDirection == kBackward)
	{
		NFmiMetTime aTime(itsTime);
		aTime.ChangeByMinutes(itsStartTimeRangeInMinutes);
		return aTime;
	}
	else
	{ // forward trajectory
		NFmiMetTime aTime(itsTime);
		aTime.ChangeByMinutes(itsStartTimeRangeInMinutes);
		aTime.ChangeByHours(itsTimeLengthInHours);
		return aTime;
	}
}

bool NFmiTrajectory::Is3DTrajectory(void) const
{
	return itsMainTrajector.Is3DTrajectory();
}

// Laskee mahd. ajassa harvennetut trajektori latlon-pisteet.
// laskuissa k‰ytet‰‰n main-trajektoria
void NFmiTrajectory::CalculateCrossSectionTrajectoryHelpData(void)
{
	itsCrossSectionTrajectoryPoints.clear();
	itsCrossSectionTrajectoryTimes.clear();

	double usedTimeStepInMinutes = 60; // t‰m‰ vaatimus tulee poikkileikkaus n‰ytˆst‰
	int wantedPoints = static_cast<int>((itsTimeLengthInHours*60/usedTimeStepInMinutes) + 1);
	int ssize = static_cast<int>(itsMainTrajector.Points().size());

	// sitten t‰ytet‰‰n apudata vektori
	int skipValue = static_cast<int>(usedTimeStepInMinutes / itsTimeStepInMinutes);
	NFmiMetTime aTime(itsTime);
	aTime.SetTimeStep(static_cast<short>(usedTimeStepInMinutes));
	const checkedVector<NFmiPoint> &points = itsMainTrajector.Points();
	int pointLocationCounter = 0;
	if(itsDirection == kForward)
	{
		for(int i=0; i<wantedPoints; i++)
		{
			itsCrossSectionTrajectoryTimes.push_back(aTime);
			if(pointLocationCounter < ssize)
				itsCrossSectionTrajectoryPoints.push_back(points[pointLocationCounter]);
			else
				itsCrossSectionTrajectoryPoints.push_back(NFmiPoint(kFloatMissing, kFloatMissing));
			pointLocationCounter += skipValue;
			aTime.NextMetTime();
		}
	}
	else // backward tilanteessa pit‰‰ t‰ytt‰‰ trajektori pisteit‰ k‰‰nteisess‰ j‰rjestyksess‰
	{
		itsCrossSectionTrajectoryPoints.resize(wantedPoints);
		itsCrossSectionTrajectoryTimes.resize(wantedPoints);
		for(int i = wantedPoints-1; i>= 0; i--)
		{
			itsCrossSectionTrajectoryTimes[i] = aTime;
			if(pointLocationCounter < ssize && pointLocationCounter >= 0)
				itsCrossSectionTrajectoryPoints[i] = points[pointLocationCounter];
			else
				itsCrossSectionTrajectoryPoints[i] = NFmiPoint(kFloatMissing, kFloatMissing);
			pointLocationCounter += skipValue;
			aTime.PreviousMetTime();
		}
	}
}

double NFmiTrajectory::IsentropicTpotValue(void) const
{
	return itsMainTrajector.IsentropicTpotValue();
}

void NFmiTrajectory::Write(std::ostream& os) const
{
	os << "// NFmiTrajectory::Write..." << std::endl;

	os << "// LatLon" << std::endl;
	os << itsLatLon << std::endl;

	NFmiMetTime usedViewMacroTime = NFmiDataStoringHelpers::GetUsedViewMacroTime();
	os << "// selected time with utc hour + minute + day shift to current day" << std::endl;
	NFmiDataStoringHelpers::WriteTimeWithOffsets(usedViewMacroTime, itsTime, os);

	os << "// data's origin time with utc hour + minute + day shift to current day" << std::endl;
	NFmiDataStoringHelpers::WriteTimeWithOffsets(usedViewMacroTime, itsOriginTime, os); // T‰t‰ ei ehk‰ voi oikeasti k‰ytt‰‰!!!

	os << "// Producer" << std::endl;
	os << itsProducer << std::endl;

	os << "// DataType + TimeStepInMinutes + TimeLengthInHours + PlumeProbFactor + PlumeParticleCount" << std::endl;
	os << itsDataType << " " << itsTimeStepInMinutes << " " << itsTimeLengthInHours << " " << itsPlumeProbFactor << " " << itsPlumeParticleCount << std::endl;

	os << "// StartLocationRangeInKM + PressureLevel + StartPressureLevelRange + IsentropicTpotValue + StartTimeRangeInMinutes" << std::endl;
	os << itsStartLocationRangeInKM << " " << itsPressureLevel << " " << itsStartPressureLevelRange << " " << itsIsentropicTpotValue << " " << itsStartTimeRangeInMinutes << std::endl;

	os << "// Direction + PlumesUsed + Isentropic + fCalcTempBalloonTrajectories" << std::endl;
	os << itsDirection << " " << fPlumesUsed << " " << fIsentropic << " " << fCalcTempBalloonTrajectories << std::endl;

	os << "// TempBalloonTrajectorSettings" << std::endl;
	os << itsTempBalloonTrajectorSettings << std::endl;

	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi viel‰ mahdollinen extra data
	// Kun tulee uusia muuttujia, tee t‰h‰n extradatan t‰yttˆ‰, jotta se saadaan talteen tiedopstoon siten ett‰
	// edelliset versiot eiv‰t mene solmuun vaikka on tullut uutta dataa.
	os << "// possible extra data" << std::endl;
	os << extraData;

	if(os.fail())
		throw std::runtime_error("NFmiTrajectory::Write failed");
}

void NFmiTrajectory::Read(std::istream& is)
{ // toivottavasti olet poistanut kommentit luettavasta streamista!!

	fCalculated = false; // kun trajektori luetaan esim. view-makrosta, se on laskematon

	is >> itsLatLon;

	NFmiMetTime usedViewMacroTime = NFmiDataStoringHelpers::GetUsedViewMacroTime();
	NFmiDataStoringHelpers::ReadTimeWithOffsets(usedViewMacroTime, itsTime, is);

	NFmiDataStoringHelpers::ReadTimeWithOffsets(usedViewMacroTime, itsOriginTime, is); // T‰t‰ ei ehk‰ voi oikeasti k‰ytt‰‰!!!

	is >> itsProducer;

	is >> itsDataType >> itsTimeStepInMinutes >> itsTimeLengthInHours >> itsPlumeProbFactor >> itsPlumeParticleCount;

	is >> itsStartLocationRangeInKM >> itsPressureLevel >> itsStartPressureLevelRange >> itsIsentropicTpotValue >> itsStartTimeRangeInMinutes;

	int tmpValue = 0;
	is >> tmpValue >> fPlumesUsed >> fIsentropic >> fCalcTempBalloonTrajectories;
	itsDirection = static_cast<FmiDirection>(tmpValue);

	is >> itsTempBalloonTrajectorSettings;

	if(is.fail())
		throw std::runtime_error("NFmiTrajectory::Read failed");
	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi viel‰ mahdollinen extra data
	is >> extraData;
	// T‰ss‰ sitten otetaaan extradatasta talteen uudet muuttujat, mit‰ on mahdollisesti tullut
	// eli jos uusia muutujia tai arvoja, k‰sittele t‰ss‰.

	if(is.fail())
		throw std::runtime_error("NFmiTrajectory::Read failed");
}


//_________________________________________________________ NFmiTrajectorySystem
NFmiTrajectorySystem::NFmiTrajectorySystem(NFmiInfoOrganizer *theInfoOrganizer, NFmiProducerSystem *theProducerSystem)
:itsCurrentVersionNumber(itsLatestVersionNumber)
,itsInfoOrganizer(theInfoOrganizer)
,itsProducerSystem(theProducerSystem)
,itsNuclearPlants()
,itsOtherPlaces()
,itsTrajectories()
,itsSelectedLatLon()
,itsSelectedTime()
,itsSelectedProducer()
,itsSelectedDataType(2)
,itsSelectedTimeStepInMinutes(30) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedTimeLengthInHours(24) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedPlumeProbFactor(25) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedPlumeParticleCount(10) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedStartLocationRangeInKM(0) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedPressureLevel(850) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedStartPressureLevelRange(0) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedDirection(kForward) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsSelectedStartTimeRangeInMinutes(0) // siirr‰ t‰m‰ asetus myˆs tiedostoon konffeihin
,itsTrajectoryViewTimeBag()
,fTrajectoryViewTimeBagDirty(true)
,fPlumesUsed(true)
,fTrajectoryViewOn(false)
,fShowTrajectoryArrows(true)
,fShowTrajectoryAnimationMarkers(true)
,fSelectedTrajectoryIsentropic(false)
,fShowTrajectoriesInCrossSectionView(false)
,itsTempBalloonTrajectorSettings()
,fCalcTempBalloonTrajectors(false)
,fUseMapTime(false)
,fTrajectorySaveEnabled(true)
,itsTrajectorySavePath()
,itsTrajectorySaveFilePattern()
,itsLastTrajectoryLegendStr()
{
	if(itsProducerSystem && itsProducerSystem->ExistProducer(1))
	{
		itsSelectedProducer = itsProducerSystem->Producer(1).GetProducer();
	}
}

NFmiTrajectorySystem::~NFmiTrajectorySystem(void)
{
}

void NFmiTrajectorySystem::AddTrajectory(bool fCalculateData)
{
	fTrajectoryViewTimeBagDirty = true;
	boost::shared_ptr<NFmiTrajectory> tmp(boost::shared_ptr<NFmiTrajectory>(new NFmiTrajectory()));
	SetSelectedValuesToTrajectory(tmp, true, false);

	if(fCalculateData)
		CalculateTrajectory(tmp);

	itsTrajectories.push_back(tmp);
}

// laskee kuinka monen aika-askeleen j‰lkeen pit‰‰ aina
// laskea pluumi trajektoreihin haluttu satunnaisuus
static int CalcRandomizerStep(int theTimeStepInMinutes, int theRandomizerStepInMinutes)
{
	double value = static_cast<double>(theRandomizerStepInMinutes) / theTimeStepInMinutes;
	int randStep = round(value);
	if(randStep < 1)
		randStep = 1;
	return randStep;
}

static NFmiMetTime CalcRandStartTime(const NFmiMetTime &theStartTime, double theStartTimeRangeInMinutes, int theRoundStepInMinutes)
{
	double rangeRandValue = theStartTimeRangeInMinutes * static_cast<double>(2*rand()-RAND_MAX) / RAND_MAX; // joku reaali luku v‰lill‰ -1 ja - 1
	long usedRangeRandValue = round(rangeRandValue/theRoundStepInMinutes) * theRoundStepInMinutes;
	NFmiMetTime aTime(theStartTime);
	aTime.SetTimeStep(static_cast<short>(theRoundStepInMinutes));
	aTime.ChangeByMinutes(usedRangeRandValue);
	return aTime;
}

static NFmiPoint CalcRandStartPoint(const NFmiPoint &theStartPoint, double theStartLocationRangeInKM)
{
	double rangeRandValue = theStartLocationRangeInKM * 1000 * static_cast<double>(rand()) / RAND_MAX; // joku reaali luku v‰lill‰ 0 - 1
	double dirRandValue = 360 * static_cast<double>(rand()) / RAND_MAX; // joku reaali luku v‰lill‰ 0 - 1
	NFmiLocation loc(theStartPoint);
	loc.SetLocation(dirRandValue, rangeRandValue);
	return loc.GetLocation();
}

static double CalcRandStartPressureLevel(double theStartPressureLevel, double theStartPressureLevelRange, double theMaxPressureLevel)
{
	double rangeRandValue = theStartPressureLevelRange * static_cast<double>(2*rand()-RAND_MAX) / RAND_MAX; // joku reaali luku v‰lill‰ -1 ja 1
	double value = theStartPressureLevel + rangeRandValue;
	if(value < 1)
		value = 1;
	if(value > theMaxPressureLevel) // ei leikata max-rajaan, vaan 'pompautetaan' takaisin hieman
		value = value + (theMaxPressureLevel - value);
	return value;
}

void NFmiTrajectorySystem::MakeSureThatTrajectoriesAreCalculated(void)
{
	checkedVector<boost::shared_ptr<NFmiTrajectory> >::iterator it = itsTrajectories.begin();
	for( ; it != itsTrajectories.end(); ++it)
	{
		if((*(*it)).Calculated() == false) // lasketaan trajektorit vain jos niit‰ ei ole jo laskettu
			CalculateTrajectory(*it);
	}
}

void NFmiTrajectorySystem::CalculateTrajectory(boost::shared_ptr<NFmiTrajectory> &theTrajectory)
{
	boost::shared_ptr<NFmiFastQueryInfo> info = GetWantedInfo(theTrajectory);
	CalculateTrajectory(theTrajectory, info);
}

void NFmiTrajectorySystem::CalculateTrajectory(boost::shared_ptr<NFmiTrajectory> &theTrajectory, boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
	if(theInfo && theInfo->Param(kFmiWindSpeedMS) && theInfo->Param(kFmiWindDirection))
	{
		theInfo->First();
		theTrajectory->Clear(); // nollataan trajektori varmuuden vuoksi
		theTrajectory->Calculated(true); // merkit‰‰n trajektori lasketuksi
		theTrajectory->OriginTime(theInfo->OriginTime());
		// lasketaan ensin "p‰‰" trajektory
		NFmiTempBalloonTrajectorSettings balloonTrajectorSettings(theTrajectory->TempBalloonTrajectorSettings());
		NFmiSingleTrajector trajector(theTrajectory->LatLon(), theTrajectory->Time(), theTrajectory->PressureLevel());
		CalculateSingleTrajectory(theInfo, trajector, theTrajectory->TimeStepInMinutes(), theTrajectory->TimeLengthInHours(), 0, 0, theTrajectory->Direction(), theTrajectory->Isentropic(), theTrajectory->CalcTempBalloonTrajectories(), balloonTrajectorSettings);
		theTrajectory->MainTrajector(trajector);

		if(theTrajectory->PlumesUsed())
		{ // lasketaan halutut pluumi tarjektorit myˆs
			int trajCount = theTrajectory->PlumeParticleCount();
			int randStep = CalcRandomizerStep(theTrajectory->TimeStepInMinutes(), 30);
			double randFactor = theTrajectory->PlumeProbFactor();
			NFmiMetTime startTime(theTrajectory->Time());
			NFmiMetTime usedTime(startTime);
			NFmiPoint startPoint(theTrajectory->LatLon());
			NFmiPoint usedPoint(startPoint);
			double startPressureLevel = theTrajectory->PressureLevel();
			double usedPressureLevel = startPressureLevel;
			for(int i=0; i<trajCount; i++)
			{
				if(theTrajectory->StartLocationRangeInKM())
					usedPoint = ::CalcRandStartPoint(startPoint, theTrajectory->StartLocationRangeInKM());
				if(theTrajectory->StartTimeRangeInMinutes())
					usedTime = ::CalcRandStartTime(startTime, theTrajectory->StartTimeRangeInMinutes(), 10);
				if(theTrajectory->StartPressureLevelRange())
					usedPressureLevel = ::CalcRandStartPressureLevel(startPressureLevel, theTrajectory->StartPressureLevelRange(), 1000);
				boost::shared_ptr<NFmiSingleTrajector> aTrajector(new NFmiSingleTrajector(usedPoint, usedTime, usedPressureLevel));
				CalculateSingleTrajectory(theInfo, *(aTrajector.get()), theTrajectory->TimeStepInMinutes(), theTrajectory->TimeLengthInHours(), randFactor, randStep, theTrajectory->Direction(), theTrajectory->Isentropic(), theTrajectory->CalcTempBalloonTrajectories(), balloonTrajectorSettings);
				theTrajectory->AddPlumeTrajector(aTrajector);
			}
		}
	}
}

// Oletus: value ei ole kFloatMissing!
// Laskee annetusta WS:st‰ halutun suuruisen +- muutoksen.
// Lis‰ksi varmistaa ettei ole negatiivinen nopeus.
// randFactor on satunnaisuuden suurin vaihtelu arvo verrattuna annettuun WS:‰‰n.
static double RandomizeWSValue(double WS, double randFactor)
{
	double randValue = (static_cast<double>(2*rand()) - RAND_MAX) / RAND_MAX; // joku reaali luku v‰lill‰ -1 - 1
	double modifyValue = WS * randValue * randFactor * 0.01; // muutetaan luku oikeaksi muutos arvoksi
	return modifyValue;
}

static double WDAdd(double WD, double changeValue, double maxValue)
{
	double result = WD + changeValue;
	if(result < 0)
		result += maxValue;
	if(result > maxValue)
		result = ::fmod(result, maxValue);
	return result;
}

// Oletus: value ei ole kFloatMissing!
// Laskee annetusta WS:st‰ halutun suuruisen +- muutoksen.
// Lis‰ksi varmistaa ettei ole negatiivinen nopeus.
// randFactor on satunnaisuuden suurin vaihtelu arvo verrattuna annettuun WS:‰‰n.
static double RandomizeWDValue(double randFactor, double maxValue)
{
	double randValue = (static_cast<double>(2*rand()) - RAND_MAX) / RAND_MAX; // joku reaali luku v‰lill‰ -1 - 1
	// min/max muutos on 1/3 osa maxValuesta
	double modifyValue = maxValue * 0.33 * randValue * randFactor * 0.01; // muutetaan luku oikeaksi muutos arvoksi ottaen huomioon max arvo
	return modifyValue;
}

static double RandomizewValue(double w, double randFactor)
{
	double randValue = (static_cast<double>(2*rand()) - RAND_MAX) / RAND_MAX; // joku reaali luku v‰lill‰ -1 - 1
	double modifyValue = w * randValue * randFactor * 0.01; // muutetaan luku oikeaksi muutos arvoksi
	return modifyValue;
}

// Oletus: theInfo on jo tarkistettu.
void NFmiTrajectorySystem::CalculateSingleTrajectory(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, NFmiSingleTrajector &theTrajector, int theTimeStepInMinutes, int theTimeLengthInHours, double theRandFactor, int theRandStep, FmiDirection theDirection, bool fIsentropic, bool fCalcBalloonTrajectory, NFmiTempBalloonTrajectorSettings &theTempBalloonTrajectorSettings)
{
	if(theInfo->SizeLevels() > 1)
		CalculateSingle3DTrajectory(theInfo, theTrajector, theTimeStepInMinutes, theTimeLengthInHours, theRandFactor, theRandStep, theDirection, fIsentropic, fCalcBalloonTrajectory, theTempBalloonTrajectorSettings);
	else
	{
		bool forwardDir = (theDirection == kForward);
		theTrajector.AddPoint(theTrajector.StartLatLon()); // 1. piste pit‰‰ lis‰t‰ erikseen listaan
		NFmiLocation currentLoc(theTrajector.StartLatLon());
		NFmiLocation nextLoc;
		NFmiMetTime startTime(theTrajector.StartTime());
		NFmiMetTime currentTime(startTime);
		currentTime.SetTimeStep(static_cast<short>(theTimeStepInMinutes));
		NFmiMetTime endTime(startTime);
		endTime.ChangeByHours(forwardDir ? theTimeLengthInHours : -theTimeLengthInHours);
		double WS = kFloatMissing;
		double WSdiff = 0;
		double WD = kFloatMissing;
		double WDdiff = 0;
		int index = 0;
		for( ; forwardDir ? (currentTime <= endTime) : (currentTime >= endTime) ; forwardDir ? currentTime.NextMetTime() : currentTime.PreviousMetTime() )
		{
			theInfo->Param(kFmiWindSpeedMS);
			WS = theInfo->InterpolatedValue(currentLoc.GetLocation(), currentTime);
			theInfo->Param(kFmiWindDirection);
			WD = theInfo->InterpolatedValue(currentLoc.GetLocation(), currentTime);
			if(WS == kFloatMissing || WD == kFloatMissing)
				break;
			if(theRandStep && (index % theRandStep) == 0)
			{
				WSdiff = RandomizeWSValue(WS, theRandFactor);
				WDdiff = RandomizeWDValue(theRandFactor, 360);
			}
			WS = ::fabs(WS+WSdiff); // fabs = varmistetaan ett‰ lopullinen nopeus aina positiivinen
			WD = ::WDAdd(WD, WDdiff, 360);
			double dist = WS * theTimeStepInMinutes * 60; // saadaan kuljettu matka metrein‰
			double dir = ::fmod(WD + 180, 360);
			if(!forwardDir)
				dir = ::fmod(WD, 360); // backwardissa k‰‰nnet‰‰n tuuli 180 astetta
			nextLoc = currentLoc.GetLocation(dir, dist);
			theTrajector.AddPoint(nextLoc.GetLocation());
			currentLoc = nextLoc;
			index++;
		}
	}
}

static bool IsInfoHybridData(boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
	bool hybridData = theInfo->DataType() == NFmiInfoData::kHybridData;
	if(theInfo->DataType() == NFmiInfoData::kEditable)
	{
		theInfo->FirstLevel();
		if(theInfo->Level()->LevelType() == kFmiHybridLevel)
			hybridData = true;
	}
	return hybridData;
}

static unsigned long GetInfoGroundLevelIndex(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, bool hybridData)
{
	unsigned long groundLevelIndex = static_cast<unsigned long>(-1);
	if(hybridData)
	{
		if(theInfo->PressureValueAvailable())
		{
			if(!theInfo->PressureParamIsRising())
				groundLevelIndex = 0;
			else
				groundLevelIndex = theInfo->SizeLevels() - 1;
		}
	}
	else
	{	// muuten painepinta dataa
		if(theInfo->PressureLevelDataAvailable())
		{
			if(!theInfo->PressureParamIsRising())
				groundLevelIndex = 0;
			else
				groundLevelIndex = theInfo->SizeLevels() - 1;
		}
	}
	return groundLevelIndex;
}

static unsigned long GetInfoTopLevelIndex(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, bool hybridData)
{
	unsigned long topLevelIndex = static_cast<unsigned long>(-1);
	if(hybridData)
	{
		if(theInfo->PressureValueAvailable())
		{
			if(theInfo->PressureParamIsRising())
				topLevelIndex = 0;
			else
				topLevelIndex = theInfo->SizeLevels() - 1;
		}
	}
	else
	{	// muuten painepinta dataa
		if(theInfo->PressureLevelDataAvailable())
		{
			if(theInfo->PressureParamIsRising())
				topLevelIndex = 0;
			else
				topLevelIndex = theInfo->SizeLevels() - 1;
		}
	}
	return topLevelIndex;
}

static FmiParameterName GetInfoUsedVerticalVelotityParam(boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
	FmiParameterName usedWParam = kFmiBadParameter;
	if(theInfo->Param(kFmiVelocityPotential))
		usedWParam = kFmiVelocityPotential;
	else if(theInfo->Param(kFmiVerticalVelocityMMS))
		usedWParam = kFmiVerticalVelocityMMS;
	return usedWParam;
}

void NFmiTrajectorySystem::Make3DRandomizing(double &WS, double &WD, double &w, int theRandStep, int theCounter, double theRandomFactor, NFmiSingleTrajector &theTrajector)
{
	if(theRandStep && (theCounter % theRandStep) == 0)
	{ // halutuin v‰liajoin lasketaan satunnaisuus 'korjaus' luvut joilla rukataan currentteja suuntia ja nopeuksia aina
	  // eli ei lasketa satunnais lukuja joka aika askeleella ett‰ ei tule hirveet‰ syherˆ‰, vaan aina tietyin v‰liajoin
		double WSdiff = RandomizeWSValue(WS, theRandomFactor);
		double WDdiff = RandomizeWDValue(theRandomFactor, 360);
		double wdiff = RandomizewValue(w, theRandomFactor);
		theTrajector.SetRandomValues(WDdiff, WSdiff, wdiff);
	}
	WS = ::fabs(WS + theTrajector.RandWSdiff()); // fabs = varmistetaan ett‰ lopullinen nopeus aina positiivinen
	WD = ::WDAdd(WD, theTrajector.RandWDdiff(), 360);
	w += theTrajector.Randwdiff();
}

static bool MakeGroundAdjustment(double &WS, double &WD, double &w, double &P, boost::shared_ptr<NFmiFastQueryInfo> &theInfo, double xInd, double yInd, double tInd, unsigned long theLevelIndex, bool isHybridData, FmiParameterName theUsedWParam)
{
	if(WS == kFloatMissing || WD == kFloatMissing || w == kFloatMissing)
	{ // jos joku n‰ist‰ puuttuvaa, voi olla ett‰ ollaan menty maan "sis‰‰n" ja pit‰‰ laskea pinta arvot
		double limitPressure = isHybridData ? 600 : 900;  // hybridi, 600 mb onko esim. Alpit 500 mb asti?!?!, painepinta datalle lasketaan data 1000mb:en joka tapauksessa
		if(P > limitPressure)
		{ // jos ollaan tarpeeksi l‰hell‰ maanpintaa (hybridi datan ollessa kyseess‰)

			theInfo->Param(kFmiPressure);
			float groundP = theInfo->FastPressureLevelValue(xInd, yInd, tInd, theLevelIndex);
			theInfo->Param(kFmiWindSpeedMS);
			float groundWS = theInfo->FastPressureLevelValue(xInd, yInd, tInd, theLevelIndex);
			theInfo->Param(kFmiWindDirection);
			float groundWD = theInfo->FastPressureLevelValue(xInd, yInd, tInd, theLevelIndex);
			theInfo->Param(theUsedWParam);
			float groundw = theInfo->FastPressureLevelValue(xInd, yInd, tInd, theLevelIndex);

			if(groundP == kFloatMissing || groundWS == kFloatMissing || groundWD == kFloatMissing || groundw == kFloatMissing)
				return false; // eli nyt on data loppunut joko ajallisesti, alueellisesti tai korkeudellisesti
			else
			{
				P = groundP;
				WS = groundWS;
				WD = groundWD;
				w = groundw;
			}
		}
		else // ollaan liian korkealla ett‰ haluttaisiin laskea maanpinta arvoja
			return false;
	}
	return true;
}

static NFmiLocation CalcNewLocation(const NFmiLocation &theCurrentLocation, double WS, double WD, int theTimeStepInMinutes, bool isForwardDir)
{
		double dist = WS * theTimeStepInMinutes * 60; // saadaan kuljettu matka metrein‰
		double dir = ::fmod(isForwardDir ? (WD + 180) : WD, 360); // jos backward trajectory pit‰‰ k‰‰nt‰‰ virtaus suunta 180 asteella
		return theCurrentLocation.GetLocation(dir, dist);
}


static double CalcNewPressureLevel(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, double theCurrentPressure, double w, double xInd, double yInd, double tInd, unsigned long theGroundLevelIndex, bool isForwardDir, int theTimeStepInMinutes, bool hybridData)
{
	// paine muutos lasku (w on mm/s, josta saa muutettua hPa/s jakamalla -9300:lla (kysy Viljolta))
	double deltaP = w /(isForwardDir ? -9300.0 : 9300.0) * theTimeStepInMinutes * 60;   // jos backward trajectory pit‰‰ k‰‰nt‰‰ virtaus suunta vertikaali suunnassa
	double maxP = 1000;
	if(hybridData)
	{ // pyydet‰‰n maanpinta levelin paine maanpinta arvoksi
		theInfo->Param(kFmiPressure);
		maxP = theInfo->FastPressureLevelValue(xInd, yInd, tInd, theGroundLevelIndex);
	}

	double nextPressure = kFloatMissing;
	// laitetaan saatu paine maanpinnan ja 1 hPa:n v‰liin
	nextPressure = FmiMin(maxP, theCurrentPressure+deltaP);

	return nextPressure;
}

static double CalcNewPressureLevelWithBalloon(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double theCurrentPressure, unsigned long thePressureParamIndex, double xInd, double yInd, double tInd, double pInd, unsigned long theGroundLevelIndex, unsigned long /* theTopLevelIndex */ , bool isForwardDir, int theTimeStepInMinutes, bool hybridData, NFmiTempBalloonTrajectorSettings &theTempBalloonTrajectorSettings)
{
	theInfo->ParamIndex(theInfo->HeightParamIndex());
	double Z = theInfo->FastPressureLevelValue(xInd, yInd, tInd, pInd);
	double deltaP = theTempBalloonTrajectorSettings.CalcDeltaP(theInfo, theLatlon, theTime, theCurrentPressure, thePressureParamIndex, Z, theTimeStepInMinutes, theGroundLevelIndex);
	if(!isForwardDir)
		deltaP = -deltaP;
	double maxP = 1000;
	if(hybridData)
	{ // pyydet‰‰n maanpinta levelin paine maanpinta arvoksi

		theInfo->LevelIndex(theGroundLevelIndex);
		theInfo->ParamIndex(thePressureParamIndex);
		maxP = theInfo->InterpolatedValue(theLatlon, theTime);
	}

	double nextPressure = kFloatMissing;
	// laitetaan saatu paine maanpinnan ja 1 hPa:n v‰liin
	nextPressure = theCurrentPressure+deltaP;

	if(nextPressure > maxP || nextPressure < 1)
		return kFloatMissing;
	return nextPressure;
}

// onko value limittien v‰liss‰. limitit voivat olla miss‰ j‰rjestyksess‰ tahansa ja
// ne voivat olla puuttuvia, jolloin palautetaan false
// Oletus: value ei voi olla puuttuva.
static bool IsValueBetween(double value, double limit1, double limit2)
{
	if(limit1 == kFloatMissing || limit2 == kFloatMissing)
		return false;
	if(limit1 >= value && value >= limit2)
		return true;
	if(limit2 >= value && value >= limit1)
		return true;
	return false;
}

// kun tiedet‰‰n lineaarinen arvo ja halutaan laskea sit‰ vastaava logaritmisen P:n arvo
static double CalcLogPFromLinearValues(double v1, double v2, double v, double p1, double p2)
{
	double w = (v-v1)/(v2-v1);
	double p = ::exp(w*(::log(p2)- ::log(p1)) + ::log(p1));
	return p;
}

// palauttaa kurrenttiin leveliin paine arvon haluttuun kohtaan ja aikaan
static double GetPressureValue(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, double xInd, double yInd, double tInd, double pInd, bool hybridData, unsigned long thePressureParamIndex)
{
	if(hybridData)
	{
		unsigned long oldParamIndex = theInfo->ParamIndex();
		theInfo->ParamIndex(thePressureParamIndex);
		double p = theInfo->FastPressureLevelValue(xInd, yInd, tInd, pInd);
		theInfo->ParamIndex(oldParamIndex);
		return p;
	}
	else // muuten oletetaan ett‰ kyseess‰ on painepinta dataa
		return theInfo->Level()->LevelValue();
}

// Oletus: theWantedTpot ei voi olla puuttuva.
static double CalcNewPressureLevelIsentropically(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiLocation &theLoc, const NFmiMetTime &theTime, double theWantedTpot, unsigned long theTpotParamIndex, unsigned long thePressureParamIndex, double theStartPSearchIndex, bool hybridData)
{
	double xInd = 0;
	double yInd = 0;
	double tInd = 0;
	bool status1 = theInfo->GetLocationIndex(theLoc.GetLocation(), xInd, yInd);
	bool status2 = theInfo->GetTimeIndex(theTime, tInd);
	if(status1 == false || status2 == false)
		return kFloatMissing;
	theInfo->ParamIndex(theTpotParamIndex);
	unsigned long levelIndex1 = static_cast<unsigned long>(theStartPSearchIndex);
	bool isPIndexInBetween = theInfo->SizeLevels() >= levelIndex1 + 1;
	levelIndex1 = isPIndexInBetween ? levelIndex1 : levelIndex1-1;
	unsigned long levelIndex2 = isPIndexInBetween ? levelIndex1+1 : levelIndex1;
	double value1 = theInfo->FastPressureLevelValue(xInd, yInd, tInd, levelIndex1);
	double P1 = GetPressureValue(theInfo, xInd, yInd, tInd, levelIndex1, hybridData, thePressureParamIndex);
	double value2 = theInfo->FastPressureLevelValue(xInd, yInd, tInd, levelIndex2);
	double P2 = GetPressureValue(theInfo, xInd, yInd, tInd, levelIndex2, hybridData, thePressureParamIndex);
	double nextPressure = kFloatMissing;
	if(IsValueBetween(theWantedTpot, value1, value2))
		nextPressure = ::CalcLogPFromLinearValues(value1, value2, theWantedTpot, P1, P2);
	else
	{ // etsit‰‰n leveli‰ mist‰ haluttu tpot lˆytyy
		bool valueRisingInLevels = value2 > value1;
		bool firstDirectionUp = (valueRisingInLevels && (theWantedTpot > value1));
		unsigned long startLevel = levelIndex1; // otetaan muistiin aloitus kerros, jos pit‰‰ etsi‰ toisesta suunnasta
		double startP = P1;
		double startValue = value1;
		do
		{
			value1 = value2;
			P1 = P2;
			levelIndex1 = levelIndex2;
			levelIndex2 = (firstDirectionUp) ? ++levelIndex2  : --levelIndex2;
			value2 = theInfo->FastPressureLevelValue(xInd, yInd, tInd, levelIndex2);
			P2 = GetPressureValue(theInfo, xInd, yInd, tInd, levelIndex2, hybridData, thePressureParamIndex);
			if(IsValueBetween(theWantedTpot, value1, value2))
			{
				nextPressure = ::CalcLogPFromLinearValues(value1, value2, theWantedTpot, P1, P2);
				return nextPressure;
			}
		} while(levelIndex2 > 0 && levelIndex2 < theInfo->SizeLevels());

		// jos ei lˆytynyt toisesta suunnasta, kokeillaan toista suuntaa
		levelIndex2 = startLevel;
		P2 = startP;
		value2 = startValue;
		do
		{
			value1 = value2;
			P1 = P2;
			levelIndex1 = levelIndex2;
			levelIndex2 = (firstDirectionUp) ? --levelIndex2  : ++levelIndex2; // t‰ss‰ piti k‰‰nt‰‰ level laskuria!
			value2 = theInfo->FastPressureLevelValue(xInd, yInd, tInd, levelIndex2);
			P2 = GetPressureValue(theInfo, xInd, yInd, tInd, levelIndex2, hybridData, thePressureParamIndex);
			if(IsValueBetween(theWantedTpot, value1, value2))
			{
				nextPressure = ::CalcLogPFromLinearValues(value1, value2, theWantedTpot, P1, P2);
				return nextPressure;
			}
		} while(levelIndex2 > 0 && levelIndex2 < theInfo->SizeLevels());
	}

	return nextPressure;
}

// katsoo onko annettu paine currentPressure alle hybridi-datan pinnan
// jos on, laskee uuden paineen maanpinnalle ja sijoittaa arvon currentPressure
// parametriin, muuten ei tee mit‰‰n.
static void CalcStartingPointGroundAdjustment(NFmiFastQueryInfo &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, unsigned long groundLevelIndex, bool hybridData, double &currentPressure, double &theHeightValue)
{
	if(currentPressure != kFloatMissing && hybridData && theInfo.LevelIndex(groundLevelIndex))
	{
		if(theInfo.Param(kFmiPressure))
		{
			theInfo.ParamIndex(theInfo.PressureParamIndex());
			double groundPressure = theInfo.InterpolatedValue(theLatlon, theTime);
			if(groundPressure != kFloatMissing && currentPressure > groundPressure)
			{
				currentPressure = groundPressure;
				theInfo.ParamIndex(theInfo.HeightParamIndex());
				theHeightValue = theInfo.InterpolatedValue(theLatlon, theTime);
			}
		}
	}
}

static float GetHeightValueForNewPressure(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, const NFmiPoint &theLatlon, const NFmiMetTime &theTime, double thePressure, unsigned long theGroundLevelIndex)
{
	theInfo->ParamIndex(theInfo->HeightParamIndex());
	float heightValue = theInfo->PressureLevelValue(static_cast<float>(thePressure), theLatlon, theTime);
	if(heightValue == kFloatMissing)
	{ // kokeillaan lˆytyykˆ sitten pinta arvoa, jos paineen avulla haettu meni esim. tarkkuus ongelman takia juuri alle alimman kerroksen
		theInfo->LevelIndex(theGroundLevelIndex);
		heightValue = theInfo->InterpolatedValue(theLatlon, theTime);
	}
	return heightValue;
}

// Oletus: theInfo on jo tarkistettu.
// theConstantW [m/s]
void NFmiTrajectorySystem::CalculateSingle3DTrajectory(boost::shared_ptr<NFmiFastQueryInfo> &theInfo, NFmiSingleTrajector &theTrajector, int theTimeStepInMinutes, int theTimeLengthInHours, double theRandFactor, int theRandStep, FmiDirection theDirection, bool fIsentropic, bool fCalcBalloonTrajectory, NFmiTempBalloonTrajectorSettings &theTempBalloonTrajectorSettings)
{
	// jos hybrid dataa, voidaan maanpinta laskea alimman painepinnan paineen avulla
	// jos ei, eli painepinta dataa, pidet‰‰n 1000 hPa:ta maanpintana
	bool hybridData = IsInfoHybridData(theInfo);
	unsigned long groundLevelIndex = ::GetInfoGroundLevelIndex(theInfo, hybridData);
	unsigned long topLevelIndex = ::GetInfoTopLevelIndex(theInfo, hybridData);
	bool forwardDir = (theDirection == kForward);

	if(fIsentropic && (!theInfo->Param(kFmiPotentialTemperature)))
		return ; // haluttiin isentrooppista trajektoria, mutta datassa ei ole pot-T:t‰ ei jatketa
	if(fIsentropic && fCalcBalloonTrajectory)
		return ; // haluttiin isentrooppista trajektoria ja samalla luotauspallo juttua, ei toimi!
	unsigned long tpotParamIndex = theInfo->ParamIndex();
	if(hybridData && (fIsentropic || fCalcBalloonTrajectory) && (!theInfo->Param(kFmiPressure)))
		return ; // haluttiin isentrooppista trajektoria, oli hybridi-dataa mutta datassa ei ole paineparametria ei jatketa
	unsigned long pressureParamIndex = theInfo->ParamIndex();

	FmiParameterName usedWParam = GetInfoUsedVerticalVelotityParam(theInfo);
	if(usedWParam == kFmiBadParameter)
		return ; // ei ole vertikaalinopeus parametria, ei voi jatkaa

	double currentPressure = theTrajector.StartPressureLevel();
	NFmiLocation currentLoc(theTrajector.StartLatLon());
	NFmiMetTime startTime(theTrajector.StartTime());
	theInfo->ParamIndex(theInfo->HeightParamIndex());
	double heightValue = theInfo->PressureLevelValue(static_cast<float>(currentPressure), currentLoc.GetLocation(), startTime);
	double nextPressure = currentPressure;
	if(!fCalcBalloonTrajectory)
	{
		// aloituspisteen paine pit‰‰ fiksata jos kyse hybridi datasta ja aloitus piste on alempana kuin datan alin mallipinta
		::CalcStartingPointGroundAdjustment(*theInfo, theTrajector.StartLatLon(), theTrajector.StartTime(), groundLevelIndex, hybridData, currentPressure, heightValue);
		theTrajector.AddPoint(theTrajector.StartLatLon(), static_cast<float>(currentPressure), static_cast<float>(heightValue)); // 1. piste pit‰‰ lis‰t‰ erikseen listaan
	}
	NFmiLocation nextLoc;
	NFmiMetTime currentTime(startTime);
	currentTime.SetTimeStep(static_cast<short>(theTimeStepInMinutes));
	NFmiMetTime endTime(startTime);
	endTime.ChangeByHours(forwardDir ? theTimeLengthInHours : -theTimeLengthInHours);
	double WS = kFloatMissing;
	double WD = kFloatMissing;
	double w = kFloatMissing;
	double isentropicTpotValue = kFloatMissing; // jos isentrooppi trajektori, pysyt‰‰n t‰ll‰ potentiaali tasolla
	int index = 0;
	double xInd = 0;
	double yInd = 0;
	double tInd = 0;
	double pInd = 0;
	NFmiPoint currentLatLon;
	theTempBalloonTrajectorSettings.Reset();

	for( ; forwardDir ? (currentTime < endTime) : (currentTime > endTime) ;  )
	{
		currentLatLon = currentLoc.GetLocation();
		bool status1 = theInfo->GetLocationIndex(currentLatLon, xInd, yInd);
		bool status2 = theInfo->GetTimeIndex(currentTime, tInd);

		if(fCalcBalloonTrajectory && status1 && status2 && theTempBalloonTrajectorSettings.State() == kBase)
		{
			// pallo laskuissa aloitetaan aina pinnalta
			theInfo->ParamIndex(pressureParamIndex);
			theInfo->LevelIndex(groundLevelIndex);
			nextPressure = currentPressure = theInfo->InterpolatedValue(currentLatLon, currentTime);
			theInfo->ParamIndex(theInfo->HeightParamIndex());
			heightValue = theInfo->InterpolatedValue(currentLatLon, currentTime);
			theTrajector.AddPoint(theTrajector.StartLatLon(), static_cast<float>(currentPressure), static_cast<float>(heightValue)); // 1. piste pit‰‰ lis‰t‰ erikseen listaan
		}
		bool status3 = theInfo->GetLevelIndex(currentLatLon, currentTime, currentPressure, pInd);
		if(status1 && status2 && status3)
		{
			theInfo->Param(kFmiWindSpeedMS);
			WS = theInfo->FastPressureLevelValue(xInd, yInd, tInd, pInd);
			theInfo->Param(kFmiWindDirection);
			WD = theInfo->FastPressureLevelValue(xInd, yInd, tInd, pInd);
			theInfo->Param(usedWParam);
			w = theInfo->FastPressureLevelValue(xInd, yInd, tInd, pInd);
			if(fIsentropic && index == 0)
			{
				theInfo->ParamIndex(tpotParamIndex);
				isentropicTpotValue = theInfo->FastPressureLevelValue(xInd, yInd, tInd, pInd);
				if(isentropicTpotValue == kFloatMissing)
					break;
				theTrajector.IsentropicTpotValue(isentropicTpotValue);
			}
		}
		else if(status1 == false || status2 == false)
			break;

		if(!::MakeGroundAdjustment(WS, WD, w, currentPressure, theInfo, xInd, yInd, tInd, groundLevelIndex, hybridData, usedWParam))
			break;
		Make3DRandomizing(WS, WD, w, theRandStep, index, theRandFactor, theTrajector);
		nextLoc = ::CalcNewLocation(currentLoc, WS, WD, theTimeStepInMinutes, forwardDir);

		(forwardDir) ? currentTime.NextMetTime() : currentTime.PreviousMetTime(); // isentrooppi laskuja varten pit‰‰ laskea seuraava aika
		if(fIsentropic)
			nextPressure = ::CalcNewPressureLevelIsentropically(theInfo, nextLoc, currentTime, isentropicTpotValue, tpotParamIndex, pressureParamIndex, pInd, hybridData);
		else if(fCalcBalloonTrajectory)
			nextPressure = ::CalcNewPressureLevelWithBalloon(theInfo, nextLoc.GetLocation(), currentTime, currentPressure, pressureParamIndex, xInd, yInd, tInd, pInd, groundLevelIndex, topLevelIndex, forwardDir, theTimeStepInMinutes, hybridData, theTempBalloonTrajectorSettings);
		else
			nextPressure = ::CalcNewPressureLevel(theInfo, currentPressure, w, xInd, yInd, tInd, groundLevelIndex, forwardDir, theTimeStepInMinutes, hybridData);
		if(nextPressure == kFloatMissing)
			break;
		heightValue = ::GetHeightValueForNewPressure(theInfo, nextLoc.GetLocation(), currentTime, nextPressure, groundLevelIndex);
		theTrajector.AddPoint(nextLoc.GetLocation(), static_cast<float>(nextPressure), static_cast<float>(heightValue));

		currentLoc = nextLoc;
		currentPressure = nextPressure;
		index++;
	}
}

static void SetFastInfoToZero(boost::shared_ptr<NFmiFastQueryInfo> &theOwnerInfo)
{
	theOwnerInfo = boost::shared_ptr<NFmiFastQueryInfo>(static_cast<NFmiFastQueryInfo*>(0));
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiTrajectorySystem::GetWantedInfo(boost::shared_ptr<NFmiTrajectory> &theTrajectory)
{
	NFmiInfoData::Type dataType = NFmiInfoData::kViewable;
	if(theTrajectory->DataType() == 2)
		dataType = NFmiInfoData::kHybridData;
	else if(theTrajectory->DataType() == 3)
		dataType = NFmiInfoData::kTrajectoryHistoryData;
	bool useGroundData = false;
	if(theTrajectory->DataType() == 0)
		useGroundData = true;
	boost::shared_ptr<NFmiFastQueryInfo> tmp = itsInfoOrganizer->FindInfo(dataType, theTrajectory->Producer(), useGroundData);
	if(tmp == 0 && theTrajectory->Producer().GetIdent() == kFmiMETEOR)
	{ // Jos ei lˆytynyt infoa ja jos editoitu oli valittu dataksi
	  // pit‰‰ etsi‰ editoitavasta datasta.
		// T‰m‰ palauttaa mink‰ tahansa editoitavan datan eik‰ tarkastele tyyppeja tai leveleit‰!!
		tmp = itsInfoOrganizer->FindInfo(NFmiInfoData::kEditable, theTrajectory->Producer(), useGroundData);
		if(tmp)
		{
			if(useGroundData)
			{
				if(tmp->SizeLevels() > 1)
					::SetFastInfoToZero(tmp);
			}
			else
			{
				tmp->FirstLevel();
				FmiLevelType lType = tmp->Level()->LevelType();
				if(dataType == NFmiInfoData::kHybridData && lType != kFmiHybridLevel)
					::SetFastInfoToZero(tmp);
				else if(dataType == NFmiInfoData::kViewable && lType != kFmiPressureLevel)
					::SetFastInfoToZero(tmp);
				else if(dataType == NFmiInfoData::kTrajectoryHistoryData)
					::SetFastInfoToZero(tmp); // editoirdusta datasta ei saa t‰t‰
			}
		}
	}
	return tmp;
}

void NFmiTrajectorySystem::ClearTrajectories(void)
{
	fTrajectoryViewTimeBagDirty = true;
	itsTrajectories.clear();
}

// pit‰‰ p‰‰tell‰ tuottaja id:n perusteella mik‰ on halutun dadio buttonin indeksi alkaa 0
static const NFmiProducer GetSelectedProducer(NFmiProducerSystem &theProducerSystem, int theRadioIndex)
{
	if(theProducerSystem.Producers().size() > static_cast<size_t>(theRadioIndex))
		return theProducerSystem.Producer(theRadioIndex+1).GetProducer();
	else
		return theProducerSystem.Producer(1).GetProducer(); // otetaan 1. tuottaja, jos voidaan
}

static void InitializeSilamStationList(NFmiSilamStationList &theStationList, const std::string &theFileName)
{
	if(theFileName.empty()) // jos ei ole annettu asetuksissa tiedoston nime‰, ei heitet‰ poikkeusta. Muissa virheiss‰ (esim. tiedostoa ei lˆydy) lent‰‰ kyll‰.
		return ;
	theStationList.Clear();
	theStationList.Init(theFileName);
}

void NFmiTrajectorySystem::InitializeFromSettings(void)
{
	int model = NFmiSettings::Require<int>("MetEditor::TrajectorySystem::Model");
	itsSelectedProducer = GetSelectedProducer(*itsProducerSystem, model);
	itsSelectedDataType = NFmiSettings::Require<int>("MetEditor::TrajectorySystem::ModelType");

	fTrajectorySaveEnabled = NFmiSettings::Require<bool>("MetEditor::TrajectorySystem::TrajectorySaveEnabled");
	itsTrajectorySavePath = NFmiSettings::Require<std::string>("MetEditor::TrajectorySystem::TrajectorySavePath");
	itsTrajectorySaveFilePattern = NFmiSettings::Require<std::string>("MetEditor::TrajectorySystem::TrajectorySaveFilePattern");
	itsLastTrajectoryLegendStr = NFmiSettings::Require<std::string>("MetEditor::TrajectorySystem::LastTrajectoryLegendStr");

	std::string silamPlantFileName = NFmiSettings::Optional<std::string>("MetEditor::TrajectorySystem::SilamPlantFile", "");
	InitializeSilamStationList(itsNuclearPlants, silamPlantFileName);
	std::string silamOtherPlacesFileName = NFmiSettings::Optional<std::string>("MetEditor::TrajectorySystem::SilamOtherPlacesFile", "");
	InitializeSilamStationList(itsOtherPlaces, silamOtherPlacesFileName);
}

// Huom! t‰m‰ asettaa kaikki talletettavat asiat settingseihin, mutta ei tee save:a.
void NFmiTrajectorySystem::StoreSettings(void)
{
	int model = itsProducerSystem->FindProducerInfo(itsSelectedProducer);
	NFmiSettings::Set("MetEditor::TrajectorySystem::Model", NFmiStringTools::Convert(model-1));
	NFmiSettings::Set("MetEditor::TrajectorySystem::ModelType", NFmiStringTools::Convert(itsSelectedDataType));

	// huom! trajektori save juttuja ei ainakaan toistaiseksi talleteta takaisin
//	fTrajectorySaveEnabled = NFmiSettings::Require<bool>("MetEditor::TrajectorySystem::TrajectorySaveEnabled");
//	itsTrajectorySavePath = NFmiSettings::Require<std::string>("MetEditor::TrajectorySystem::TrajectorySavePath");
//	itsTrajectorySaveFilePattern = NFmiSettings::Require<std::string>("MetEditor::TrajectorySystem::TrajectorySaveFilePattern");
// HUOM2: itsLastTrajectoryLegendStr ei voi tallettaa takaisin konffeihin, koska NFmiSettings-systeemi ei tue monirivisi‰ arvoja asetuksilla
//	itsLastTrajectoryLegendStr = NFmiSettings::Require<std::string>("MetEditor::TrajectorySystem::LastTrajectoryLegendStr");

}

std::string NFmiTrajectorySystem::MakeCurrentTrajectorySaveFileName(void)
{
	NFmiMetTime currentTime(1);
	// t‰ss‰ siis rakennetaan ajan perusteella tiedosto nimi, olettaen ett‰ itsTrajectorySaveFilePattern
	// sis‰lt‰‰ ToStr:n mukaiset aikapatternit
	std::string fileNameWithTimeStamp = currentTime.ToStr(itsTrajectorySaveFilePattern, kFinnish);
	std::string finalFileName = itsTrajectorySavePath;
	if(finalFileName.size())
	{
		// jos on polku ja se ei p‰‰ty hakemiston erottimeen, lis‰t‰‰n se
		if(!(finalFileName[finalFileName.size()-1] == '\\' || finalFileName[finalFileName.size()-1] == '/'))
			finalFileName += kFmiDirectorySeparator;
	}
	finalFileName += fileNameWithTimeStamp;
	return finalFileName;
}

static bool KeepLevelSettingsForTrajektories(checkedVector<boost::shared_ptr<NFmiTrajectory> > &theTrajectoryList)
{
	size_t trajectoryCount = theTrajectoryList.size();
	if(trajectoryCount > 1)
	{
		std::set<unsigned long> producers;
		std::set<double> levels;
		std::set<int> dataTypes;
		checkedVector<boost::shared_ptr<NFmiTrajectory> >::iterator it = theTrajectoryList.begin();
		for( ; it != theTrajectoryList.end(); ++it)
		{
			producers.insert((*it)->Producer().GetIdent());
			levels.insert((*it)->PressureLevel());
			dataTypes.insert((*it)->DataType());
		}
		if(trajectoryCount > producers.size() && levels.size() > 1)
			return true;
	}
	return false;
}

// Asetetaan tietyt valitut arvot kaikille trajektoreille, mutta ei esim. mallia tai datatyyppi‰!
// Asetetaan mm. alkupiste, alkuaika jne.
void NFmiTrajectorySystem::SetSelectedValuesToAllTrajectories(void)
{
	fTrajectoryViewTimeBagDirty = true;
	// miten saisi t‰ll‰isen mem_fun virityksen toimimaan for_each:ille?!?!?!?
//	std::for_each(itsTrajectories.begin(), itsTrajectories.end(), std::mem_fun(&NFmiTrajectorySystem::SetSelectedValuesToTrajectory));
	bool keepLevelSettings = KeepLevelSettingsForTrajektories(itsTrajectories);
	checkedVector<boost::shared_ptr<NFmiTrajectory> >::iterator it = itsTrajectories.begin();
	for( ; it != itsTrajectories.end(); ++it)
	{
		SetSelectedValuesToTrajectory(*it, false, keepLevelSettings); // false = tuottaja ja data tyyppi ei vaihdu
		CalculateTrajectory(*it);
	}
}

void NFmiTrajectorySystem::SetSelectedValuesToLastTrajectory(void)
{
	fTrajectoryViewTimeBagDirty = true;
	if(itsTrajectories.empty())
		return ;
	checkedVector<boost::shared_ptr<NFmiTrajectory> >::iterator it = itsTrajectories.end();
	--it;
	if(it != itsTrajectories.end())
	{
		SetSelectedValuesToTrajectory(*it, true, false); // true= myˆs tuottaja ja data tyyppi vaihtuu
		CalculateTrajectory(*it);
	}
}

void NFmiTrajectorySystem::ReCalculateTrajectories(void)
{
	fTrajectoryViewTimeBagDirty = true;
	checkedVector<boost::shared_ptr<NFmiTrajectory> >::iterator it = itsTrajectories.begin();
	for( ; it != itsTrajectories.end(); ++it)
		CalculateTrajectory(*it);
}

void NFmiTrajectorySystem::SetSelectedValuesToTrajectory(boost::shared_ptr<NFmiTrajectory> &theTrajectory, bool fInitialize, bool fKeepLevelSettings)
{
	if(fInitialize)
	{ // muutamat asetukset laitetaan vain initialisointi vaiheessa
		theTrajectory->Producer(itsSelectedProducer);
		theTrajectory->DataType(itsSelectedDataType);
	}
	theTrajectory->LatLon(itsSelectedLatLon);
	theTrajectory->Time(itsSelectedTime);
	theTrajectory->TimeStepInMinutes(itsSelectedTimeStepInMinutes);
	theTrajectory->TimeLengthInHours(itsSelectedTimeLengthInHours);
	theTrajectory->PlumesUsed(fPlumesUsed);
	theTrajectory->PlumeProbFactor(itsSelectedPlumeProbFactor);
	theTrajectory->PlumeParticleCount(itsSelectedPlumeParticleCount);
	theTrajectory->StartLocationRangeInKM(itsSelectedStartLocationRangeInKM);
	theTrajectory->StartTimeRangeInMinutes(itsSelectedStartTimeRangeInMinutes);
	if(fKeepLevelSettings == false)
	{
		theTrajectory->PressureLevel(itsSelectedPressureLevel);
		theTrajectory->StartPressureLevelRange(itsSelectedStartPressureLevelRange);
	}
	theTrajectory->Direction(itsSelectedDirection);
	theTrajectory->Isentropic(fSelectedTrajectoryIsentropic);
	theTrajectory->CalcTempBalloonTrajectories(fCalcTempBalloonTrajectors);
	theTrajectory->TempBalloonTrajectorSettings(itsTempBalloonTrajectorSettings);
}

const NFmiTimeBag& NFmiTrajectorySystem::TrajectoryViewTimeBag(void)
{
	if(fTrajectoryViewTimeBagDirty)
	{
		CalculateTrajectoryViewTimeBag();
		fTrajectoryViewTimeBagDirty = false;
	}
	return itsTrajectoryViewTimeBag;
}

void NFmiTrajectorySystem::TrajectoryViewTimeBag(const NFmiTimeBag &newTimeBag)
{
	itsTrajectoryViewTimeBag = newTimeBag;
	fTrajectoryViewTimeBagDirty = false;
}

// time1 ei ole koskaan puuttuva, time2 voi olla puuttuva
static bool LessThan(const NFmiMetTime &time1, const NFmiMetTime &time2)
{
	if(time2 == gMissingTime)
		return true;
	else
		return time1 < time2;
}

// time1 ei ole koskaan puuttuva, time2 voi olla puuttuva
static bool GreaterThan(const NFmiMetTime &time1, const NFmiMetTime &time2)
{
	if(time2 == gMissingTime)
		return true;
	else
		return time1 > time2;
}

void NFmiTrajectorySystem::CalculateTrajectoryViewTimeBag(void)
{
	NFmiMetTime firstTime(gMissingTime);
	NFmiMetTime lastTime(gMissingTime);
	NFmiMetTime fTime(gMissingTime);
	NFmiMetTime lTime(gMissingTime);
	checkedVector<boost::shared_ptr<NFmiTrajectory> >::iterator it = itsTrajectories.begin();
	for( ; it != itsTrajectories.end(); ++it)
	{
		fTime = (*it)->CalcPossibleFirstTime();
		if(::LessThan(fTime, firstTime))
			firstTime = fTime;
		lTime = (*it)->CalcPossibleLastTime();
		if(::GreaterThan(lTime, lastTime))
			lastTime = lTime;
	}
	if(firstTime != gMissingTime && lastTime != gMissingTime)
		itsTrajectoryViewTimeBag = NFmiTimeBag(firstTime, lastTime, 60);
	else
	{ // ei ollut aikoja, tehd‰‰n jonkunlainen default timebagi
		NFmiMetTime time1;
		NFmiMetTime time2;
		time2.ChangeByHours(6);
		itsTrajectoryViewTimeBag = NFmiTimeBag(time1, time2, 60);
	}
}

// theIndex on 0-pohjainen j‰rjestys luku
const NFmiTrajectory& NFmiTrajectorySystem::Trajectory(int theIndex) const
{
	static const NFmiTrajectory dummy;
	if(static_cast<int>(itsTrajectories.size()) > theIndex)
		return *(itsTrajectories[theIndex].get());
	else
		return dummy;
}


void NFmiTrajectorySystem::Write(std::ostream& os) const
{
	os << "// NFmiTrajectorySystem::Write..." << std::endl;

	os << "// version number" << std::endl;
	itsCurrentVersionNumber = itsLatestVersionNumber; // aina kirjoitetaan viimeisell‰ versio numerolla
	os << itsCurrentVersionNumber << std::endl;

	os << "// Container<Trajectories>" << std::endl;
	NFmiDataStoringHelpers::WriteContainer(itsTrajectories, os, std::string("\n"));

	os << "// Selected-LatLon" << std::endl;
	os << itsSelectedLatLon << std::endl;

	NFmiMetTime usedViewMacroTime = NFmiDataStoringHelpers::GetUsedViewMacroTime();
	os << "// selected time with utc hour + minute + day shift to current day" << std::endl;
	NFmiDataStoringHelpers::WriteTimeWithOffsets(usedViewMacroTime, itsSelectedTime, os);

	os << "// Selected-Producer" << std::endl;
	os << itsSelectedProducer << std::endl;

	os << "// SelectedDataType + SelectedTimeStepInMinutes + SelectedTimeLengthInHours" << std::endl;
	os << itsSelectedDataType << " " << itsSelectedTimeStepInMinutes << " " << itsSelectedTimeLengthInHours << std::endl;

	os << "// SelectedPlumeProbFactor + SelectedPlumeParticleCount" << std::endl;
	os << itsSelectedPlumeProbFactor << " " << itsSelectedPlumeParticleCount << std::endl;

	os << "// SelectedStartLocationRangeInKM + SelectedPressureLevel + SelectedStartPressureLevelRange" << std::endl;
	os << itsSelectedStartLocationRangeInKM << " " << itsSelectedPressureLevel << " " << itsSelectedStartPressureLevelRange << std::endl;

	os << "// SelectedDirection + SelectedStartTimeRangeInMinutes" << std::endl;
	os << itsSelectedDirection << " " << itsSelectedStartTimeRangeInMinutes << std::endl;

	os << "// PlumesUsed + TrajectoryViewOn + ShowTrajectoryArrows + ShowTrajectoryAnimationMarkers + SelectedTrajectoryIsentropic + ShowTrajectoriesInCrossSectionView + UseMapTime" << std::endl;
	os << fPlumesUsed << " " << fTrajectoryViewOn << " " << fShowTrajectoryArrows << " " << fShowTrajectoryAnimationMarkers << " " << fSelectedTrajectoryIsentropic << " " << fShowTrajectoriesInCrossSectionView << " " << fUseMapTime << std::endl;

	os << itsTempBalloonTrajectorSettings << std::endl;

	os << "// CalcTempBalloonTrajectors" << std::endl;
	os << fCalcTempBalloonTrajectors << std::endl;

	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi viel‰ mahdollinen extra data
	// Kun tulee uusia muuttujia, tee t‰h‰n extradatan t‰yttˆ‰, jotta se saadaan talteen tiedopstoon siten ett‰
	// edelliset versiot eiv‰t mene solmuun vaikka on tullut uutta dataa.
	os << "// possible extra data" << std::endl;
	os << extraData;

	if(os.fail())
		throw std::runtime_error("NFmiTrajectorySystem::Write failed");
}

void NFmiTrajectorySystem::Read(std::istream& is)
{ // toivottavasti olet poistanut kommentit luettavasta streamista!!

	is >> itsCurrentVersionNumber;
	if(itsCurrentVersionNumber > itsLatestVersionNumber)
		throw std::runtime_error("NFmiTrajectorySystem::Read failed the version number war newer than program can handle.");

	if(is.fail())
		throw std::runtime_error("NFmiTrajectorySystem::Read failed");

	itsTrajectories.clear();
	checkedVector<boost::shared_ptr<NFmiTrajectory> >::size_type ssize = 0;
	is >> ssize;
	for(checkedVector<boost::shared_ptr<NFmiTrajectory> >::size_type i=0; i< ssize; i++)
	{
		boost::shared_ptr<NFmiTrajectory> tmp(boost::shared_ptr<NFmiTrajectory>(new NFmiTrajectory()));
		is >> tmp;
		itsTrajectories.push_back(tmp);
	}


	if(is.fail())
		throw std::runtime_error("NFmiTrajectorySystem::Read failed");
	is >> itsSelectedLatLon;

	// time with utc hour + day shift to current day
	NFmiMetTime usedViewMacroTime = NFmiDataStoringHelpers::GetUsedViewMacroTime();
	NFmiDataStoringHelpers::ReadTimeWithOffsets(usedViewMacroTime, itsSelectedTime, is);

	is >> itsSelectedProducer;
	is >> itsSelectedDataType >> itsSelectedTimeStepInMinutes >> itsSelectedTimeLengthInHours;
	is >> itsSelectedPlumeProbFactor >> itsSelectedPlumeParticleCount;
	is >> itsSelectedStartLocationRangeInKM >> itsSelectedPressureLevel >> itsSelectedStartPressureLevelRange;
	int tmpValue = 0;
	is >> tmpValue >> itsSelectedStartTimeRangeInMinutes;
	itsSelectedDirection = static_cast<FmiDirection>(tmpValue);

	fTrajectoryViewTimeBagDirty = true; // trajektori timebagi pit‰‰ laskea uudestaan

	is >> fPlumesUsed >> fTrajectoryViewOn >> fShowTrajectoryArrows >> fShowTrajectoryAnimationMarkers >> fSelectedTrajectoryIsentropic >> fShowTrajectoriesInCrossSectionView >> fUseMapTime;

	if(is.fail())
		throw std::runtime_error("NFmiTrajectorySystem::Read failed");
	is >> itsTempBalloonTrajectorSettings;

	is >> fCalcTempBalloonTrajectors;

	if(is.fail())
		throw std::runtime_error("NFmiTrajectorySystem::Read failed");
	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi viel‰ mahdollinen extra data
	is >> extraData;
	// T‰ss‰ sitten otetaaan extradatasta talteen uudet muuttujat, mit‰ on mahdollisesti tullut
	// eli jos uusia muutujia tai arvoja, k‰sittele t‰ss‰.

	if(is.fail())
		throw std::runtime_error("NFmiTrajectorySystem::Read failed");

	itsCurrentVersionNumber = itsLatestVersionNumber; // aina jatketaan viimeisell‰ versio numerolla
}

void NFmiTrajectorySystem::Init(const NFmiTrajectorySystem &theOther)
{
	itsTrajectories = theOther.itsTrajectories;
	itsSelectedLatLon = theOther.itsSelectedLatLon;
	itsSelectedTime = theOther.itsSelectedTime;
	itsSelectedProducer = theOther.itsSelectedProducer;
	itsSelectedDataType = theOther.itsSelectedDataType;
	itsSelectedTimeStepInMinutes = theOther.itsSelectedTimeStepInMinutes;
	itsSelectedTimeLengthInHours = theOther.itsSelectedTimeLengthInHours;
	itsSelectedPlumeProbFactor = theOther.itsSelectedPlumeProbFactor;
	itsSelectedPlumeParticleCount = theOther.itsSelectedPlumeParticleCount;
	itsSelectedStartLocationRangeInKM = theOther.itsSelectedStartLocationRangeInKM;
	itsSelectedPressureLevel = theOther.itsSelectedPressureLevel;
	itsSelectedStartPressureLevelRange = theOther.itsSelectedStartPressureLevelRange;
	itsSelectedDirection = theOther.itsSelectedDirection;
	itsSelectedStartTimeRangeInMinutes = theOther.itsSelectedStartTimeRangeInMinutes;
	fTrajectoryViewTimeBagDirty = true;

	fPlumesUsed = theOther.fPlumesUsed;
	fTrajectoryViewOn = theOther.fTrajectoryViewOn;
	fShowTrajectoryArrows = theOther.fShowTrajectoryArrows;
	fShowTrajectoryAnimationMarkers = theOther.fShowTrajectoryAnimationMarkers;
	fSelectedTrajectoryIsentropic = theOther.fSelectedTrajectoryIsentropic;
	fShowTrajectoriesInCrossSectionView = theOther.fShowTrajectoriesInCrossSectionView;
	fUseMapTime = theOther.fUseMapTime;

	itsTempBalloonTrajectorSettings = theOther.itsTempBalloonTrajectorSettings;
	fCalcTempBalloonTrajectors = theOther.fCalcTempBalloonTrajectors;
}

// tallettaa kaikki trajektorit XML formaattiin haluttuun tiedostoon
bool NFmiTrajectorySystem::SaveXML(const std::string &theFileName)
{
	if(theFileName.empty())
		throw std::runtime_error(std::string("trajectory save failed, empty filename given."));

	if(itsTrajectories.size() > 0)
	{
		checkedVector<boost::shared_ptr<NFmiTrajectory> >& trajectories = this->itsTrajectories;
		std::string xmlStr;
																	// 1. Kirjoitetaan hederi XML-tiedostoon
		xmlStr += "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>\n";  
		xmlStr += "<fmitrajectory>\n";
		xmlStr += "<header>\n";
		xmlStr += "<time>\n";

		NFmiMetTime metTime(1);
		NFmiTime localTime = metTime.CorrectLocalTime();
		
		xmlStr +=  (char*)localTime.ToStr("YYYYMMDDHHmm"); //Miten t‰h‰n saa minuutit ja ehk‰ paikalliseksi??
		xmlStr +="\n</time>\n";

		xmlStr += "<legendtext>\n";						// Ja t‰m‰ pit‰isi napata kuvaruudulta
		xmlStr += itsLastTrajectoryLegendStr;
		xmlStr +="\n</legendtext>\n";
		xmlStr += "</header>\n";  

		xmlStr += "<data>\n";		//Tulostetaan trajektoridatapisteet trajektori kerrallaan
		xmlStr += "<trajectories>\n";

		checkedVector<boost::shared_ptr<NFmiTrajectory> >::iterator it = trajectories.begin(); 
		
		for( ; it != trajectories.end() ; ++it)
		{
			xmlStr += (*it).get()->ToXMLStr();
		}

																// 4. Kirjoitetaan loppurimpsut XML-tiedostoon
		xmlStr += "</trajectories>\n";
		xmlStr += "</data>\n";
		xmlStr += "</fmitrajectory>\n";

		std::ofstream out(theFileName.c_str());
		if(out)
		{
			out << xmlStr << std::endl;
		}
		else
			throw std::runtime_error(std::string("trajectory save failed, cannot create file:\n") + theFileName);

		return true;
	}
	return false;
}

std::string NFmiTrajectory::ToXMLStr(void)
{
	std::string xmlStr;
															// 2. Kirjoitetaan Pƒƒtrajektori
	NFmiSingleTrajector mainTrajector(MainTrajector());  

	int TimeStepInMinutes(itsTimeStepInMinutes);
	NFmiProducer Producer(itsProducer);
	FmiDirection Direction(itsDirection);

	xmlStr += mainTrajector.ToXMLStr(TimeStepInMinutes, Producer, Direction);

															// 3. Kirjoitetaan trajektoriLAUMA
	 NFmiString newName = "H_"+Producer.GetName();
	 NFmiProducer newProducer(Producer.GetIdent(),newName);

	if(PlumesUsed())
	{
		const std::vector<boost::shared_ptr<NFmiSingleTrajector> >& plumes = PlumeTrajectories(); 
		std::vector<boost::shared_ptr<NFmiSingleTrajector> >::const_iterator it = plumes.begin();
		for( ; it != plumes.end(); ++it)
           xmlStr += (*it).get()->ToXMLStr(TimeStepInMinutes, newProducer, Direction);
	}

	return xmlStr;
}
static std::string Point2XML(const NFmiPoint &thePoint)
{
	std::string str;

	str += (char*)NFmiValueString::GetStringWithMaxDecimalsSmartWay(thePoint.Y(), 2);

	str += " ";
	str += (char*)NFmiValueString::GetStringWithMaxDecimalsSmartWay(thePoint.X(), 2);
	str += " ";

	return str;
}

static std::string MakeValueStr(float theValue, int theDecimalCount, const std::string &wantedMissingStr)
{
	if(theValue == kFloatMissing)
		return wantedMissingStr;
	else
		return (char*)NFmiValueString::GetStringWithMaxDecimalsSmartWay(theValue, theDecimalCount);
}

std::string NFmiSingleTrajector::ToXMLStr(int TimeStepInMinutes, NFmiProducer &Producer, FmiDirection Direction)
{
	std::string str;

	str += "<producer>\n";		// Tarkista, koska t‰m‰ puutuu VAX-versiosta, mutta voisi olla looginen 
	str +=  (char*)Producer.GetName();		
	str += "\n</producer>\n";  

	if(Direction == kForward)		
		str += "<trajectory direction=\"+1\">\n";
	else if (Direction == kBackward)
		{
		str += "<trajectory direction=\"-1\">\n";
			TimeStepInMinutes = -TimeStepInMinutes;
		}

	str += "<points>\n";

	NFmiMetTime tmpTime(itsStartTime);
	tmpTime.SetTimeStep(1);

	size_t ssize = itsPoints.size();
	for(size_t i=0; i<ssize; i++)
	{
		str += "<point> <time> ";
		str +=  (char*)tmpTime.ToStr("YYYYMMDDHHmm"); 
		str += " </time> <location type=\"4\">  ";
		const NFmiPoint &latlon = itsPoints[i];

		str += ::Point2XML(latlon);  
//pit‰isikˆh‰n olla metodina paineen kirjailu. Ainakin sitten kun saadaan metrinen korkeus mukaan
		float P = itsPressures[i];
		str += ::MakeValueStr(P, 2, "-999.0");

		float zValue = itsHeightValues[i];
		str += " ";
		str += ::MakeValueStr(zValue, 2, "-999.0");			//HUOM! t‰h‰n tulee korkeus metrein‰
		str += " ";

		str += "</location> </point>\n";
		tmpTime.ChangeByMinutes(TimeStepInMinutes);
	}
	str += "</points>\n";
	str += "</trajectory>\n";

	return str;
}

// S‰‰det‰‰n kaikki aikaa liittyv‰t jutut parametrina annettuun aikaan, ett‰ SmartMet s‰‰tyy ladattuun CaseStudy-dataan mahdollisimman hyvin.
void NFmiTrajectorySystem::SetCaseStudyTimes(const NFmiMetTime &theCaseStudyTime)
{
	if(itsTrajectories.size() > 0)
	{
		long diffInMinutes = theCaseStudyTime.DifferenceInMinutes(itsTrajectories[0]->Time());
		for(size_t i = 0; i < itsTrajectories.size(); i++)
		{
			NFmiMetTime newTime = itsTrajectories[i]->Time();
			newTime.ChangeByMinutes(diffInMinutes);
			itsTrajectories[i]->Time(newTime);
			itsTrajectories[i]->Calculated(false);
		}
	}
	itsSelectedTime = theCaseStudyTime;
	fTrajectoryViewTimeBagDirty = true;
}
