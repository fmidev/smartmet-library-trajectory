#include "NFmiTrajectory.h"
#include "NFmiTrajectorySystem.h"

#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiQueryData.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>

#include <stdexcept>
#include <string>

// ----------------------------------------------------------------------
/*!
 * \brief Supported output formats
 */
// ----------------------------------------------------------------------

enum OutputFormat
  {
	ASCII,
	CSV,
	GML,
	GPX,
	KML,
	KMZ,
	XML
  };

// ----------------------------------------------------------------------
/*!
 * \brief Parsed command line options
 */
// ----------------------------------------------------------------------

struct Options
{
  Options();

  bool verbose;
  unsigned int timestep;
  unsigned int hours;
  std::string queryfile;
  OutputFormat format;
  unsigned int plumesize;
  double disturbance;
  double arearadius;
  double timeinterval;
  double pressure;
  double pressurerange;
  bool isentropic;
  bool forwards;
};

Options options;

// ----------------------------------------------------------------------
/*!
 * \brief Default command line options
 */
// ----------------------------------------------------------------------

Options::Options()
  : verbose(false)
  , timestep(10)
  , hours(24)
  , queryfile("-")
  , format(XML)
  , plumesize(0)
  , disturbance(25)
  , arearadius(0)
  , timeinterval(0)
  , pressure(850)
  , pressurerange(0)
  , isentropic(false)
  , forwards(true)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse an output format description
 */
// ----------------------------------------------------------------------

OutputFormat parse_format(const std::string & desc)
{
  static 

  std::string name = boost::algorithm::to_lower_copy(desc);

  if     (name == "kml")	return KML;
  else if(name == "kmz")	return KMZ;
  else if(name == "gml")	return GML;
  else if(name == "gpx")	return GPX;
  else if(name == "ascii")	return ASCII;
  else if(name == "csv")	return CSV;
  else if(name == "xml")	return XML;
  else 
	throw std::runtime_error("Unknown output format '"+desc+"'");
}


// ----------------------------------------------------------------------
/*!
 * \brief Parse command line options
 * \return True, if execution may continue as usual
 */
// ----------------------------------------------------------------------

