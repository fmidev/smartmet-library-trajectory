#include "NFmiSingleTrajector.h"
#include "NFmiTrajectory.h"
#include "NFmiTrajectorySystem.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <locus/Query.h>
#include <locus/QueryOptions.h>
#include <macgyver/AnsiEscapeCodes.h>
#include <macgyver/StringConversion.h>
#include <macgyver/TemplateFormatter.h>
#include <macgyver/TimeFormatter.h>
#include <macgyver/TimeParser.h>
#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiQueryData.h>
#include <newbase/NFmiSettings.h>
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
  void report(std::ostream &out) const;

  bool verbose;
  NFmiPoint coordinate;
  boost::posix_time::ptime starttime;
  unsigned int timestep;
  unsigned int hours;
  std::string queryfile;
  std::string outfile;
  std::string templatedir;
  std::string templatefile;
  std::string format;
  unsigned int plumesize;
  double disturbance;
  boost::optional<double> arearadius;
  boost::optional<unsigned int> timeinterval;
  boost::optional<double> pressure;
  boost::optional<double> pressurerange;
  boost::optional<double> height;
  boost::optional<double> heightrange;
  bool isentropic;
  bool backwards;
  bool compress;
  bool debughash;

  bool kml_tessellate;
  bool kml_extrude;
};

Options options;

// ----------------------------------------------------------------------
/*!
 * \brief Default command line options
 */
// ----------------------------------------------------------------------

Options::Options()
    : verbose(false),
      coordinate(NFmiPoint(kFloatMissing, kFloatMissing)),
      starttime(),
      timestep(10),
      hours(24),
      queryfile("-"),
      outfile("-"),
      templatedir("/usr/share/smartmet/trajectory"),
      templatefile(),
      format("xml"),
      plumesize(0),
      disturbance(25),
      arearadius(),
      timeinterval(),
      pressure(),
      pressurerange(),
      height(),
      heightrange(),
      isentropic(false),
      backwards(false),
      compress(false),
      debughash(false),
      kml_tessellate(false),
      kml_extrude(false)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Print the parsed options
 */
// ----------------------------------------------------------------------

void Options::report(std::ostream &out) const
{
#define HEADER(stream, title)                                                                 \
  stream << ANSI_BOLD_ON << ANSI_UNDERLINE_ON << title << ANSI_BOLD_OFF << ANSI_UNDERLINE_OFF \
         << '\n';
#define REPORT(stream, name, value)                                                          \
  stream << ANSI_ITALIC_ON << std::setw(35) << std::left << name << ANSI_ITALIC_OFF << value \
         << '\n';

  HEADER(out, "Options summary");
  REPORT(out, "Verbose:", verbose);
  REPORT(out, "Longitude:", coordinate.X());
  REPORT(out, "Latitude:", coordinate.Y());
  REPORT(out, "Start time:", to_iso_extended_string(starttime));
  REPORT(out, "Time step in minutes:", timestep);
  REPORT(out, "Simulation length in hours:", hours);
  REPORT(out, "Input querydata:", queryfile);
  REPORT(out, "Output file:", outfile);
  REPORT(out, "Output format:", format);
  REPORT(out, "Compression:", compress);
  REPORT(out, "Template directory:", templatedir);
  REPORT(out, "Template file:", templatefile);
  REPORT(out, "Plume size:", plumesize);
  REPORT(out, "Plume disturbance factor:", disturbance);
  if (arearadius) REPORT(out, "Plume dispersion radius in km:", *arearadius);
  if (timeinterval) REPORT(out, "Plume time dispersion in minutes:", *timeinterval);
  if (pressure) REPORT(out, "Plume pressure level:", *pressure);
  if (pressurerange) REPORT(out, "Plume pressure dispersion in hPa:", *pressurerange);
  REPORT(out, "Isentropic mode:", isentropic);
  REPORT(out, "Backward simulation in time:", backwards);
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

  if (!fs::exists(p))
    throw std::runtime_error("Template directory '" + options.templatedir + "' does not exist");

  if (!fs::is_directory(p))
    throw std::runtime_error("Not a directory: '" + options.templatedir + "'");

  std::cerr << "Formats found from template directory '" << options.templatedir << "':\n\n";

  size_t count = 0;
  for (fs::directory_iterator i(p), end; i != end; ++i)
  {
    if (i->path().extension() == ".c2t")
      std::cerr << std::setw(3) << std::right << ++count << ". " << i->path().stem().string()
                << '\n';
  }

  std::cerr << std::endl << "Found " << count << (count != 1 ? " formats.\n" : " format.\n");
}

