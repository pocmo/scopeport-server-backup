// This file is part of ScopePort (Linux server).
//
// Copyright 2007, 2008, 2009 Lennart Koopmann
//
// ScopePort (Linux server) is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ScopePort (Linux server) is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ScopePort (Linux server).  If not, see <http://www.gnu.org/licenses/>.

// The database class.
#include "database/Database.h"
// Host handling.
#include "host/Host.h"
// Services.
#include "services/Services.h"
// Warning conditions.
#include "notifications/Warning.h"
// Converting numbers to names etc.
#include "database/Information.h"
// This class is used for sending mails.
#include "notifications/Mail.h"
// The class for XMPP conversations.
#include "notifications/XMPP.h"
// General notifications.
#include "notifications/GeneralNotifications.h"
// This is responsible for sending emergency notifications.
#include "notifications/EmergencyNotifications.h"
// This is the class which is organizing the logging of events.
#include "log/Log.h"
// This class provides information about the calling process.
#include "health/Health.h"
// Clickatell SMS API.
#include "notifications/Clickatell.h"
// The cloud/clustering methods.
#include "cloud/Cloud.h"

#include "internal.h"

mySQLData dbData;
mailingData mailData;
XMPPData xmppData;
mobilecData clickatellData;

bool debug = 0;

unsigned int nodeID = 0;

bool clientHandler = 0;

gnutls_anon_server_credentials_t anoncred;

int percentalDifference(int i1, int i2)
{
  // Check if there is a difference.
  if(i1 == i2)
  {
    return 0;
  }

  int more = 0;
  int less = 0;

  // Find out which is the higher value.
  if(i1 > i2)
  {
    more = i1;
    less = i2;
  }
  else
  {
    more = i2;
    less = i1;
  }

  // Return the percental difference.
  return 100-((less*100)/more);
}


int stringToInteger(string st)
{
  int result;
  if(stringstream(st) >> result)
  {
    return result;
  }
  else
  {
    return 0;
  }
}


string integerToString(int i)
{
  stringstream res;
  res << i;
  return res.str();
}


bool numOnly(string checkMe)
{
  bool result;
  if(checkMe.empty())
    return 0;
  int checkMe_length = checkMe.length();
  for(int i=0; i < checkMe_length; i++)
  {
    if(isdigit(checkMe.at(i)))
    {
      result = 1;
    }
    else
    {
      // Stop and return false if a non-digit char was detected.
      return 0;
      break;
    }
  }
  return result;
}


string selfTest(int port, int loglevel)
{
  // Check if port is numOnly.
  if(port <= 0)
    return "Port is in wrong format. Only numbers allowed.";

  // Check if logfile is writable.
  ofstream testlog(LOGFILE, ios::app);
  if(testlog.fail())
  {
    testlog.close();
    // Opening failed. Try to create the logfile.
    if(fopen(LOGFILE,"w") == NULL)
      return "Could not open logfile. Could not create logfile.";
    ofstream testlog(LOGFILE, ios::app);
    // Could not open (maybe) created file. Abort.
    if(testlog.fail()) return "Could not open logfile for writing."
        "						Check permissions and if file exists.";
    testlog.close();
  }

  // Check if loglevel value is in range.
  if(loglevel < 0 || loglevel > 1) return "Loglevel must be between 0 or 1.";

  return "1";
}


string noSpaces(string str)
{
  int i;
  if(str.length() > 0)
  {
    int strlen = str.length();
    stringstream newstr;
    for(i = 0; i < strlen; i++)
    {
      if(!isspace(str.at(i)))
      {
        newstr << str.at(i);
      }
    }
    return newstr.str();
  }
  return "err";
}


char* noNewLine(char* str, int size )
{
  int i;
  for (i = 0; i < size; ++i)
  {
    if(str[i] == '\n' )
    {
      str[i] = '\0';
      return str;
    }
  }
  return str;
}


string noNewLineS(string str)
{
  stringstream newStr;
  istringstream iss(str);
  string token;

  while(getline(iss, token, '\n'))
  {
    newStr << token << " ";
  }

  return newStr.str();
}


unsigned long resolveName(char* server)
{
  if(strlen(server) <= 0)
    return 0;
  struct hostent *host;
  if((host = gethostbyname(server)) == NULL)
    return 0;
  return *((unsigned long*) host->h_addr_list[0]);
}


