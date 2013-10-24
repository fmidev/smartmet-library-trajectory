#include "NFmiSingleTrajector.h"
#include "NFmiTrajectory.h"
#include "NFmiTrajectorySystem.h"

#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiQueryData.h>

#include <fminames/FmiLocationQueryOptions.h>
#include <fminames/FmiLocationQuery.h>

#include <macgyver/AnsiEscapeCodes.h>
#include <macgyver/Cast.h>
#include <macgyver/TimeFormatter.h>

#include <brainstorm/spine/TemplateFormatter.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// ----------------------------------------------------------------------
/*!
 * \brief Parsed command line options
 */
// ----------------------------------------------------------------------

struct Options
{
  Options();
  void report(std::ostream & out) const;

  bool verbose;
  NFmiPoint coordinate;
  unsigned int timestep;
  unsigned int hours;
  std::string queryfile;
  std::string outfile;
  std::string templatedir;
  std::string templatefile;
  std::string format;
  unsigned int plumesize;
  double disturbance;
  double arearadius;
  double timeinterval;
  double pressure;
  double pressurerange;
  bool isentropic;
  bool backwards;

  bool debughash;

};

Options options;

// ----------------------------------------------------------------------
/*!
 * \brief Default command line options
 */
// ----------------------------------------------------------------------

Options::Options()
  : verbose(false)
  , coordinate(NFmiPoint(kFloatMissing,kFloatMissing))
  , timestep(10)
  , hours(24)
  , queryfile("-")
  , outfile("-")
  , templatedir("tmpl")
  , templatefile()
  , format("xml")
  , plumesize(0)
  , disturbance(25)
  , arearadius(0)
  , timeinterval(0)
  , pressure(850)
  , pressurerange(0)
  , isentropic(false)
  , backwards(false)
  , debughash(false)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Print the parsed options
 */
// ----------------------------------------------------------------------

void Options::report(std::ostream & out) const
{
#define HEADER(stream,title) stream << ANSI_BOLD_ON << ANSI_UNDERLINE_ON << title << ANSI_BOLD_OFF << ANSI_UNDERLINE_OFF << '\n';
#define REPORT(stream,name,value) stream << ANSI_ITALIC_ON << std::setw(35) << std::left << name << ANSI_ITALIC_OFF << value << '\n';

  HEADER(out,"Options summary");
  REPORT(out,"Verbose:",verbose);
  REPORT(out,"Longitude:",coordinate.X());
  REPORT(out,"Latitude:",coordinate.Y());
  REPORT(out,"Time step in minutes:",timestep);
  REPORT(out,"Simulation length in hours:",hours);
  REPORT(out,"Input querydata:",queryfile);
  REPORT(out,"Output file:",outfile);
  REPORT(out,"Output format:",format);
  REPORT(out,"Template directory:",templatedir);
  REPORT(out,"Template file:",templatefile);
  REPORT(out,"Plume size:",plumesize);
  REPORT(out,"Plume disturbance factor:",disturbance);
  REPORT(out,"Plume dispersion radius in km:",arearadius);
  REPORT(out,"Plume time dispersion in minutes:", timeinterval);
  REPORT(out,"Plume pressure level:", pressure);
  REPORT(out,"Plume pressure dispersion in hPa:", pressurerange);
  REPORT(out,"Isentropic mode:",isentropic);
  REPORT(out,"Backward simulation in time:",backwards);
  out << '\n';
}

// ----------------------------------------------------------------------
/*!
 * \brief Print available formats based on the template directory
 */
// ----------------------------------------------------------------------

void list_formats()
{
  namespace fs = boost::filesystem;

  fs::path p = options.templatedir;

  if(!fs::exists(p))
	throw std::runtime_error("Template directory '"+options.templatedir+"' does not exist");

  if(!fs::is_directory(p))
	throw std::runtime_error("Not a directory: '"+options.templatedir+"'");

  std::cerr << "Formats found from template directory '" << options.templatedir << "':\n\n";

  size_t count = 0;
  for(fs::directory_iterator i(p), end; i!=end; ++i)
	{
	  if(i->path().extension() == ".c2t")
		std::cerr << std::setw(3) << std::right << ++count << ". " << i->path().stem().string() << '\n';
	}

  std::cerr << std::endl << "Found " << count << (count!=1 ? " formats.\n" : " format.\n");
}

// ----------------------------------------------------------------------
/*!
 * \brief Easier representation for direction
 */
