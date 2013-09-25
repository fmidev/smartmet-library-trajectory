#include "NFmiTrajectory.h"
#include <smarttools/NFmiDataStoringHelpers.h>


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
,itsStartPressureLevelRange(0)
,itsIsentropicTpotValue(kFloatMissing)
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
,itsStartPressureLevelRange(0)
,itsIsentropicTpotValue(kFloatMissing)
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
// laskuissa käytetään main-trajektoria
void NFmiTrajectory::CalculateCrossSectionTrajectoryHelpData(void)
{
	itsCrossSectionTrajectoryPoints.clear();
	itsCrossSectionTrajectoryTimes.clear();

	double usedTimeStepInMinutes = 60; // tämä vaatimus tulee poikkileikkaus näytöstä
	int wantedPoints = static_cast<int>((itsTimeLengthInHours*60/usedTimeStepInMinutes) + 1);
	int ssize = static_cast<int>(itsMainTrajector.Points().size());

	// sitten täytetään apudata vektori
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
	else // backward tilanteessa pitää täyttää trajektori pisteitä käänteisessä järjestyksessä
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
	NFmiDataStoringHelpers::WriteTimeWithOffsets(usedViewMacroTime, itsOriginTime, os); // Tätä ei ehkä voi oikeasti käyttää!!!

	os << "// Producer" << std::endl;
	os << itsProducer << std::endl;

	os << "// DataType + TimeStepInMinutes + TimeLengthInHours + PlumeProbFactor + PlumeParticleCount" << std::endl;
	os << itsDataType << " " << itsTimeStepInMinutes << " " << itsTimeLengthInHours << " " << itsPlumeProbFactor << " " << itsPlumeParticleCount << std::endl;

	os << "// StartLocationRangeInKM + PressureLevel + StartPressureLevelRange + IsentropicTpotValue + StartTimeRangeInMinutes" << std::endl;
	os << itsStartLocationRangeInKM << " " << itsPressureLevel << " " << itsStartPressureLevelRange << " " << itsIsentropicTpotValue << " " << itsStartTimeRangeInMinutes << std::endl;

	os << "// Direction + PlumesUsed + Isentropic + fCalcTempBalloonTrajectories" << std::endl;
	os << static_cast<int>(itsDirection) << " " << fPlumesUsed << " " << fIsentropic << " " << fCalcTempBalloonTrajectories << std::endl;

	os << "// TempBalloonTrajectorSettings" << std::endl;
	os << itsTempBalloonTrajectorSettings << std::endl;

	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi vielä mahdollinen extra data
	// Kun tulee uusia muuttujia, tee tähän extradatan täyttöä, jotta se saadaan talteen tiedopstoon siten että
	// edelliset versiot eivät mene solmuun vaikka on tullut uutta dataa.
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

	NFmiDataStoringHelpers::ReadTimeWithOffsets(usedViewMacroTime, itsOriginTime, is); // Tätä ei ehkä voi oikeasti käyttää!!!

	is >> itsProducer;

	is >> itsDataType >> itsTimeStepInMinutes >> itsTimeLengthInHours >> itsPlumeProbFactor >> itsPlumeParticleCount;

	is >> itsStartLocationRangeInKM >> itsPressureLevel >> itsStartPressureLevelRange >> itsIsentropicTpotValue >> itsStartTimeRangeInMinutes;

	int tmpValue = 0;
	is >> tmpValue >> fPlumesUsed >> fIsentropic >> fCalcTempBalloonTrajectories;
	itsDirection = static_cast<FmiDirection>(tmpValue);

	is >> itsTempBalloonTrajectorSettings;

	if(is.fail())
		throw std::runtime_error("NFmiTrajectory::Read failed");
	NFmiDataStoringHelpers::NFmiExtraDataStorage extraData; // lopuksi vielä mahdollinen extra data
	is >> extraData;
	// Tässä sitten otetaaan extradatasta talteen uudet muuttujat, mitä on mahdollisesti tullut
	// eli jos uusia muutujia tai arvoja, käsittele tässä.

	if(is.fail())
		throw std::runtime_error("NFmiTrajectory::Read failed");
}