void* serviceHandler(void* arg)
{
  Log::debug(debug, "serviceHandler(): Started.");
  Log log(LOGFILE, dbData);
  Database db(dbData);

  // Calculate a kinda random ID for this handler.
  static timeval tv;
  static timeval tv2;
  static struct timezone tz;
  gettimeofday(&tv, &tz);
  gettimeofday(&tv2, &tz);
  unsigned int handlerID = tv.tv_usec * 3;

  stringstream handlerIDStringS;
  handlerIDStringS << handlerID;
  string handlerIDString = handlerIDStringS.str();

  Services service(dbData, handlerID);

  // Try to establish a database connection.
  if(db.initConnection())
  {
    Log::debug(debug, "serviceHandler() " + handlerIDString + ": Connected to database.");
    time_t rawtime;
    time(&rawtime);
    int twoMinutesAgo = rawtime-120;
    // Fetch a service to handle.
    stringstream query;
    //query << "SELECT id FROM services WHERE (reserved_for = "
    //      << nodeID
    //      << " AND TIMEDIFF(reserved_on, NOW()) < '00:01:30') "
    //         "OR handler = 0 OR lastcheck < "
    //      << twoMinutesAgo;
    query << "SELECT id FROM services WHERE handler = 0 OR handler IS NULL OR lastcheck < "
      << twoMinutesAgo;
    if(mysql_real_query(db.getHandle(), query.str().c_str(), strlen(query.str().c_str())) == 0)
    {
      Log::debug(debug, "serviceHandler() " + handlerIDString + ": Initial query succeeded.");
      // Query successful.
      MYSQL_ROW serviceResult;
      MYSQL_RES* res = mysql_store_result(db.getHandle());
      serviceResult = mysql_fetch_row(res);
      if(mysql_num_rows(res) > 0)
      {
        // We fetched a service to handle.
        Log::debug(debug, "serviceHandler() " + handlerIDString + ": A service to handle has been fetched.");
        if(serviceResult[0] == NULL)
        {
          // We got NULL fields.
          Log::debug(debug, "serviceHandler() " + handlerIDString + ": ID is NULL. Terminating this thread.");
          mysql_free_result(res);
          mysql_close(db.getHandle());
          return arg;
        }

        // Tell the service object it's own ID.
        service.setServiceID(stringToInteger(serviceResult[0]));

        // Initially fill with settings. This will be repeated before every check.
        if(!service.updateSettings())
        {
          // Could not store the response time.
          Log::debug(debug, "serviceHandler() " + handlerIDString + ": Could not initially set service settings.");
          log.putLog(2, "xxx", "Could not initially set service settings.");
          service.updateStatus(SERVICE_STATE_INTERR);
          mysql_free_result(res);
          mysql_close(db.getHandle());
          return arg;
        }
        Log::debug(debug, "serviceHandler() " + handlerIDString + ": Service setings have been set initially.");
      }
      else
      {
        Log::debug(debug, "serviceHandler() " + handlerIDString + ": No service to handle found. Terminating this thread.");
        // No services have been fetched.
        mysql_free_result(res);
        mysql_close(db.getHandle());
        return arg;
      }
      mysql_free_result(res);
    }
    else
    {
      Log::debug(debug, "serviceHandler() " + handlerIDString + ": Could not fetch service to handle. Terminating this thread.");
      log.putLog(2, "2000", "Could not fetch service to handle.");
      mysql_close(db.getHandle());
      return arg;
    }

    // The service data is now available in MYSQL_ROW service.

    // Announce that this handler handles the service.
    stringstream announce;
    announce  << "UPDATE services SET handler = "
      << service.getHandlerID()
      << ", node_id = "
      << nodeID
      << ", reserved_for = NULL, reserved_on = NULL"
      << " WHERE id = "
      << service.getServiceID();

    if(!db.setQuery(db.getHandle(), announce.str()))
    {
      Log::debug(debug, "serviceHandler() " + handlerIDString + ": Could not announce handling of this service.");
      log.putLog(2, "3000", "Could not update service handler.");
      mysql_close(db.getHandle());
      return arg;
    }

    Log::debug(debug, "serviceHandler() " + handlerIDString + ": Handling of this service announced.");

    mysql_close(db.getHandle());

  }
  else
  {
    // Could not establish a database connection. Close this thread.
    Log::debug(debug, "serviceHandler() " + handlerIDString + ": Could not establish connection to database. Terminating this thread.");
    return arg;
  }

  // We got all the service data.

  // Run forever
  while(1)
  {
    // Check if this service is still wanted to be checked for.
    Log::debug(debug, "serviceHandler() " + handlerIDString + ": Entered checking loop.");
    if(db.initConnection())
    {
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Connected to database.");
      stringstream checkService;
      checkService  << "SELECT id FROM services WHERE id = "
        << service.getServiceID();
      //<< " AND (reserved_for = " << nodeID << " OR reserved_for IS NULL)";
      unsigned int serviceState = db.getNumOfResults(checkService.str());
      if(serviceState <= 0)
      {
        stringstream message;
        message << "Closing service handler #" << service.getHandlerID()
          << " as it is not needed anymore.";
        Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Closing service handler because serviceState is <= 0. Terminating this thread.");
        log.putLog(0, "NOTICE", message.str());
        mysql_close(db.getHandle());
        return arg;
      }
      mysql_close(db.getHandle());
    }
    else
    {
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Could not connect to database. Closing this handler.");
      log.putLog(2, "xxx", "Could not check if service still needs to be checked (Database error). Closing this handler.");
      return arg;
    }

    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: We still want to check this service.");
    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Updating service settings.");

    // Update settings.
    if(!service.updateSettings())
    {
      // Could not store the response time.
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Could not update service settings. Closing this handler.");
      log.putLog(2, "xxx", "Could not update service settings. Closing this handler.");
      return arg;
    }

    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Service settings updated.");
    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Starting check.");

    // Check service twice and build an average response time.
    int serviceResult = SERVICE_STATE_INTERR;
    int firstServiceResult = SERVICE_STATE_INTERR;
    int secondServiceResult = SERVICE_STATE_INTERR;
    for(int run = 0; run <= 1; run++)
    {
      serviceResult = service.checkService(run);
      if(run == 0)
      {
        firstServiceResult = serviceResult;
        if(debug)
        {
          stringstream srss;
          srss << firstServiceResult;
          Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Check result I: " + srss.str());
        }
      }
      else
      {
        secondServiceResult = serviceResult;
        if(debug)
        {
          stringstream srss;
          srss << secondServiceResult;
          Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Check result II: " + srss.str());
        }
      }
    }

    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Checking if the measured response time is higher than the defined maximum.");

    // Find out if the response time was too high.
    if(service.checkResponseTime() == 0)
    {
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Measured respone time is too high! Setting status to SERVICE_STATE_OKAYTIME.");
      // Mark service with "Too high response time"
      serviceResult = SERVICE_STATE_OKAYTIME;
    }
    else
    {
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Measured respone time is not too high!");
    }

    // Update the service status.
    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Updating status.");
    service.updateStatus(serviceResult);

    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Storing response time.");

    // Store the response time in the database.
    if(!service.storeResponseTime())
    {
      // Could not store the response time.
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Could not store response time. Closing this handler.");
      log.putLog(2, "008", "Could not update service response time. Closing this handler.");
      return arg;
    }

    if(serviceResult == SERVICE_STATE_CONFAIL || serviceResult == SERVICE_STATE_OKAYTIME
      || serviceResult == SERVICE_STATE_TIMEOUT)
    {
      /*
       * Check succeeded.
       *
       * The service is down, timed out or has a too high response time.
       * The method will find out if the response time was too high
       * or if the service is just down/not reachable.
       */
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Sending warnings.");
      service.sendWarning();
    }
    else if(serviceResult == SERVICE_STATE_INTERR)
    {
      /*
       * The service could not be checked because an internal error occured.
       * Send a general warning!
       */
      Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Sending general notifications because serviceResult is SERVICE_STATE_INTERR");
      GeneralNotifications gn(dbData, mailData, xmppData, clickatellData);
      gn.sendMessages("Critical failure", "Could not check service because an "
        "internal error occured");
    }

    Log::debug(debug, "serviceHandler() " + handlerIDString + " [loop]: Done. Next cycle in 60 seconds.");

    // Wait 60 seconds until next check.
    sleep(60);
  }

  return arg;
}