// ----------------------------------------------------------------------

int pretty_direction(FmiDirection dir)
{
  switch(dir)
	{
	case kForward: return 1;
	case kBackward: return -1;
	default: throw std::runtime_error("Invalid trajectory direction encountered");
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a lon,lat coordinate
 */
// ----------------------------------------------------------------------

NFmiPoint parse_lonlat(const std::string & theStr)
{
  std::vector<std::string> parts;
  boost::algorithm::split(parts, theStr, boost::is_any_of(","));

  if(parts.size() != 2)
	throw std::runtime_error("Invalid coordinate specification '"+theStr+"'");

  return NFmiPoint(Fmi::number_cast<double>(parts[0]),
				   Fmi::number_cast<double>(parts[1]));
  
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a lat,lon coordinate
 */
// ----------------------------------------------------------------------

NFmiPoint parse_latlon(const std::string & theStr)
{
  std::vector<std::string> parts;
  boost::algorithm::split(parts, theStr, boost::is_any_of(","));

  if(parts.size() != 2)
	throw std::runtime_error("Invalid coordinate specification '"+theStr+"'");

  return NFmiPoint(Fmi::number_cast<double>(parts[1]),
				   Fmi::number_cast<double>(parts[0]));
  
}

// ----------------------------------------------------------------------
/*!
 * \brief Print location information
 */
// ----------------------------------------------------------------------

void print_location(std::ostream & out, const FmiNames::FmiSimpleLocation & theLoc)
{
  out << "Location name: " << theLoc.name << '\n'
	  << "           id: " << theLoc.id << '\n'
	  << "          lon: " << theLoc.lon << '\n'
	  << "          lat: " << theLoc.lat << '\n'
	  << "      country: " << theLoc.country << '\n'
	  << "        admin: " << theLoc.admin << '\n'
	  << "      feature: " << theLoc.feature << '\n'
	  << "     timezone: " << theLoc.timezone << '\n'
	  << "    elevation: " << theLoc.elevation << '\n';
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

  double opt_longitude = kFloatMissing;
  double opt_latitude = kFloatMissing;
  std::string opt_lonlat;
  std::string opt_latlon;
  std::string opt_place;

  po::options_description desc("Available options");
  desc.add_options()
	("help,h" , "print out usage information")
	("version,V" , "display version number")
	("verbose,v" , po::bool_switch(&options.verbose), "verbose mode")
	("querydata,q", po::value(&options.queryfile), "input querydata (standard input)")
	("outfile,o", po::value(&options.outfile), "output file name")
	("place,p", po::value(&opt_place), "location name")
	("lonlat", po::value(&opt_lonlat), "longitude,latitude")
	("latlon", po::value(&opt_latlon), "latitude,longitude")
	("longitude,x", po::value(&opt_longitude), "dispersal longitude")
	("latitude,y", po::value(&opt_latitude), "dispersal latitude")
	("format,f", po::value(&opt_format), "output format (kml)")
	("list-formats", "list known output formats and exit")
	("templatedir", po::value(&options.templatedir), "template directory for output formats")
	("template", po::value(&options.templatefile), "specific output template file to use")
	("timestep,T", po::value(&options.timestep), "time step in minutes (10)")
	("hours,H", po::value(&options.hours), "simulation length in hours (24)")
	("plume-size,N", po::value(&options.plumesize), "plume size (0)")
	("disturbance,D", po::value(&options.disturbance), "plume disturbance factor (25)")
	("radius,R", po::value(&options.arearadius), "plume dispersal radius in km (0)")
	("interval,I", po::value(&options.timeinterval), "plume dispersal time interval (+-) in minutes (0)")
	("pressure,P", po::value(&options.pressure), "initial dispersal pressure")
	("pressure-range", po::value(&options.pressurerange), "plume dispersal pressure range")
	("isentropic,i", po::bool_switch(&options.isentropic), "isentropic simulation")
	("backwards,b", po::bool_switch(&options.backwards), "backwards simulation in time")
	("debug-hash", po::bool_switch(&options.debughash), "print the internal hash tables")
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
				<< ")\n";
	  return false;
	}

  if(opt.count("help"))
	{
	  std::cout <<
		"Usage: qdtrajectory [options] querydata\n"
		"       qdtrajectory -i querydata [options]\n"
		"       qdtrajectory [options] < querydata\n"
		"       cat querydata | qdtrajectory [options]\n"
		"\n"
		"Calculate trajectories based on model level querydata.\n"
		"\n"
		"The supported output formats can be listed using the --list-formats\n"
		"option, which essentially just lists the output templates found from\n"
		"the template directory. Some common output formats include:\n"
		"\n"
		"     gml   Geography markaup language\n"
		"     gpx   GPS eXchange format\n"
		"     kml   Keyhole markup language\n"
		"     kmz   Zipped KML\n"
		"\n"
		"In addition the 'debug' format is supported for dumping the contents of\n"
		"the simulation in a legacy XML format. Please use --list-formats to see\n"
		"which formats are supported.\n"
		"\n";

	  std::cout << desc << '\n';
	  return false;
	}

  if(opt.count("list-formats"))
	{
	  list_formats();
	  return false;
	}

  if(options.queryfile != "-" && !fs::exists(options.queryfile))
	throw std::runtime_error("Input file '"+options.queryfile+"' does not exist");

  if(!opt_format.empty())
	options.format = boost::algorithm::to_lower_copy(opt_format);

  if(!opt_place.empty())
	{
	  FmiNames::FmiLocationQueryOptions opts;
	  FmiNames::FmiLocationQuery query;
	  auto res = query.FetchByName(opts,opt_place);
	  if(res.empty())
		throw std::runtime_error("Unknown location '"+opt_place);
	  if(options.verbose)
		print_location(std::cerr,res[0]);

	  options.coordinate = NFmiPoint(res[0].lon,res[0].lat);
	}

  if(!opt_lonlat.empty())
	options.coordinate = parse_lonlat(opt_lonlat);

  if(!opt_latlon.empty())
	options.coordinate = parse_latlon(opt_lonlat);

  if(opt_longitude != kFloatMissing)
	options.coordinate.X(opt_longitude);

  if(opt_latitude != kFloatMissing)
	options.coordinate.Y(opt_latitude);

  if(options.coordinate.X() == kFloatMissing || options.coordinate.Y() == kFloatMissing)
	throw std::runtime_error("No simulation coordinate specified");

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
  trajectory->LatLon(options.coordinate);

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
  trajectory->Direction(options.backwards ? kBackward : kForward);

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
 * \brief Convert NFmiMetTime to ptime
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime to_ptime(const NFmiMetTime & theTime)
{
  boost::gregorian::date date(theTime.GetYear(),
							  theTime.GetMonth(),
							  theTime.GetDay());
  
  boost::posix_time::ptime utc(date,
							   boost::posix_time::hours(theTime.GetHour())+
							   boost::posix_time::minutes(theTime.GetMin())+
							   boost::posix_time::seconds(theTime.GetSec()));
  return utc;
}

// ----------------------------------------------------------------------
/*!
 * \brief Hash a single trajector
 */
// ----------------------------------------------------------------------

void hash_trajector(CTPP::CDT & hash, int index, const NFmiSingleTrajector & trajector)
{
  static Fmi::TimeFormatter * isoformatter   = Fmi::TimeFormatter::create("iso");
  static Fmi::TimeFormatter * epochformatter = Fmi::TimeFormatter::create("epoch");
  static Fmi::TimeFormatter * stampformatter = Fmi::TimeFormatter::create("timestamp");
  static Fmi::TimeFormatter * sqlformatter   = Fmi::TimeFormatter::create("sql");
  static Fmi::TimeFormatter * xmlformatter   = Fmi::TimeFormatter::create("xml");

  // Data being printed

  const auto & points = trajector.Points();
  const auto & pressures = trajector.Pressures();
  const auto & heights = trajector.HeightValues();
  
  // For iteration
  int timestep = options.timestep;
  if(options.backwards)
	timestep = -timestep;

  // Common information

  hash["index"] = index;

  if(index == 0)
	hash["name"] = "Main trajectory";
  else
	hash["name"] = "Plume " + boost::lexical_cast<std::string>(index);

  // Start time

  NFmiMetTime t(trajector.StartTime());
  t.SetTimeStep(1);

  for(size_t i=0; i<points.size(); i++)
	{
	  CTPP::CDT & group = hash["points"][i];

	  boost::posix_time::ptime pt = to_ptime(t);

	  group["timestamp"] = stampformatter->format(pt);
	  group["epoch"]     = epochformatter->format(pt);
	  group["isotime"]   = isoformatter->format(pt);
	  group["sqltime"]   = sqlformatter->format(pt);
	  group["xmltime"]   = xmlformatter->format(pt);

	  group["longitude"] = round(10000*points[i].X())/10000;
	  group["latitude"]  = round(10000*points[i].Y())/10000;
	  group["pressure"]  = round(10*pressures[i])/10;
	  group["height"]    = round(10*heights[i])/10;

	  t.ChangeByMinutes(timestep);
	}
}


// ----------------------------------------------------------------------
/*!
 * \brief Build CTTP hash of the results
 *
 * We assume the input hash is clear.
 *
 * General structure:
 *
 *  direction
 *  producer
 *  trajectory
 *  list<plumes>
 *
 * Trajectory contains points only:
 *
 *  time
 *  longitude
 *  latitude
 *  pressure
 *  height
 */
// ----------------------------------------------------------------------

void build_hash(CTPP::CDT & hash,
				const NFmiTrajectory & trajectory)
{
  hash["direction"] = pretty_direction(trajectory.Direction());
  hash["producer"] = trajectory.Producer().GetName().CharPtr();
  hash["pressure"] = options.pressure;
  hash["longitude"] = options.coordinate.X();
  hash["latitude"] = options.coordinate.Y();

  if(options.plumesize > 0)
	{
	  hash["radius"] = options.arearadius;
	  hash["disturbance"] = options.disturbance;
	  hash["interval"] = options.timeinterval;
	  hash["range"] = options.pressurerange;
	}


  CTPP::CDT & tgroup = hash["trajectories"];

  int index = 0;
  hash_trajector(tgroup[index],index,trajectory.MainTrajector());

  if(trajectory.PlumesUsed())
	{
	  const auto & plumes = trajectory.PlumeTrajectories();
	  BOOST_FOREACH(const auto & traj, plumes)
		{
		  ++index;
		  hash_trajector(tgroup[index],index,*traj);
		}
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Establish template file
 */
// ----------------------------------------------------------------------

std::string template_filename()
{
  if(!options.templatefile.empty())
	return options.templatefile;

  return (options.templatedir + "/" + options.format + ".c2t");
}

// ----------------------------------------------------------------------
/*!
 * \brief Format the result
 */
// ----------------------------------------------------------------------

std::string format_result(boost::shared_ptr<NFmiTrajectory> trajectory)
{
  // Get the output template

  std::string tmpl = template_filename();

  if(!boost::filesystem::exists(tmpl))
	throw std::runtime_error("Template file '"+tmpl+"' is missing");

  if(options.verbose)
	std::cerr << "Format template file is '" << tmpl << "'\n";

  // Build the hash

  CTPP::CDT hash;
  build_hash(hash, *trajectory);

  if(options.debughash)
	std::cerr << hash.Dump() << std::endl;

  // Format data using the template

  std::ostringstream output;
  std::ostringstream log;
  try
	{
	  BrainStorm::TemplateFormatter formatter;
	  formatter.load_template(tmpl);
	  formatter.process(hash, output, log);
	}
  catch(const std::exception &)
	{
	  std::cerr << log.str() << std::endl;
	  throw;
	}

  return output.str();
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

  if(options.verbose)
	options.report(std::cerr);

  // Read input data

  if(options.verbose)
	std::cerr << "Reading querydata from " << pretty_input_filename(options.queryfile) << std::endl;
  NFmiQueryData qd(options.queryfile);
  auto qi = boost::make_shared<NFmiFastQueryInfo>(&qd);

  // Verify the data is suitable

  auto leveltype = qi->Level()->LevelType();
  if(leveltype != kFmiHybridLevel && leveltype != kFmiPressureLevel)
	throw std::runtime_error("The input querydata must contain hybrid or pressure level data");

  // Calculate the trajectory

  auto trajectory = calculate_trajectory(qi);

  // Format the result

  std::string result;

  if(options.format == "debug")
	result = trajectory->ToXMLStr();
  else
	result = format_result(trajectory);

  // Write the output

  if(options.outfile == "-")
	std::cout << result << std::endl;
  else
	{
	  std::ofstream out(options.outfile.c_str());
	  if(!out)
		throw std::runtime_error("Failed to open '"+options.outfile+"' for writing");
	  if(options.verbose)
		std::cerr << "Writing to '" + options.outfile + "'\n";
	  out << result;
	  out.close();
	}

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
        std::cerr << "Error: Caught an unknown exception\n";
        return 1;
  }
