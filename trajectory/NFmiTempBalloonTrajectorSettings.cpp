// *****************************************************************************
// ***** NFmiTempBalloonTrajectorSettings **************************************
// *****************************************************************************

#include "NFmiTempBalloonTrajectorSettings.h"
#include <smarttools/NFmiDataStoringHelpers.h>

// laske currentilla arvoilla nousu/laskunopeus yksikössä hPa/s. Osaa tehdä päätelmiä eri luotaus
// pallon lennon
// vaiheista ja osaa mm. siirtyä seuraavaan vaiheeseen.
double NFmiTempBalloonTrajectorSettings::CalcOmega(double Z, int theTimeStepInMinutes)
{
  if (theTimeStepInMinutes <= 0 || Z == kFloatMissing) itsState = kNoDirection;

  switch (itsState)
  {
    case kBase:
      itsState = kUp;  // HUOM! tästä on siis tarkoitus jatkaa suoraan kUp caseen!
    case kUp:
      return CalcOmegaInPhase1(Z, theTimeStepInMinutes);
    case kTop:
      return CalcOmegaInPhase2(theTimeStepInMinutes);
    case kDown:
      return CalcOmegaInPhase3(Z, theTimeStepInMinutes);
    case kNoDirection:  // eli pallo on törmännyt jo maahan tai data on ollut puutteellista
      return kFloatMissing;
    default:
      itsState = kNoDirection;
      return kFloatMissing;
  }
}

double NFmiTempBalloonTrajectorSettings::CalcDeltaP(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                                    const NFmiPoint &theLatlon,
                                                    const NFmiMetTime &theTime,
                                                    double theCurrentPressure,
                                                    unsigned long thePressureParamIndex,
                                                    double Z,
                                                    int theTimeStepInMinutes,
                                                    unsigned long theGroundLevelIndex)
{
  if (theTimeStepInMinutes <= 0 || Z == kFloatMissing) itsState = kNoDirection;

  switch (itsState)
  {
    case kBase:
      itsState = kUp;  // HUOM! tästä on siis tarkoitus jatkaa suoraan kUp caseen!
    case kUp:
      return CalcDeltaPInPhase1(theInfo,
                                theLatlon,
                                theTime,
                                theCurrentPressure,
                                thePressureParamIndex,
                                Z,
                                theTimeStepInMinutes);
    case kTop:
      return CalcDeltaPInPhase2(theTimeStepInMinutes);
    case kDown:
      return CalcDeltaPInPhase3(theInfo,
                                theLatlon,
                                theTime,
                                theCurrentPressure,
                                thePressureParamIndex,
                                Z,
                                theTimeStepInMinutes,
                                theGroundLevelIndex);
    case kNoDirection:  // eli pallo on törmännyt jo maahan tai data on ollut puutteellista
      return kFloatMissing;
    default:
      itsState = kNoDirection;
      return kFloatMissing;
  }
}

double NFmiTempBalloonTrajectorSettings::CalcDeltaPInPhase1(
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiPoint &theLatlon,
    const NFmiMetTime &theTime,
    double theCurrentPressure,
    unsigned long thePressureParamIndex,
    double Z,
    int theTimeStepInMinutes)
{
  // 1.a.1 Laske nousunopeus (w)
  double w = CalcRisingRate(Z);
  // 1.a.1b korkeuden muutos (dz = w*timestep)
  double dz = w * theTimeStepInMinutes * 60.;
  // 1.a.2 laske uusi korkeus (z = z+dz)
  double zNew = Z + dz;
  // 1.a.5 jos uusi korkeus yli top-korkeuden, "palauta" pallo top korkeuteen ja aloita kellutus
  if (zNew >= itsTopHeightInKM * 1000.)
  {
    zNew = itsTopHeightInKM * 1000.;
    itsState = kTop;
    if (itsFloatingTimeInMinutes <= 0) itsState = kDown;
  }
  // 1.b.1 jatka...  laske deltaP ja palauta se [hPa/s]
  theInfo->ParamIndex(thePressureParamIndex);
  double newP = theInfo->HeightValue(static_cast<float>(zNew), theLatlon, theTime);
  if (newP != kFloatMissing) return newP - theCurrentPressure;
  return kFloatMissing;
}

double NFmiTempBalloonTrajectorSettings::CalcDeltaPInPhase2(int theTimeStepInMinutes)
{
  // 2.1 lisää aikasteppi kellutus aikaan
  itsCurrentFloatTimeInMinutes += theTimeStepInMinutes;
  // 2.2 ollaanko jo kelluttu tarpeeksi
  // 2.3 jos kellutus lopussa, siirry seuraavaan vaiheeseen
  if (itsCurrentFloatTimeInMinutes >= itsFloatingTimeInMinutes) itsState = kDown;
  // 2.4 palauta 0 omega
  return 0;
}

// phase3 eli pallon lasku/putoamis vaihe
double NFmiTempBalloonTrajectorSettings::CalcDeltaPInPhase3(
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiPoint &theLatlon,
    const NFmiMetTime &theTime,
    double theCurrentPressure,
    unsigned long thePressureParamIndex,
    double Z,
    int theTimeStepInMinutes,
    unsigned long theGroundLevelIndex)
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
  if (zNew <= surfaceZ)
  {
    // 3.4.1 jos ollaan, laske pinta korkeuteen omega ja laita state:ksi loppu ja palauta omega
    zNew = surfaceZ;
    itsState = kNoDirection;
  }
  // 3.4.2 laske deltaP ja palauta se
  theInfo->ParamIndex(thePressureParamIndex);
  // HUOM! jos ollaan jo pinnassa, pyydä pinta painetta, muuten painetta saadusta korkeudesta
  double newP = (itsState == kNoDirection)
                    ? theInfo->InterpolatedValue(theLatlon, theTime)
                    : theInfo->HeightValue(static_cast<float>(zNew), theLatlon, theTime);
  if (newP != kFloatMissing) return newP - theCurrentPressure;
  return kFloatMissing;
}