// ----------------------------------------------------------------------
/*!
 * \brief Easier representation for direction
 */
// ----------------------------------------------------------------------

int pretty_direction(FmiDirection dir)
{
  switch (dir)
  {
    case kForward:
      return 1;
    case kBackward:
      return -1;
    default:
      throw std::runtime_error("Invalid trajectory direction encountered");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a lon,lat coordinate
 */
// ----------------------------------------------------------------------

NFmiPoint parse_lonlat(const std::string &theStr)
{
  std::vector<std::string> parts;
  boost::algorithm::split(parts, theStr, boost::is_any_of(","));

  if (parts.size() != 2)
    throw std::runtime_error("Invalid coordinate specification '" + theStr + "'");

  return NFmiPoint(Fmi::stod(parts[0]), Fmi::stod(parts[1]));
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a lat,lon coordinate
 */
// ----------------------------------------------------------------------

NFmiPoint parse_latlon(const std::string &theStr)
{
  std::vector<std::string> parts;
  boost::algorithm::split(parts, theStr, boost::is_any_of(","));

  if (parts.size() != 2)
    throw std::runtime_error("Invalid coordinate specification '" + theStr + "'");

  return NFmiPoint(Fmi::stod(parts[1]), Fmi::stod(parts[0]));
}

// ----------------------------------------------------------------------
/*!
 * \brief Print location information
 */
// ----------------------------------------------------------------------

void print_location(std::ostream &out, const Locus::SimpleLocation &theLoc)
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
 * \brief Parse the start time of the simulation
 */
// ----------------------------------------------------------------------

boost::posix_time::ptime parse_starttime(const std::string &theStr)
{
  boost::posix_time::ptime p;

  if (theStr == "now")
    p = Fmi::TimeParser::parse("0");
  else if (theStr != "origintime")
    p = Fmi::TimeParser::parse(theStr);

  // Round up to nearest timestep
  if (!p.is_not_a_date_time() && options.timestep > 1)
  {
    int offset = p.time_of_day().total_seconds() % (60 * options.timestep);
    if (offset > 0) p += boost::posix_time::seconds(60 * options.timestep - offset);
  }

  return p;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse command line options
 * \return True, if execution may continue as usual
 */
// ----------------------------------------------------------------------

bool parse_options(int argc, char *argv[])
{
  namespace po = boost::program_options;
  namespace fs = boost::filesystem;

  std::string opt_format;

  double opt_longitude = kFloatMissing;
  double opt_latitude = kFloatMissing;
  std::string opt_lonlat;
  std::string opt_latlon;
  std::string opt_place;
  std::string opt_starttime = "now";

  po::options_description desc("Available options");
  desc.add_options()("help,h", "print out usage information")("version,V",
                                                              "display version number")(
      "verbose,v", po::bool_switch(&options.verbose), "verbose mode")(
      "querydata,q", po::value(&options.queryfile), "input querydata (standard input)")(
      "outfile,o", po::value(&options.outfile), "output file name")(
      "place,p", po::value(&opt_place), "location name")(
      "lonlat", po::value(&opt_lonlat), "longitude,latitude")(
      "latlon", po::value(&opt_latlon), "latitude,longitude")(
      "longitude,x", po::value(&opt_longitude), "dispersal longitude")(
      "latitude,y", po::value(&opt_latitude), "dispersal latitude")(
      "format,f", po::value(&opt_format), "output format (kml)")(
      "list-formats", "list known output formats and exit")(
      "templatedir", po::value(&options.templatedir), "template directory for output formats")(
      "template", po::value(&options.templatefile), "specific output template file to use")(
      "starttime,t",
      po::value(&opt_starttime),
      "simulation start time (now, origintime or formatted time)")(
      "timestep,T", po::value(&options.timestep), "time step in minutes (10)")(
      "hours,H", po::value(&options.hours), "simulation length in hours (24)")(
      "plume-size,N", po::value(&options.plumesize), "plume size (0)")(
      "disturbance,D", po::value(&options.disturbance), "plume disturbance factor (25)")(
      "radius,R", po::value<double>(), "plume dispersal radius in km (0)")(
      "interval,I", po::value<unsigned int>(), "plume dispersal time interval (+-) in minutes (0)")(
      "pressure,P", po::value<double>(), "initial dispersal pressure")(
      "pressure-range", po::value<double>(), "plume dispersal pressure range")(
      "height,Z", po::value<double>(), "plume dispersal height")(
      "height-range", po::value<double>(), "plume dispersal height range")(
      "isentropic,i", po::bool_switch(&options.isentropic), "isentropic simulation")(
      "backwards,b", po::bool_switch(&options.backwards), "backwards simulation in time")(
      "compress,z", po::bool_switch(&options.compress), "gzip the output")(
      "debug-hash", po::bool_switch(&options.debughash), "print the internal hash tables")(
      "kml-tessellate", po::bool_switch(&options.kml_tessellate), "tessellate KML tracks")(
      "kml-extrude", po::bool_switch(&options.kml_extrude), "extrude KML tracks");

  po::positional_options_description p;
  p.add("querydata", 1);

  po::variables_map opt;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), opt);

  po::notify(opt);

  if (opt.count("version") != 0)
  {
    std::cout << "qdtrajectory v1.0 (" << __DATE__ << ' ' << __TIME__ << ")\n";
    return false;
  }

  if (opt.count("help"))
  {
    std::cout << "Usage: qdtrajectory [options] querydata\n"
                 "       qdtrajectory -i querydata [options]\n"
                 "       qdtrajectory [options] < querydata\n"
                 "       cat querydata | qdtrajectory [options]\n"
                 "\n"
                 "Calculate trajectories based on model or pressure level querydata.\n"
                 "The program always calculates one main trajectory. An additional\n"
                 "plume of perturbed trajectories can be simulated by selecing a plume\n"
                 "size and adjusting the relevant perturbation parameters.\n"
                 "\n"
                 "The supported output formats can be listed using the --list-formats\n"
                 "option, which essentially just lists the output templates found from\n"
                 "the template directory. Some common output formats include:\n"
                 "\n"
                 "     gpx   GPS eXchange format\n"
                 "     kml   Keyhole markup language\n"
                 "     kmz   Compressed KML\n"
                 "\n"
                 "In addition the extended KML format with the gx:track element is supported\n"
                 "with format names 'kmlx' and 'kmzx'.\n"
                 "\n"
                 "The 'debug' format dumps the contents of the respective C++ objects\n"
                 "in a legacy XML format, and is not intended for production use.\n"
                 "\n";

    std::cout << desc << '\n';
    return false;
  }

  if (opt.count("list-formats"))
  {
    list_formats();
    return false;
  }

  if (opt.count("radius")) options.arearadius = opt["radius"].as<double>();

  if (opt.count("interval")) options.timeinterval = opt["interval"].as<unsigned int>();

  if (opt.count("pressure")) options.pressure = opt["pressure"].as<double>();

  if (opt.count("pressure-range")) options.pressurerange = opt["pressure-range"].as<double>();

  if (opt.count("height")) options.height = opt["height"].as<double>();

  if (opt.count("height-range")) options.heightrange = opt["height-range"].as<double>();

  if (options.queryfile != "-" && !fs::exists(options.queryfile))
    throw std::runtime_error("Input file '" + options.queryfile + "' does not exist");

  if (!opt_format.empty()) options.format = boost::algorithm::to_lower_copy(opt_format);

  if (!opt_place.empty())
  {
    Locus::QueryOptions opts;
    auto username = NFmiSettings::Require<std::string>("LOCUS_USER");
    auto password = NFmiSettings::Require<std::string>("LOCUS_PASSWD");
    auto host = NFmiSettings::Require<std::string>("LOCUS_HOST");
    std::string database = "fminames";
    Locus::Query query(host, username, password, database);
    auto res = query.FetchByName(opts, opt_place);
    if (res.empty()) throw std::runtime_error("Unknown location '" + opt_place);
    if (options.verbose) print_location(std::cerr, res[0]);

    options.coordinate = NFmiPoint(res[0].lon, res[0].lat);
  }

  if (!opt_lonlat.empty()) options.coordinate = parse_lonlat(opt_lonlat);

  if (!opt_latlon.empty()) options.coordinate = parse_latlon(opt_lonlat);

  if (opt_longitude != kFloatMissing) options.coordinate.X(opt_longitude);

  if (opt_latitude != kFloatMissing) options.coordinate.Y(opt_latitude);

  if (options.coordinate.X() == kFloatMissing || options.coordinate.Y() == kFloatMissing)
    throw std::runtime_error("No simulation coordinate specified");

  if (!opt_starttime.empty()) options.starttime = parse_starttime(opt_starttime);

  if (!options.pressure && !options.height)
    throw std::runtime_error("Must specify simulation start pressure or height");

  if (options.pressure && options.height)
    throw std::runtime_error("Cannot specify both simulation start pressure and height");

  if (options.pressurerange && options.heightrange)
    throw std::runtime_error("Cannot specify both simulation pressure and height ranges");

  if (options.heightrange && !options.height)
    throw std::runtime_error(
        "Specifying simulation height range without a height is not supported");

  if (options.plumesize == 0)
  {
    if (options.arearadius || options.timeinterval || options.pressurerange || options.heightrange)
      throw std::runtime_error(
          "Specifying intervals or ranges when plume size is 0 is meaningless");
  }

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Find maximum value at a location
 */
// ----------------------------------------------------------------------

float maximum_value_vertically(NFmiFastQueryInfo &theQ,
                               const NFmiPoint &thePoint,
                               const NFmiMetTime &theTime)
{
  float pmax = kFloatMissing;
  for (theQ.ResetLevel(); theQ.NextLevel();)
  {
    float value = theQ.InterpolatedValue(thePoint, theTime);
    if (pmax == kFloatMissing)
      pmax = value;
    else if (value != kFloatMissing)
      pmax = std::max(pmax, value);
  }
  return pmax;
}

// ----------------------------------------------------------------------
/*!
 * \brief Calculate a trajectory
 */
// ----------------------------------------------------------------------

boost::shared_ptr<NFmiTrajectory> calculate_trajectory(
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  auto trajectory = boost::make_shared<NFmiTrajectory>();

  // Copy producer information
  trajectory->Producer(*theInfo->Producer());

  // trajectory->DataType(...)					// Unused

  // Point of interest
  trajectory->LatLon(options.coordinate);

  // Start time, time step and simulation length
  if (options.starttime.is_not_a_date_time())
    trajectory->Time(theInfo->OriginTime());
  else
    trajectory->Time(options.starttime);

  trajectory->TimeStepInMinutes(options.timestep);
  trajectory->TimeLengthInHours(options.hours);

  // Plume mode, disturbance factor as a percentage, plume size
  trajectory->PlumesUsed(options.plumesize > 0);
  trajectory->PlumeParticleCount(options.plumesize);
  trajectory->PlumeProbFactor(options.disturbance);

  // Forward or backward in time
  trajectory->Direction(options.backwards ? kBackward : kForward);

  // Follow potential temperature
  trajectory->Isentropic(options.isentropic);

  // Balloon trajectories
  // trajectory->CalcTempBalloonTrajectors(false);
  // trajectory->TempBalloonTrajectorSettings(NFmiTempBalloonTrajectorSettings());

  // Plume initial dispersion radius, start time interval, and pressure disribution
  if (options.arearadius) trajectory->StartLocationRangeInKM(*options.arearadius);
  if (options.timeinterval) trajectory->StartTimeRangeInMinutes(*options.timeinterval);
  if (options.pressure) trajectory->PressureLevel(*options.pressure);
  if (options.pressurerange) trajectory->StartPressureLevelRange(*options.pressurerange);

  // Convert input height parameters to simulation pressure parameters
  if (options.height)
  {
    if (!theInfo->Param(kFmiPressure))
      throw std::runtime_error("Pressure parameter missing from forecast data");

    // maximum pressure = lowest level value, but we do not know the order of the levels
    float pmax = maximum_value_vertically(*theInfo, trajectory->LatLon(), trajectory->Time());

    if (!options.heightrange)
    {
      float p = theInfo->HeightValue(*options.height, trajectory->LatLon(), trajectory->Time());

      if (p != kFloatMissing)
        trajectory->PressureLevel(p);
      else
        trajectory->PressureLevel(pmax);
    }
    else
    {
      float dz = *options.heightrange / 2;
      float p1 =
          theInfo->HeightValue(*options.height - dz, trajectory->LatLon(), trajectory->Time());
      float p2 =
          theInfo->HeightValue(*options.height + dz, trajectory->LatLon(), trajectory->Time());

      if (p2 == kFloatMissing) p2 = pmax;
      if (p1 == kFloatMissing) p1 = pmax;

      trajectory->PressureLevel((p1 + p2) / 2);
      trajectory->StartPressureLevelRange((p1 - p2) / 2);
    }
  }

  NFmiTrajectorySystem::CalculateTrajectory(trajectory, theInfo);

  return trajectory;
}

// ----------------------------------------------------------------------
/*!
 * \brief Hash a single trajector
 */
// ----------------------------------------------------------------------

void hash_trajector(CTPP::CDT &hash, int index, const NFmiSingleTrajector &trajector)
{
  static Fmi::TimeFormatter *isoformatter = Fmi::TimeFormatter::create("iso");
  static Fmi::TimeFormatter *epochformatter = Fmi::TimeFormatter::create("epoch");
  static Fmi::TimeFormatter *stampformatter = Fmi::TimeFormatter::create("timestamp");
  static Fmi::TimeFormatter *sqlformatter = Fmi::TimeFormatter::create("sql");
  static Fmi::TimeFormatter *xmlformatter = Fmi::TimeFormatter::create("xml");

  // Data being printed

  const auto &points = trajector.Points();
  const auto &pressures = trajector.Pressures();
  const auto &heights = trajector.HeightValues();

  // For iteration
  int timestep = options.timestep;
  if (options.backwards) timestep = -timestep;

  // Common information

  hash["index"] = index;

  if (index == 0)
    hash["name"] = "Main trajectory";
  else
    hash["name"] = "Plume " + Fmi::to_string(index);

  // Start time

  NFmiMetTime t(trajector.StartTime());
  t.SetTimeStep(1);

  for (size_t i = 0; i < points.size(); i++)
  {
    CTPP::CDT &group = hash["points"][i];

    boost::posix_time::ptime pt = t;

    // Track start and end times
    if (i == 0)
      hash["starttime"] = xmlformatter->format(pt);
    else if (i == points.size() - 1)
      hash["endtime"] = xmlformatter->format(pt);

    group["timestamp"] = stampformatter->format(pt);
    group["epoch"] = epochformatter->format(pt);
    group["isotime"] = isoformatter->format(pt);
    group["sqltime"] = sqlformatter->format(pt);
    group["xmltime"] = xmlformatter->format(pt);

    group["longitude"] = round(10000 * points[i].X()) / 10000;
    group["latitude"] = round(10000 * points[i].Y()) / 10000;
    group["pressure"] = round(10 * pressures[i]) / 10;
    group["height"] = round(10 * heights[i]) / 10;

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

void build_hash(CTPP::CDT &hash, const NFmiTrajectory &trajectory)
{
  hash["direction"] = pretty_direction(trajectory.Direction());
  hash["producer"] = trajectory.Producer().GetName().CharPtr();
  hash["pressure"] = trajectory.PressureLevel();
  hash["longitude"] = trajectory.LatLon().X();
  hash["latitude"] = trajectory.LatLon().Y();

  hash["kml"]["tessellate"] = options.kml_tessellate ? 1 : 0;
  hash["kml"]["extrude"] = options.kml_extrude ? 1 : 0;

  if (options.plumesize > 0)
  {
    hash["plume"]["disturbance"] = trajectory.PlumeProbFactor();
    hash["plume"]["radius"] = trajectory.StartLocationRangeInKM();
    hash["plume"]["interval"] = trajectory.StartTimeRangeInMinutes();
    hash["plume"]["range"] = trajectory.StartPressureLevelRange();
  }

  CTPP::CDT &tgroup = hash["trajectories"];

  int index = 0;
  hash_trajector(tgroup[index], index, trajectory.MainTrajector());

  if (trajectory.PlumesUsed())
  {
    const auto &plumes = trajectory.PlumeTrajectories();
    BOOST_FOREACH (const auto &traj, plumes)
    {
      ++index;
      hash_trajector(tgroup[index], index, *traj);
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
  if (!options.templatefile.empty()) return options.templatefile;

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

  if (!boost::filesystem::exists(tmpl))
    throw std::runtime_error("Template file '" + tmpl + "' is missing");

  if (options.verbose) std::cerr << "Format template file is '" << tmpl << "'\n";

  // Build the hash

  CTPP::CDT hash;
  build_hash(hash, *trajectory);

  if (options.debughash) std::cerr << hash.Dump() << std::endl;

  // Format data using the template

  std::ostringstream output;
  std::ostringstream log;
  try
  {
    Fmi::TemplateFormatter formatter;
    formatter.load_template(tmpl);
    formatter.process(hash, output, log);
  }
  catch (const std::exception &)
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

std::string pretty_input_filename(const std::string &theName)
{
  if (theName == "-")
    return "standard input";
  else
    return "'" + theName + "'";
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program without exception handling
 */
// ----------------------------------------------------------------------

int run(int argc, char *argv[])
{
  if (!parse_options(argc, argv)) return 0;

  if (options.verbose) options.report(std::cerr);

  // Read input data

  if (options.verbose)
    std::cerr << "Reading querydata from " << pretty_input_filename(options.queryfile) << std::endl;
  NFmiQueryData qd(options.queryfile);
  auto qi = boost::make_shared<NFmiFastQueryInfo>(&qd);

  // Verify the data is suitable

  auto leveltype = qi->Level()->LevelType();
  if (leveltype != kFmiHybridLevel && leveltype != kFmiPressureLevel)
    throw std::runtime_error("The input querydata must contain hybrid or pressure level data");

  // Calculate the trajectory

  auto trajectory = calculate_trajectory(qi);

  // Format the result

  std::string result;

  if (options.format == "debug")
    result = trajectory->ToXMLStr();
  else
    result = format_result(trajectory);

  // Write the output

  boost::iostreams::filtering_stream<boost::iostreams::output> filter;

  if (options.compress || options.format == "kmz" || options.format == "kmzx")
    filter.push(boost::iostreams::gzip_compressor());

  if (options.outfile == "-")
  {
    filter.push(std::cout);
    filter << result;
    filter.flush();
    filter.reset();
  }
  else
  {
    std::ofstream out(options.outfile.c_str());
    if (!out) throw std::runtime_error("Failed to open '" + options.outfile + "' for writing");

    if (options.verbose) std::cerr << "Writing to '" + options.outfile + "'\n";

    filter.push(out);
    filter << result;
    filter.flush();
    filter.reset();
    out.close();
  }

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program
 */
// ----------------------------------------------------------------------

int main(int argc, char *argv[]) try
{
  return run(argc, argv);
}
catch (std::exception &e)
{
  std::cerr << "Error: " << e.what() << std::endl;
  return 1;
}
catch (...)
{
  std::cerr << "Error: Caught an unknown exception\n";
  return 1;
}