bool parse_options(int argc, char * argv[])
{
  namespace po = boost::program_options;
  namespace fs = boost::filesystem;

  std::string opt_format;

  po::options_description desc("Available options");
  desc.add_options()
	("help,h" , "print out usage information")
	("version,V" , "display version number")
	("verbose,v" , po::bool_switch(&options.verbose), "verbose mode")
	("querydata,q", po::value(&options.queryfile), "input querydata (standard input)")
	("format,f", po::value(&opt_format), "output format (kml)")
	("timestep,T", po::value(&options.timestep), "time step in minutes (10)")
	("hours,H", po::value(&options.hours), "simulation length in hours (24)")
	("plumesize,N", po::value(&options.plumesize), "plume size (0)")
	("disturbance,D", po::value(&options.disturbance), "plume disturbance factor (25)")
	("radius,R", po::value(&options.arearadius), "plume dispersal radius in km (0)")
	("interval,I", po::value(&options.timeinterval), "plume dispersal time interval (+-) in minutes (0)")
	("pressure,P", po::value(&options.pressure), "initial dispersal pressure")
	("pressurerange", po::value(&options.pressurerange), "plume dispersal pressure range")
	("isentropic,i", po::bool_switch(&options.isentropic), "isentropic simulation")
	("backwards,b", po::bool_switch(&options.forwards), "backwards simulation in time")
	;

  po::positional_options_description p;
  p.add("querydata",1);

  po::variables_map opt;
  po::store(po::command_line_parser(argc, argv)
			.options(desc)
			.positional(p)
			.run(),
			opt);

  po::notify(opt);

  if(opt.count("version") != 0)
	{
	  std::cout << "qdtrajectory v1.0 ("
				<< __DATE__
				<< ' '
				<< __TIME__
				<< ')'
				<< std::endl;
	}

  if(opt.count("help"))
	{
	  std::cout << "Usage: qdtrajectory [options] querydata" << std::endl
				<< "       qdtrajectory -i querydata [options]" << std::endl
				<< "       qdtrajectory [options] < querydata" << std::endl
				<< "       cat querydata | qdtrajectory [options]" << std::endl
				<< std::endl
				<< "Calculate trajectories based on model level querydata." << std::endl
				<< std::endl
				<< "Supported output formats:" << std::endl
				<< std::endl
				<< "\tascii\tTabular coordinate data" << std::endl
				<< "\tcsv\tComma separated coordinate data" << std::endl
				<< "\tgml\tGeography markaup language" << std::endl
				<< "\tgpx\tGPS eXchange format" << std::endl
				<< "\tkml\tKeyhole markup language" << std::endl
				<< "\tkmz\tZipped KML" << std::endl
				<< "\txml\tSimulation system native XML" << std::endl
				<< std::endl
				<< desc << std::endl;
	  return false;
	}

  if(options.queryfile != "-" && !fs::exists(options.queryfile))
	throw std::runtime_error("Input file '"+options.queryfile+"' does not exist");

  if(!opt_format.empty())
	options.format = parse_format(opt_format);

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Calculate a trajectory
 */
// ----------------------------------------------------------------------

boost::shared_ptr<NFmiTrajectory>
calculate_trajectory(boost::shared_ptr<NFmiFastQueryInfo> & theInfo)
{
  auto trajectory = boost::make_shared<NFmiTrajectory>();

// Copy producer information
  trajectory->Producer(*theInfo->Producer());

  // trajectory->DataType(...)					// Unused

  // Point of interest
  trajectory->LatLon(NFmiPoint(25,60));

  // Start time, time step and simulation length
  trajectory->Time(NFmiMetTime());
  trajectory->TimeStepInMinutes(options.timestep);
  trajectory->TimeLengthInHours(options.hours);

  // Plume mode, disturbance factor as a percentage, plume size
  trajectory->PlumesUsed(options.plumesize > 0);
  trajectory->PlumeParticleCount(options.plumesize);
  trajectory->PlumeProbFactor(options.disturbance);

  // Plume initial dispersion radius, start time interval, and pressure disribution 
  trajectory->StartLocationRangeInKM(options.arearadius);
  trajectory->StartTimeRangeInMinutes(options.timeinterval);
  trajectory->PressureLevel(options.pressure);
  trajectory->StartPressureLevelRange(options.pressurerange);

  // Forward or backward in time
  trajectory->Direction(options.forwards ? kForward : kBackward);

  // Follow potential temperature
  trajectory->Isentropic(options.isentropic);

  // Balloon trajectories
  // trajectory->CalcTempBalloonTrajectors(false);
  // trajectory->TempBalloonTrajectorSettings(NFmiTempBalloonTrajectorSettings());

  NFmiTrajectorySystem::CalculateTrajectory(trajectory, theInfo);

  return trajectory;
}

// ----------------------------------------------------------------------
/*!
 * \brief Pretty print input file name
 */
// ----------------------------------------------------------------------

std::string pretty_input_filename(const std::string & theName)
{
  if(theName == "-")
	return "standard input";
  else
	return "'" + theName + "'";
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program without exception handling
 */
// ----------------------------------------------------------------------

int run(int argc, char * argv[])
{
  if(!parse_options(argc,argv))
	return 0;

  // Read input data

  if(options.verbose)
	std::cerr << "Reading querydata from " << pretty_input_filename(options.queryfile) << std::endl;
  NFmiQueryData qd(options.queryfile);
  auto qi = boost::make_shared<NFmiFastQueryInfo>(&qd);

  // Verify the data is suitable

  if(qi->Level()->LevelType() != kFmiHybridLevel)
	throw std::runtime_error("The input querydata must contain hybrid levels");

  // Calculate the trajectory

  auto trajectory = calculate_trajectory(qi);

  std::cout << trajectory->ToXMLStr() << std::endl;

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program
 */
// ----------------------------------------------------------------------

int main(int argc, char * argv[])
try
  {
        return run(argc,argv);
  }
catch(std::exception & e)
  {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
  }
catch(...)
  {
        std::cerr << "Error: Caught an unknown exception" << std::endl;
        return 1;
  }