// Sodankylän observatoriosta saatu pallon 'vakio' nousunopeus kaava. Riippuvainen korkeudesta
double NFmiTempBalloonTrajectorSettings::CalcRisingRate(double Z)
{
  double w = ((1.19 * itsRisingSpeed) - 4.42) * 1. / 10000. * Z + 3.42;
  return w;
}

// Sodankylän observatoriosta saatu pallon 'vakio' lasku/putoamisnopeus kaava. Riippuvainen
// korkeudesta
double NFmiTempBalloonTrajectorSettings::CalcFallingRate(double Z)
{
  double w = (-3.44 * 1. / 100000000. * Z * Z) + (1.24 * 1. / 10000. * Z) - 5.17;
  return w;
}

// HUOM! Tämän pitäisi olla Z-riippuvainen (g ja roo muuttuvat korkeuden mukaan).
// Laskee paineen muutosnopeuden (omega [hPa/s]) vertikaalinopeuden (w) ja korkeuden (z)
// avulla.
static double CalcOmegaValue(double w, double /* Z */)
{
  const double g = 9.81;  // putoamis kiihtyvyys (pitäisi vaihtua korkeuden mukana)
  const double rooAir =
      1;  // ilman tiheys rooAir (olisi pinnalla n. 1.2 ja pitäisi muuttua korkeuden mukaan)
  double omega =
      -g * rooAir *
      w;  // käytetty kaavaa omega = -g*roo*w eli -kiihtyvyys*ilman tiheys*vertikaali nopeus
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
  if (zNew >= itsTopHeightInKM * 1000.)
  {
    zNew = itsTopHeightInKM * 1000.;
    itsState = kTop;
    if (itsFloatingTimeInMinutes <= 0) itsState = kDown;
  }
  // 1.b.1 jatka...  laske omega ja palauta se [hPa/s]
  double realDZ = zNew - Z;
  double realW = realDZ / (theTimeStepInMinutes * 60.);
  return CalcOmegaValue(realW, Z);
}

// phase2 eli pallon kellutus vaihe (kelluu ylä rajalla)
double NFmiTempBalloonTrajectorSettings::CalcOmegaInPhase2(int theTimeStepInMinutes)
{
  // 2.1 lisää aikasteppi kellutus aikaan
  itsCurrentFloatTimeInMinutes += theTimeStepInMinutes;
  // 2.2 ollaanko jo kelluttu tarpeeksi
  // 2.3 jos kellutus lopussa, siirry seuraavaan vaiheeseen
  if (itsCurrentFloatTimeInMinutes >= itsFloatingTimeInMinutes) itsState = kDown;
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
  if (zNew <= surfaceZ)
  {
    // 3.4.1 jos ollaan, laske pinta korkeuteen omega ja laita state:ksi loppu ja palauta omega
    zNew = surfaceZ;
    itsState = kNoDirection;
  }
  // 3.4.2 laske omega ja palauta se
  double realDZ = zNew - Z;
  double realW = realDZ / (theTimeStepInMinutes * 60.);
  return CalcOmegaValue(realW, Z);
}

void NFmiTempBalloonTrajectorSettings::Write(std::ostream &os) const
{
  os << "// NFmiTempBalloonTrajectorSettings::Write..." << std::endl;

  os << "// RisingSpeed + FallSpeed + TopHeightInKM + FloatingTimeInMinutes" << std::endl;
  os << itsRisingSpeed << " " << itsFallSpeed << " " << itsTopHeightInKM << " "
     << itsFloatingTimeInMinutes << std::endl;

  NFmiDataStoringHelpers::NFmiExtraDataStorage extraData;  // lopuksi vielä mahdollinen extra data
  // Kun tulee uusia muuttujia, tee tähän extradatan täyttöä, jotta se saadaan talteen tiedopstoon
  // siten että
  // edelliset versiot eivät mene solmuun vaikka on tullut uutta dataa.
  os << "// possible extra data" << std::endl;
  os << extraData;

  if (os.fail()) throw std::runtime_error("NFmiTempBalloonTrajectorSettings::Write failed");
}

void NFmiTempBalloonTrajectorSettings::Read(std::istream &is)
{  // toivottavasti olet poistanut kommentit luettavasta streamista!!

  is >> itsRisingSpeed >> itsFallSpeed >> itsTopHeightInKM >> itsFloatingTimeInMinutes;
  itsState = kBase;
  itsCurrentFloatTimeInMinutes = 0;

  if (is.fail()) throw std::runtime_error("NFmiTempBalloonTrajectorSettings::Read failed");
  NFmiDataStoringHelpers::NFmiExtraDataStorage extraData;  // lopuksi vielä mahdollinen extra data
  is >> extraData;
  // Tässä sitten otetaaan extradatasta talteen uudet muuttujat, mitä on mahdollisesti tullut
  // eli jos uusia muutujia tai arvoja, käsittele tässä.

  if (is.fail()) throw std::runtime_error("NFmiTempBalloonTrajectorSettings::Read failed");
}