void* serviceChecks(void* arg)
{
  Log::debug(debug, "serviceChecks(): Started");
  Log log(LOGFILE, dbData);
  Database db(dbData);
  // Run forever.
  while(1)
  {
    Log::debug(debug, "serviceChecks(): In loop.");
    // Try to establish a database connection.
    if(db.initConnection())
    {
      Log::debug(debug, "serviceChecks(): Connected to database.");
      time_t rawtime;
      time(&rawtime);
      int twoMinutesAgo = rawtime-120;
      // Get the number of services that have no handler yet.
      stringstream query;
      //query << "SELECT id FROM services WHERE (reserved_for = "
      //      << nodeID
      //      << " AND TIMEDIFF(reserved_on, NOW()) < '00:01:30') "
      //         "OR handler = 0 OR lastcheck < "
      //      << twoMinutesAgo;
      query << "SELECT id FROM services WHERE handler = 0 OR handler = NULL OR lastcheck < "
        << twoMinutesAgo << " LIMIT 100";

      unsigned int numOfServices = db.getNumOfResults(query.str());
      if(debug)
      {
        stringstream msg;
        msg << "serviceChecks(): Fetched " << numOfServices << " services that need to get a serviceHandler()";
        Log::debug(debug, msg.str());
      }
      mysql_close(db.getHandle());

      // Start a thread for every service that has no handler yet.
      for(unsigned int i = 0;i < numOfServices; i++)
      {
        if(debug)
        {
          stringstream msg;
          msg << "serviceChecks(): Starting serviceHandler() " << i;
          Log::debug(debug, msg.str());
        }
        // Start thread.
        pthread_t thread;
        if(pthread_create(&thread, 0, serviceHandler, NULL))
        {
          log.putLog(2, "1000", "Could not create service thread");
          if(debug)
          {
            stringstream msg;
            msg << "serviceChecks(): Starting of serviceHandler() " << i << " failed";
            Log::debug(debug, msg.str());
          }
        }
        else
        {
          if(debug)
          {
            stringstream msg;
            msg << "serviceChecks(): Started serviceHandler() " << i;
            Log::debug(debug, msg.str());
          }
        }

        // Wait one second before starting the next thread.
        sleep(1);
      }
    }
    else
    {
      // Could not connect to database.
      log.putLog(2, "018", "Could not start service check module! (Database error) - "
        "Retry in 5 minutes.");
      Log::debug(debug, "serviceChecks(): Could not connect to database.");
      GeneralNotifications gn(dbData, mailData, xmppData, clickatellData);
      gn.sendMessages("Critical failure", "Could not start service check module! "
        "(Database error) Retry in 5 minutes.");

      // Sleep five minutes.
      sleep(300);
    }

    // Everything went fine. Wait a minute for next check.
    sleep(60);

  }

  return arg;
}


int servSock;
int clientSocket;
struct sockaddr_in address;
socklen_t addrlen;

int loglevel = 0;
bool blacklisting = 0;

// This counts the received packages.
long double packageCountOK = 0;
long double packageCountERR = 0;

void* cloudStatusUpdater(void* args)
{
  Log log(LOGFILE, dbData);

  Database db(dbData);
  Cloud cloud(nodeID, dbData);

  while(1)
  {
    if(db.initConnection())
    {
      while(cloud.updateOwnStatus(db))
      {
        sleep(10);
      }
      mysql_close(db.getHandle());
    }
    else
    {
      // Could not connect to database.
      log.putLog(2, "xxx", "Could not update own cloud status: Could not connect to database. Retry in one minute.");
    }
    sleep(60);
  }

  return (NULL);
}


void* cloudServiceManager(void* args)
{
  Log log(LOGFILE, dbData);

  Database db(dbData);

  while(1)
  {
    if(db.initConnection())
    {

      Cloud cloud(nodeID, dbData);

      /*
       * Service balance
       *
       */

      // Get number of currently self monitored services.
      static unsigned int ownServices = cloud.getNumberOfOwnServices(db);

      // Get ID of node with most services.
      static unsigned int nodeWithMostServices = cloud.getIdOfNodeWithMostServices(db);

      // Get the number of services the node with the most services currently handles.
      static unsigned int numberOfMostServices = cloud.getNumberOfServicesFromNode(nodeWithMostServices, db);

      /*
       * Skip everything if we are the node with the highest number of services or
       * the other node has less services than we have.
       */
      if(nodeID != nodeWithMostServices)
      {
        unsigned int numberOfRequestedServices = 0;
        if(ownServices > 0 && numberOfMostServices > ownServices)
        {
          numberOfRequestedServices = ceil((numberOfMostServices-ownServices)/2);
        }
        else
        {
          numberOfRequestedServices = ceil((numberOfMostServices)/2);
        }

        if(numberOfRequestedServices > 0)
        {
          // Request.
          if(cloud.action_requestServices(numberOfRequestedServices, nodeWithMostServices, db))
          {
            // Request was sent successful. Log it.
            stringstream logmsg;
            logmsg << "Requested " << numberOfRequestedServices << " services from node " << nodeWithMostServices;
            cloud.action_logEvent(logmsg.str(), db);
          }
          else
          {
            log.putLog(2, "xxx", "Could not request services from another node: Database error.");
          }
        }
      }

      // Check if we need to hand over a service.
      unsigned int conversationID;
      if((conversationID = cloud.checkForServiceHandoverRequest(db)) > 0)
      {
        unsigned int numberOfServices = cloud.getNumberOfRequestedServices(db, conversationID);
        unsigned int serviceHandoverRequester = cloud.getNodeIdOfHandoverRequester(db, conversationID);

        // Hand over services.
        if(cloud.handOverServices(numberOfServices, serviceHandoverRequester, db))
        {
          if(!cloud.action_replyServiceRequest(serviceHandoverRequester, "1", conversationID, db))
          {
            log.putLog(2, "xxx", "Could not reply to service request: Database error.");
          }
          stringstream logmsg;
          logmsg << "Accepted: Handing over " << numberOfServices << " services to node " << serviceHandoverRequester;
          cloud.action_logEvent(logmsg.str(), db);
        }
        else
        {
          if(!cloud.action_replyServiceRequest(serviceHandoverRequester, "0", conversationID, db))
          {
            log.putLog(2, "xxx", "Could not reply to service request: Database error.");
          }
          stringstream logmsg;
          logmsg << "Denied: Could not hand over services to node " << serviceHandoverRequester << ". General failure.";
          cloud.action_logEvent(logmsg.str(), db);
        }
      }

      mysql_close(db.getHandle());
    }
    else
    {
      // Could not connect to database.
      log.putLog(2, "xxx", "Could not update own cloud status: Could not connect to database. Retry in one minute.");
      sleep(55);
    }
    sleep(5);
  }

  return (NULL);
}


void* maintenanceThread(void* args)
{

  Log log(LOGFILE, dbData);

  while(1)
  {

    Database maintDB(dbData);

    if(maintDB.initConnection())
    {
      // Clear outdated sensor data.
      if(!maintDB.clearSensorData())
      {
        log.putLog(1, "021", "Could not delete old sensor data.");
      }

      // Clear outdated service data.
      if(!maintDB.clearServiceData())
      {
        log.putLog(1, "021", "Could not delete old service data.");
      }

      mailData = Mail::fetchSettings(dbData);
      xmppData = XMPP::fetchSettings(dbData);
      clickatellData = Clickatell::fetchSettings(dbData);

      // Update process statistics.
      if(!maintDB.setQuery(maintDB.getHandle(), Information::updateHealth(nodeID, Health::getPID(),
        clientHandler,Health::getVMSize(), Health::getThreads(), packageCountOK,
        packageCountERR, -1, -1, -1)))
      {
        log.putLog(1, "053", "Could not update health statistics.");
      }

      mysql_close(maintDB.getHandle());
    }

    // Run every minute.
    sleep(60);
  }

  // Not reached.

  return (NULL);
}


void killClient(int sig)
{
  Log log(LOGFILE, dbData);
  log.putLog(0, "029", "Closed connection to client that did not complete"
    " transaction after 5 seconds.");
  close(clientSocket);
}


void handleClient()
{

  // Start maintenance thread.
  pthread_t maintThread;
  if(pthread_create(&maintThread, 0, maintenanceThread, NULL))
  {
    cout << "Terminating: Could not create maintenance thread." << endl;
    exit(EXIT_FAILURE);
  }

  Log log(LOGFILE, dbData);

  // For closing connection to a client after 5sec.
  signal(SIGALRM, killClient);

  // Run forever.
  while(1)
  {

    // Accept connections with new socket "clientSocket".
    clientSocket = accept(servSock,(struct sockaddr *) &address, &addrlen);

    if(clientSocket <= 0)
    {
      log.putLog(1, "030", "Could not accept connection from client.");
      close(clientSocket);
      packageCountERR++;
      continue;
    }

    // Create database object.
    Database clientDB(dbData);

    // Try to open connection to database.
    if(clientDB.initConnection())
    {
      // We are connected to the database.

      Host host(dbData, clientSocket, address);

      // Start the timer. The client now has five seconds to complete transaction. Go.
      alarm(5);

      // Skip if this host is blacklisted.
      if(host.isBlacklisted(clientDB))
      {
        stringstream logmsg;
        logmsg << "Blacklisted host "
          << host.getIPv4Address()
          << " tried to connect. Blocked.";
        log.putLog(1, "xxx", logmsg.str());
        host.refuse("Blacklisted");
        mysql_close(clientDB.getHandle());
        close(clientSocket);
        // Reset the alarm timer.
        alarm(0);
        continue;
      }
      else
      {
        host.notify("Okay");
      }

      // Wait for host login. Skip if incorrect.
      if(!host.checkLogin(clientDB))
      {
        stringstream logmsg;
        string errorcode;
        // Blacklist host.
        if(host.addToBlacklist(clientDB))
        {
          errorcode = "xxx";
          logmsg << "Host "
            << host.getIPv4Address()
            << " sent wrong login credentials. Blocked and blacklisted.";
        }
        else
        {
          errorcode = "xxx";
          logmsg << "Host "
            << host.getIPv4Address()
            << " sent wrong login credentials. Blocked. Blacklisting failed.";
        }
        log.putLog(3, errorcode, logmsg.str());
        host.refuse("Login incorrect");
        mysql_close(clientDB.getHandle());
        close(clientSocket);
        // Reset the alarm timer.
        alarm(0);
        continue;
      }
      else
      {
        host.notify("Okay");
      }

      // Wait for data.
      if(!host.receiveAndStoreData(clientDB))
      {
        stringstream logmsg;
        logmsg << "Could not receive and store data from host "
          << host.getIPv4Address() << " (" << host.getLastError() << ")";
        log.putLog(3, "xxx", logmsg.str());
        host.refuse("Could not accept and store your data.");
        mysql_close(clientDB.getHandle());
        close(clientSocket);
        // Reset the alarm timer.
        alarm(0);
        continue;
      }
      else
      {
        host.notify("Okay");
      }

      // The data has been stored if we arrive here.

      // Reset the alarm timer.
      alarm(0);

      mysql_close(clientDB.getHandle());
    }
    else
    {
      // Could not connect to database.
      log.putLog(2, "049", "Client handler: Could not connect to database! "
        "Sensor package skipped.");
    }
    close(clientSocket);
  }

  // Not reached.

  exit(EXIT_FAILURE);
}


void cleanUp(int sig)
{
  remove(PIDFILE);
  gnutls_anon_free_server_credentials (anoncred);
  gnutls_global_deinit();
  exit(EXIT_FAILURE);
}


void logTLS(int level, const char *message)
{
  Log log(LOGFILE, dbData);
  stringstream logmsg;
  logmsg << message;
  log.putLog(1, "TLS", message);
}


int main(int argc, char *argv[])
{

  // So jung kommen wir nicht mehr zusammen.

  // Check if we are root.
  if(geteuid() != 0)
  {
    cout <<  "The server needs to be started as root." << endl;
    exit(EXIT_FAILURE);
  }

  // Find out if we are in debug mode.
  for(int i = 0; i < argc; i++)
  {
    if(strcmp(argv[i], "--debug") == 0)
    {
      debug = 1;
    }
  }

  // Check if config file is readable
  ifstream configtest(CONFIGFILE);
  if(configtest.fail())
  {
    cout << "Terminating: Could not read config file. Check permissions and if file exists." << endl;
    exit(EXIT_FAILURE);
  }
  configtest.close();

  /*
   * Read config.
   *
   * To add a new config parameter just add
   * the name of the parameter to the vector.
   *
   * It will be accessible as string config[nr]
   *
   */

  cout << "- Reading config:\t";
  cout.flush();
  vector<string> parameters;
  parameters.push_back("serverport");
  parameters.push_back("loglevel");
  parameters.push_back("servicechecks");
  parameters.push_back("blacklist");

  parameters.push_back("mysqlhost");
  parameters.push_back("mysqldb");
  parameters.push_back("mysqluser");
  parameters.push_back("mysqlpass");
  parameters.push_back("mysqlport");

  parameters.push_back("numprocs");

  parameters.push_back("mail-alt");
  parameters.push_back("xmpp-alt");
  parameters.push_back("mobilec-alt");

  parameters.push_back("nodeid");

  ifstream configstream(CONFIGFILE);

  int parsize = parameters.size();
  string config[parsize];

  if(!configstream.fail())
  {
    int i = 0;
    int j = 0;

    string configline;

    while(getline(configstream,configline))
    {
      i = 0;
      string token;
      istringstream iss(configline);
      while(j < parsize)
      {
        if(configline.find(parameters[j], 0 ) != string::npos)
        {
          while(getline(iss, token, '='))
          {
            // Read parameters.
            if(i == 1 && !token.empty())
            {
              config[j] = token;
            }
            i++;
          }
        }
        j++;
      }
      j = 0;
    }
  }

  configstream.close();

  // Working on the config.
  // Convert strings to int.
  int port;
  if(!config[0].empty())
  {
    port = stringToInteger(config[0].c_str());
  }
  else
  {
    cout << "Error. Port not set?" << endl;
    exit(EXIT_FAILURE);
  }

  if(!config[1].empty())
  {
    loglevel = stringToInteger(config[1].c_str());
  }
  else
  {
    cout << "Error. Loglevel not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Find out if we want to enable service checks.
  bool doServiceChecks = 0;
  if(!config[2].empty())
  {
    if(config[2] == "1")
      doServiceChecks = 1;
  }
  else
  {
    cout << "Error. Service check mode not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Find out if we want to enable the blacklist.
  if(!config[3].empty())
  {
    if(config[3] == "1")
      blacklisting = 1;
  }
  else
  {
    cout << "Error. Blacklisting mode not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Get MySQL host.
  if(!config[4].empty())
  {
    dbData.host = config[4].c_str();
  }
  else
  {
    cout << "Error. MySQL host not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Get MySQL database.
  if(!config[5].empty())
  {
    dbData.db = config[5].c_str();
  }
  else
  {
    cout << "Error. MySQL database not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Get MySQL user.
  if(!config[6].empty())
  {
    dbData.user = config[6].c_str();
  }
  else
  {
    cout << "Error. MySQL user not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Get MySQL password.
  if(!config[7].empty())
  {
    dbData.pass = config[7].c_str();
  }
  else
  {
    cout << "Error. MySQL password not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Get MySQL port.
  if(!config[8].empty() && numOnly(config[8]))
  {
    dbData.port = stringToInteger(config[8].c_str());
  }
  else
  {
    cout << "Error. MySQL port not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Get number of processes to fork.
  int numProcs = 0;
  if(!config[9].empty() && numOnly(config[9]))
  {
    numProcs = stringToInteger(config[9].c_str());
  }
  else
  {
    cout << "Error. Number of processes to fork not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Get fallback email.
  bool mailFallbackError = 0;
  if(!config[10].empty())
  {
    mailData.fallbackReceiver = config[10].c_str();
  }
  else
  {
    mailFallbackError = 1;
    mailData.fallbackReceiver = "";
  }

  // Get fallback JID.
  bool xmppFallbackError = 0;
  if(!config[11].empty())
  {
    xmppData.fallbackReceiver = config[11].c_str();
  }
  else
  {
    xmppFallbackError = 1;
    xmppData.fallbackReceiver = "";
  }

  // Get fallback mobile number for Clickatell API.
  bool mobilecFallbackError = 0;
  if(!config[12].empty())
  {
    clickatellData.fallbackNumber = config[12].c_str();
  }
  else
  {
    mobilecFallbackError = 1;
    clickatellData.fallbackNumber = "";
  }

  if(!config[13].empty())
  {
    nodeID = stringToInteger(config[13].c_str());
  }
  else
  {
    cout << "Error. Node ID not set?" << endl;
    exit(EXIT_FAILURE);
  }

  // Finished parsing of config file.
  cout << "[ OK ]" << endl;

  // Perform a selftest.
  cout << "- Selftest sequence:\t";
  cout.flush();
  if(selfTest(port, loglevel) != "1")
  {
    // Selftest failed!
    cout << "Terminating: " << selfTest(port, loglevel) << endl;
    exit(EXIT_FAILURE);
  }
  else
  {
    // No errors.
    cout << "[ OK ]" << endl;
  }

  cout << "- TLS initialization:\t";
  cout.flush();

  // GnuTLS variables.
  static gnutls_dh_params_t dh_params;

  // GnuTLS initialization.
  gnutls_global_init();
  gnutls_global_set_log_function(logTLS);
  gnutls_global_set_log_level(0);
  gnutls_anon_allocate_server_credentials(&anoncred);
  gnutls_dh_params_init(&dh_params);
  gnutls_dh_params_generate2 (dh_params, DH_BITS);
  gnutls_anon_set_server_dh_params (anoncred, dh_params);

  cout << "[ OK ]" << endl;

  // Display warnings about possibly missing fallback receivers.
  if(mailFallbackError || xmppFallbackError || mobilecFallbackError)
    cout << "- Some notification methods have no fallback receivers." << endl;

  Log log(LOGFILE, dbData);

  // Start as daemon if we are not in debug mode.
  if(!debug)
  {
    if(daemon(0,0) < 0)
    {
      cout << "Terminated. Could not initialize daemon mode!" << endl;
      exit(EXIT_FAILURE);
    }
  }

  // We are a daemon from here on.

  pid_t pid = getpid();

  // Write the pidfile.
  ofstream pidwrite(PIDFILE, ios::out);
  if(!pidwrite.fail())
  {
    pidwrite << pid << endl;
  }
  else
  {
    log.putLog(2, "50", "Terminated. Could not create pidfile!");
    exit(EXIT_FAILURE);
  }
  pidwrite.close();

#ifndef DEBUG
  // Signal handling.  (could need improvements here!)
  signal(SIGABRT, cleanUp);
  signal(SIGFPE, cleanUp);
  signal(SIGILL, cleanUp);
  signal(SIGINT, cleanUp);
  signal(SIGSEGV, cleanUp);
  signal(SIGTERM, cleanUp);
#endif

  Database db(dbData);

  if(db.initConnection())
  {
    // Check if our node ID exists
    int noderes = Cloud::checkNodeID(nodeID, db);
    if(noderes < 0)
    {
      cout << "Could not check for Node ID! (" << nodeID<< ") Database error." << endl;
      exit(EXIT_FAILURE);
    }
    else if(noderes == 0)
    {
      cout << "Invalid Node ID! (" << nodeID<< ") Does not exist." << endl;
      exit(EXIT_FAILURE);
    }

    db.setQuery(db.getHandle(), Information::clearHealth(nodeID));

    // REMOVE FOR CLOUD
    db.setQuery(db.getHandle(), "UPDATE services SET handler = 0");

    // Update notification settings.
    mailData = Mail::fetchSettings(dbData);
    xmppData = XMPP::fetchSettings(dbData);
    clickatellData = Clickatell::fetchSettings(dbData);

    if((servSock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      // Socket could not be created.
      cout << "Could not create socket! Aborting." << endl;
      exit(EXIT_FAILURE);
    }
    else
    {
      // Socket created.
      const int y = 1;
      setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
      address.sin_family = AF_INET;
      // On which IPs to listen (INADDR_ANY -> any IP).
      address.sin_addr.s_addr = INADDR_ANY;
      // On which port to listen.
      address.sin_port = htons(port);
      if(bind (servSock,(struct sockaddr *) &address,sizeof (address)) != 0)
      {
        // Could not bind to socket.
        cout << "Could not bind to socket. Is the server already running? Aborting." << endl;
        exit(EXIT_FAILURE);
      }
      else
      {
        // Bound to socket! Listen.
        listen(servSock, 50);

        addrlen = sizeof(struct sockaddr_in);

        // Fork client handler processes.
        for(int threadCount = 0; threadCount < numProcs; threadCount++)
        {
          // Fork client handlers.
          pid_t processID;
          if((processID = fork()) < 0)
          {
            cout << "Could not fork client handlers." << endl;
            exit(EXIT_FAILURE);
          }
          else if(processID == 0)
          {
            // This is the child process / Forking worked.
            clientHandler = 1;
            handleClient();
          }
        }

        clientHandler = 0;

        // Are service checks enabled in config file?
        if(doServiceChecks)
        {
          // Start service check thread.
          pthread_t serviceThread;
          if(pthread_create(&serviceThread, 0, serviceChecks, NULL))
          {
            cout << "Terminating: Could not create service checks thread." << endl;
            exit(EXIT_FAILURE);
          }
        }

        // Start maintenance thread.
        pthread_t maintThread;
        if(pthread_create(&maintThread, 0, maintenanceThread, NULL))
        {
          cout << "Terminating: Could not create maintenance thread." << endl;
          exit(EXIT_FAILURE);
        }

        // Start thread for cloud status updating.
        pthread_t cloudStatusThread;
        if(pthread_create(&cloudStatusThread, 0, cloudStatusUpdater, NULL))
        {
          cout << "Terminating: Could not create cloud status updater." << endl;
          exit(EXIT_FAILURE);
        }

        // RE-ENABLE FOR CLOUD
        //        // Start thread for cloud service management.
        //        pthread_t cloudServiceManagerThread;
        //				if(pthread_create(&cloudServiceManagerThread, 0, cloudServiceManager, NULL)) {
        //					cout << "Terminating: Could not create cloud service management thread." << endl;
        //		      exit(EXIT_FAILURE);
        //				}

        if(!Cloud::setTakeoff(nodeID, db))
        {
          cout << "Terminating: Could not update startup timestamp." << endl;
          exit(EXIT_FAILURE);
        }

        // Keep the main thread running.
        //int fails = 0;
        while(1)
        {
          XMPP xmpp(xmppData, dbData);

          // Update health statistics.
          if(!db.setQuery(db.getHandle(), Information::updateHealth(nodeID, Health::getPID(),
            clientHandler,Health::getVMSize(), Health::getThreads(),
            packageCountOK, packageCountERR,
            Health::getDBSize(dbData, 1)/1024,
            Health::getDBSize(dbData, 2)/1024,
            Health::getDBSize(dbData, 3)/1024)))
          {
            log.putLog(1, "052", "Could not update health statistics.");
          }

          sleep(60);
        }

        close(servSock);
      }
    }
  }
  else
  {
    cout  << "Could not connect to database server. Aborting." << endl
      << "\tThe following error was reported: " << mysql_error(db.getHandle()) << endl;
    log.putLog(2, "051", "Could not connect to database server. Aborting.");
  }

  // Not reached.

  exit(EXIT_FAILURE);
}
