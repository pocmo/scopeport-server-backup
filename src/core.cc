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

#include "internal.h"

// The database class.
#include "database/Database.h"
// Authentication handling.
#include "conversation/Auth.h"
// Host blacklist.
#include "conversation/Blacklist.h"
// Services.
#include "services/Services.h"
// Warning conditions.
#include "notifications/Warning.h"
// Converting numbers to names etc.
#include "database/Information.h"
// This class is used for validating client data.
#include "conversation/ReadData.h"
// This class is used for talking to clients.
#include "conversation/Negotiation.h"
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


mySQLData dbData;
mailingData mailData;
XMPPData xmppData;
mobilecData clickatellData;

unsigned int nodeID = 0;

bool clientHandler = 0;

gnutls_anon_server_credentials_t anoncred;

inline int stringToInteger(string st){
	int result;
	if(stringstream(st) >> result){
		return result;
	}else{
		return 0;
	}
}

bool numOnly(string checkMe){
	bool result;
	if(checkMe.empty())
		return 0;
	int checkMe_length = checkMe.length();
		for(int i=0; i < checkMe_length; i++) {
  			if(isdigit(checkMe.at(i))){
				result = 1;
  			}else{
  				// Stop and return false if a non-digit char was detected.
  				return 0;
  				break;
  			}
  		}
	return result;
}

string selfTest(int port, int loglevel){
	// Check if port is numOnly.
	if(port <= 0)
		return "Port is in wrong format. Only numbers allowed.";

	// Check if logfile is writable.
	ofstream testlog(LOGFILE, ios::app);
	if(testlog.fail()){
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

string noSpaces(string str){
	int i;
	if(str.length() > 0){
		int strlen = str.length();
		stringstream newstr;
		for(i = 0; i < strlen; i++){
			if(!isspace(str.at(i))){
				newstr << str.at(i);
			}
		}
		return newstr.str();
	}
	return "err";
}

char* noNewLine(char* str, int size ){
    int i;
    for (i = 0; i < size; ++i){
        if(str[i] == '\n' ){
            str[i] = '\0';
            return str;
        }
    }
    return str;
}

string noNewLineS(string str){
	stringstream newStr;
	istringstream iss(str);
	string token;

	while(getline(iss, token, '\n')){
		newStr << token << " ";
	}

    return newStr.str();
}

unsigned long resolveName(char* server){
	if(strlen(server) <= 0)
		return 0;
	struct hostent *host;
	if((host = gethostbyname(server)) == NULL)
		return 0;
	return *((unsigned long*) host->h_addr_list[0]);
}

void* serviceHandler(void* arg){
	Log log(LOGFILE, dbData);
	Database db(dbData);

	// Calculate a kinda random ID for this handler.
  static timeval tv;
  static timeval tv2;
  static struct timezone tz;
  gettimeofday(&tv, &tz);
  gettimeofday(&tv2, &tz);
  unsigned int handlerID = tv.tv_usec * 3;

	Services service(dbData, handlerID);

	// Try to establish a database connection.
	if(db.initConnection()){
		// Fetch a service to handle.
    string query = "SELECT id FROM services WHERE handler = 0 AND disabled = 0 LIMIT 1";
		if(mysql_real_query(db.getHandle(), query.c_str(), strlen(query.c_str())) == 0){
			// Query successful.
			MYSQL_ROW serviceResult;
			MYSQL_RES* res = mysql_store_result(db.getHandle());
			serviceResult = mysql_fetch_row(res);
			if(mysql_num_rows(res) > 0){
        // We fetched a service to handle.
				if(serviceResult[0] == NULL){
					// We got NULL fields.
					mysql_free_result(res);
					mysql_close(db.getHandle());
					return arg;
				}
				
        // Tell the service object it's own ID.
        service.setServiceID(stringToInteger(serviceResult[0]));

        // Initially fill with settings. This will be repeated before every check.
        if(!service.updateSettings()){
          // Could not store the response time.
          log.putLog(2, "xxx", "Could not initially set service settings.");
          service.updateStatus(SERVICE_STATE_INTERR);
					mysql_free_result(res);
					mysql_close(db.getHandle());
					return arg;
        }
			}else{
				// No services have been fetched.
				mysql_free_result(res);
				mysql_close(db.getHandle());
				return arg;
			}
			mysql_free_result(res);
		}else{
			log.putLog(2, "2000", "Could not fetch service to handle.");
			mysql_close(db.getHandle());
			return arg;
		}

		// The service data is now available in MYSQL_ROW service.

		// Announce that this handler handles the service.
		stringstream announce;
		announce	<< "UPDATE services SET handler = "
					<< service.getHandlerID()
					<< " WHERE id = "
					<< service.getServiceID();

		if(!db.setQuery(db.getHandle(), announce.str())){
			log.putLog(2, "3000", "Could not update service handler.");
			mysql_close(db.getHandle());
			return arg;
		}

		mysql_close(db.getHandle());

	}else{
		// Could not establish a database connection. Close this thread.
		return arg;
	}

	// We got all the service data.

	// Run forever
	while(1){
		// Check if this service is still wanted to be checked for.
		if(db.initConnection()){
			stringstream checkService;
			checkService	<< "SELECT id FROM services WHERE id = "
							<< service.getServiceID()
							<< " AND disabled = 0";
			unsigned int serviceState = db.getNumOfResults(checkService.str());
			if(serviceState <= 0){
				stringstream message;
				message	<< "Closing service handler #" << service.getHandlerID()
						<< " as it is not needed anymore.";
				log.putLog(0, "NOTICE", message.str());
				mysql_close(db.getHandle());
				return arg;
			}
			mysql_close(db.getHandle());
		}else{
      log.putLog(2, "xxx", "Could not check if service still needs to be checked (Database error). Retry in 30 seconds.");
      sleep(30);
      continue;
		}

    // Update settings.
    if(!service.updateSettings()){
      // Could not store the response time.
      log.putLog(2, "xxx", "Could not update service settings. Retry in 30 seconds.");
      service.updateStatus(SERVICE_STATE_INTERR);
      sleep(30);
      continue;
    }

		// Check service twice and build an average response time.
    int serviceResult = SERVICE_STATE_INTERR;
    int firstServiceResult = SERVICE_STATE_INTERR;
    int secondServiceResult = SERVICE_STATE_INTERR;
    for(int run = 0; run <= 1; run++){
		  serviceResult = service.checkService(run);
      if(run == 0){
        firstServiceResult = serviceResult;
      }else{
        secondServiceResult = serviceResult;
      }
    }

    /*
     * Start the whole procedure again if the first and the second
     * serviceResult are not equal because this is most probably
     * a mismeasurement.
     */
    if(firstServiceResult != secondServiceResult){
      sleep(5);
      continue;
    }

    // Find out if the response time was too high.
    if(service.checkResponseTime() == 0){
      // Mark service with "Too high response time"
      serviceResult = SERVICE_STATE_OKAYTIME;
    }

    // Update the service status.
    service.updateStatus(serviceResult);

    // Store the response time in the database.
    if(!service.storeResponseTime()){
      // Could not store the response time.
      log.putLog(2, "008", "Could not update service response time. Retry in 30 seconds.");
      service.updateStatus(SERVICE_STATE_INTERR);
      sleep(30);
      continue;
    }

		if(serviceResult == SERVICE_STATE_CONFAIL || serviceResult == SERVICE_STATE_OKAYTIME
                    || serviceResult == SERVICE_STATE_TIMEOUT){
			/*
       * Check succeeded.
       *
			 * The service is down, timed out or has a too high response time.
			 * The method will find out if the response time was too high
			 * or if the service is just down/not reachable.
			 */
			service.sendWarning();
		}else if(serviceResult == SERVICE_STATE_INTERR){
			/*
			 * The service could not be checked because an internal error occured.
			 * Send a general warning!
			 */
			GeneralNotifications gn(dbData, mailData, xmppData, clickatellData);
			gn.sendMessages("Critical failure", "Could not check service because an "
					"internal error occured");
		}

		// Wait 60 seconds until next check.
		sleep(60);
	}

	return arg;
}

void* serviceChecks(void* arg){
	Log log(LOGFILE, dbData);
	Database db(dbData);
	// Run forever.
	while(1){
		// Try to establish a database connection.
		if(db.initConnection()){
			// Get the number of services that have no handler yet.
			unsigned int numOfServices = db.getNumOfResults("SELECT id FROM services WHERE handler = 0");
			mysql_close(db.getHandle());

			// Start a thread for every service that has no handler yet.
			for(unsigned int i = 0;i < numOfServices; i++){
				// Start thread.
				pthread_t thread;
				if(pthread_create(&thread, 0, serviceHandler, NULL)) {
					log.putLog(2, "1000", "Could not create service thread");
				}

				// Wait one second before starting the next thread.
				sleep(1);
			}
		}else{
			// Could not connect to database.
			log.putLog(2, "018", "Could not start service check module! (Database error) - "
										"Retry in 5 minutes.");
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

// This thread checks sensor 0 about every 60 seconds and warns
// if a host has stopped sending sensor data.
void* onlineStateChecks(void* arg){
	// Give the clients some time to send data.
	sleep(180);

	Database db(dbData);
	Log log(LOGFILE, dbData);

	while(1){
		// Try to connect to database.
		if(db.initConnection()){
			// We are connected.
			// Get every host.
			stringstream getHostsQuery;
			time_t rawtime;
			time(&rawtime);

			// Get the timestamp 5 minutes ago.
			int timeborder = rawtime-300;

			getHostsQuery	<< "SELECT hosts.hostid, hosts.name FROM hosts "
								"LEFT JOIN lastsensordata ON lastsensordata.host = hosts.hostid "
								"WHERE hosts.disabled = 0 "
								"AND lastsensordata.st = 0 "
								"AND lastsensordata.timestamp < " << timeborder;
			if(mysql_real_query(db.getHandle(), getHostsQuery.str().c_str(),
					strlen(getHostsQuery.str().c_str())) != 0){
				// Query was not successful.
				mysql_close(db.getHandle());
				sleep(60);
				continue;
			}else{
				// Query successful.
				MYSQL_RES* res = mysql_store_result(db.getHandle());
				MYSQL_ROW row;
				// Were rows fetched / Are there hosts?
				if(mysql_num_rows(res) > 0){
					// Yes. Go through every host that was fetched.
					while((row = mysql_fetch_row(res))){

						// Only hosts that stopped sending sensor data were fetched.

						// Build query for scheduled downtime check.
						stringstream checkDowntimeQuery;
						checkDowntimeQuery	<< "SELECT ID FROM downtimes"
												" WHERE hostid = " << row[0] <<
											   " AND type = 1 AND `from` < " << rawtime <<
											   " AND `to` > " << rawtime;

						if(mysql_real_query(db.getHandle(), checkDowntimeQuery.str().c_str(),
								strlen(checkDowntimeQuery.str().c_str())) != 0){
							// Query was not successful.
							mysql_close(db.getHandle());
							continue;
						}else{

							// Query successful.

							// Check if a downtime was scheduled for this host.
							MYSQL_RES* sdres = mysql_store_result(db.getHandle());
							if(mysql_num_rows(sdres) > 0){
								// A downtime was scheduled. Don't warn. Skip.
								mysql_free_result(sdres);
								mysql_close(db.getHandle());
								continue;
							}
							mysql_free_result(sdres);

							// Update lastsensordata entry of this host/sensor.
							stringstream updateQuery;
							updateQuery << "UPDATE lastsensordata "
											"SET sv = 0 "
											"WHERE host = " << row[0] <<
											" AND st = 0";

							if(mysql_real_query(db.getHandle(), updateQuery.str().c_str(),
									strlen(updateQuery.str().c_str())) != 0){
								// Query was not successful.
								mysql_close(db.getHandle());
								continue;
							}

							// Send warning for this host.

							Warning warning(dbData);

							if(!warning.getConditions(row[0], "0")){
								if(!warning.getError().empty())
									log.putLog(2, "019", "Could not update conditions "
										"in online state module!");
								mysql_close(db.getHandle());
								continue;
							}

							string thisLastWarn = db.sGetQuery(Information::getLastWarn(row[0],"0"));
							string thisHostName = db.sGetQuery(Information::getHostName(row[0]));

							// Check if sensor value is in range.
							if(warning.checkSensor(row[0], "0", row[1], thisLastWarn) == 0){
								// Sensor is out of range! Warn.

								stringstream warningSubj;
								stringstream warningMsg;

								warningSubj	<< "Warning! Host \""
											<< thisHostName
											<< "\" is not sending sensor data!";

								warningMsg	<< "Warning! Host \""
											<< thisHostName
											<< "\" ist not sending sensor data!";

								time_t warntime;
								time(&warntime);

								db.setQuery(db.getHandle(), Information::setLastWarn(warntime, row[0], "0"));

								// Get mail receivers and send warnings.
								if(mailData.doMailing){
									Mail mailing(mailData);
									vector<string> mailRecvList;
									mailRecvList = Information::getMailWarningReceivers(db.getHandle(),
																db.sGetQuery(Information::getReceiverGroup(row[0], "0")),
																db.sGetQuery(Information::getSensorSeverity(row[0], "0")));
									// Send a warning mail to every mail receiver.
									int mailRecvCount = 0;
									int mailRecvListSize = mailRecvList.size();
									while(mailRecvCount < mailRecvListSize){
										if(!mailRecvList[mailRecvCount].empty())
											mailing.sendMail(mailRecvList[mailRecvCount], warningSubj.str(), warningMsg.str());
										mailRecvCount++;
									}
								}

								// Get XMPP receivers and send warnings.
								if(xmppData.doXMPP){
									XMPP xmpp(xmppData, dbData);
									vector<string> xmppRecvList;
									xmppRecvList = Information::getXMPPWarningReceivers(db.getHandle(),
														db.sGetQuery(Information::getReceiverGroup(row[0],"0")),
														db.sGetQuery(Information::getSensorSeverity(row[0], "0")));

									// Send a warning message to every XMPP receiver.
									int xmppRecvCount = 0;
									int xmppRecvListSize = xmppRecvList.size();
									while(xmppRecvCount < xmppRecvListSize){
										if(!xmppRecvList[xmppRecvCount].empty())
												xmpp.sendMessage(warningMsg.str(), xmppRecvList[xmppRecvCount]);
										xmppRecvCount++;
									}
								}

								// Get Clickatell Mobile API receivers and send warnings.
								if(clickatellData.doMobileC && mailData.doMailing){

									vector<string> mobilecRecvList = Information::getMobileCWarningReceivers(db.getHandle(),
											db.sGetQuery(Information::getReceiverGroup(row[0],"0")),
											db.sGetQuery(Information::getSensorSeverity(row[0], "0")));

									Mail mailing(mailData);
									int mobilecRecvCount = 0;
									int mobilecRecvListSize = mobilecRecvList.size();
									while(mobilecRecvCount < mobilecRecvListSize){

										if(!mobilecRecvList[mobilecRecvCount].empty()){

											// Build the message that fits to the API.
											stringstream newWarningMsg;
											newWarningMsg	<< "user:" << clickatellData.username << endl
															<< "password:" << clickatellData.password << endl
															<< "api_id:" << clickatellData.apiID << endl
															<< "to:" << mobilecRecvList[mobilecRecvCount] << endl
															<< "text: [ScopePort] " << warningMsg.str();
											mailing.sendMail(CLICKATELLMAIL, warningSubj.str(), newWarningMsg.str());
										}
										mobilecRecvCount++;
									}
								}
							}
						}
					}
				}
				mysql_free_result(res);
				sleep(60);
			}

			mysql_close(db.getHandle());

		}else{
			log.putLog(2, "020", "Could not start online state module! (Database error) - "
						"Retry in 5 minutes.");

			GeneralNotifications gn(dbData, mailData, xmppData, clickatellData);
			gn.sendMessages("Critical failure", "Could not start online state module! "
					"(Database error) Retry in 5 minutes.");

			sleep(300);
		}
	}

	return arg;
}

int servSock;
int clientSock;
struct sockaddr_in address;
socklen_t addrlen;

int loglevel = 0;
bool blacklisting = 0;

// This counts the received packages.
long double packageCountOK = 0;
long double packageCountERR = 0;

void* cloudStatusUpdater(void* args){
	Log log(LOGFILE, dbData);

  Database db(dbData);
  Cloud cloud(nodeID, dbData);

  while(1){
    if(db.initConnection()){
    	while(cloud.updateOwnStatus(db)){
        sleep(10);
      }
      mysql_close(db.getHandle());
    }else{
      // Could not connect to database.
			log.putLog(2, "xxx", "Could not update own cloud status: Could not connect to database. Retry in one minute.");
    }
    sleep(60);
  }

  return (NULL);
}

void* maintenanceThread(void* args){

	Log log(LOGFILE, dbData);

	while(1){

		Database maintDB(dbData);

		if(maintDB.initConnection()){
			// Clear outdated sensor data.
			if(!maintDB.clearSensorData()){
				log.putLog(1, "021", "Could not delete old sensor data.");
			}

			// Clear outdated service data.
			if(!maintDB.clearServiceData()){
				log.putLog(1, "021", "Could not delete old service data.");
			}

			mailData = Mail::fetchSettings(dbData);
			xmppData = XMPP::fetchSettings(dbData);
			clickatellData = Clickatell::fetchSettings(dbData);

			// Update process statistics.
			if(!maintDB.setQuery(maintDB.getHandle(), Information::updateHealth(Health::getPID(),
					clientHandler,Health::getVMSize(), Health::getThreads(), packageCountOK,
					packageCountERR, -1, -1, -1)))
				log.putLog(1, "053", "Could not update health statistics.");

			mysql_close(maintDB.getHandle());
		}

		// Run every minute.
		sleep(60);
	}

	// Not reached.

	return (NULL);
}

void* messageMonkey(void* args) {

	Log log(LOGFILE, dbData);

	while(1){
		// Emergency notifications.
		EmergencyNotifications en(dbData, mailData, xmppData, clickatellData);

		int res = en.fetchTasks();

		// See if there are emergency notifications to send.
		if(res == 1){
			// We need to send emergency notifications.
			int i = 0;
			int listSize = en.getListSize();
			while(i < listSize){
				res = en.sendMessage(i);
				if(res == 0){
					log.putLog(2, "023", "Error while semding emergency notification - "
									"Retry scheduled.");
					// There was an error. But we will try again next run.
					if(!en.markReceiver(i, 0))
						log.putLog(2, "024", "Could not update status of emergency receiver");
				}else if(res < 0){
					log.putLog(2, "025", "Error while semding emergency notification. - "
									"No retry scheduled.");
					// There was an critical error. We will not try again.
					if(!en.markReceiver(i, -1))
						log.putLog(2, "026", "Could not update status of emergency receiver");
				}else if(res == 1){
					// Everything went fine.
					if(!en.markReceiver(i, 1))
						log.putLog(2, "027", "Could not update status of emergency receiver");
				}
				i++;
			}
		}else if(res < 0){
			// Error.
			log.putLog(2, "028", "Could not send emergency notifications. Retry in one minute.");
		}

		// Run every minute.
		sleep(60);
	}

	// Not reached.

	return (NULL);
}

void killClient(int sig){
	Log log(LOGFILE, dbData);
	log.putLog(0, "029", "Closed connection to client that did not complete"
			" transaction after 5 seconds.");
	close(clientSock);
}

void handleClient(){

	// Start maintenance thread.
	pthread_t maintThread;
	if(pthread_create(&maintThread, 0, maintenanceThread, NULL)) {
		cout << "Terminating: Could not create maintenance thread." << endl;
		exit(-1);
	}

	Log log(LOGFILE, dbData);

	// For closing connection to a client after 5sec.
	signal(SIGALRM, killClient);

	// Run forever.
	while(1){

		// Accept connections with new socket "clientSock".
		clientSock = accept(servSock,(struct sockaddr *) &address, &addrlen);

		if(clientSock <= 0){
			log.putLog(1, "030", "Could not accept connection from client.");
			close(clientSock);
			packageCountERR++;
			continue;
		}

		// Create database object.
		Database clientDB(dbData);


		// Try to open connection to database.
		if(clientDB.initConnection()){
			// We are connected to the database.

			// Start the timer. The client now has five seconds to complete transaction. Go.
			alarm(5);

			// Create negotiation object that allows communicating with the client.
			Negotiation talk(clientSock, address);
			// Check if this host is blacklisted.
			Blacklist blacklist(dbData);
			// Check if blacklisting is enabled.
			if(blacklisting){
				// Blacklisting is enabled. Check if this host is blacklisted.
				if(!blacklist.checkHost(talk.getCurrentClientIP())){
					// Host is blacklisted - Notify.
					packageCountERR++;
					stringstream logMessage;
					logMessage	<< "Received package from Blacklisted host \""
								<< talk.getCurrentClientIP()
								<< "\". Skipping.";
					log.putLog(0, "031", logMessage.str());
					// Reset alarm timer.
					alarm(0);
					// Close database connection.
					mysql_close(clientDB.getHandle());
					// Close socket.
					close(clientSock);
					// Get out of this while cycle to accept the next client.
					continue;
				}
				// Host is not blacklisted. Go on.
			}

			// Create auth object. This keeps the passwords of configured clients.
			Auth auth(dbData);
			// Refresh authentication table.
			if(!auth.loadHosts()){
				// Could not refreseh authentication table.
				log.putLog(2, "032", "Could not refresh authentication table.");
			}
			// Handshake.
			if(!talk.performHandshake()){
				// Handshake failed.
				packageCountERR++;
				// Build log message.
				stringstream handshakeFailMessage;
				handshakeFailMessage	<< "Handshake with client "
										<< talk.getCurrentClientIP()
										<< " failed";
				log.putLog(1, "033", handshakeFailMessage.str());
				// Reset alarm timer.
				alarm(0);
				// Close database connection.
				mysql_close(clientDB.getHandle());
				// Close socket.
				close(clientSock);
				// Get out of this while cycle to accept the next client.
				continue;
			}

			// The handshake has completed successfully.
			// Wait for STARTTLS if GnuTLS was requested.
			if(talk.getTLSUsage()){
				string starttls = talk.recvMessage();
				if(starttls != "STARTTLS"){
					packageCountERR++;
					stringstream handshakeFailMessage;
					handshakeFailMessage	<< "Handshake with client "
											<< talk.getCurrentClientIP()
											<< " failed";
					log.putLog(1, "034", handshakeFailMessage.str());
					// Reset alarm timer.
					alarm(0);
					// Close database connection.
					mysql_close(clientDB.getHandle());
					// Close socket.
					close(clientSock);
					// Get out of this while cycle to accept the next client.
					continue;
				}
			}

			char buffer[TALKBUFSIZE] = "";

			gnutls_session_t session;
			if(talk.getTLSUsage()){
				// Client has chosen to use GNUTLS.
				// Init TLS session.
				const int kx_prio[] = { GNUTLS_KX_ANON_DH, 0 };
				gnutls_init(&session, GNUTLS_SERVER);
				// Use default priorities.
				gnutls_set_default_priority(session);
				gnutls_kx_set_priority(session, kx_prio);
				gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);
				gnutls_dh_set_prime_bits(session, DH_BITS);
				gnutls_transport_set_ptr(session, (gnutls_transport_ptr_t) clientSock);

				// Do TLS handshake.
				int ret = gnutls_handshake(session);
				// Did the handshake succeed?
				if(ret < 0){
					// No.
					packageCountERR++;
					stringstream shakeError;
					shakeError	<< "TLS handshake with host "
								<< talk.getCurrentClientIP()
								<< " failed.";
					log.putLog(2, "035", shakeError.str());
					alarm(0);
					mysql_close(clientDB.getHandle());
					gnutls_bye(session, GNUTLS_SHUT_WR);
				    gnutls_deinit(session);
					close(clientSock);
					continue;
				}

				// TLS handshake completed.
				// Receive data.
				ret = gnutls_record_recv(session, buffer, TALKBUFSIZE-1);
				// Is the reveived data valid?
				if(ret == 0){
					// Peer has closed the connection.
					packageCountERR++;
					alarm(0);
					gnutls_deinit(session);
					mysql_close(clientDB.getHandle());
					close(clientSock);
					continue;
				}else if(ret < 0){
					packageCountERR++;
					stringstream recvError;
					recvError	<< "Received invalid TLS package from "
								<< talk.getCurrentClientIP();
					log.putLog(1, "036", recvError.str());
					alarm(0);
					mysql_close(clientDB.getHandle());
					gnutls_bye(session, GNUTLS_SHUT_WR);
				    gnutls_deinit(session);
					close(clientSock);
					continue;
				}
			}else{
				// Client does not want TLS encryption.
				ssize_t len;
				len = read(clientSock, buffer, TALKBUFSIZE-1);

				// Was something received?
				if(len <= 0){
					packageCountERR++;
					stringstream recvError;
					recvError	<< "Could not receive unencrypted sensor data from "
								<< talk.getCurrentClientIP();
					log.putLog(1, "037", recvError.str());
					alarm(0);
					mysql_close(clientDB.getHandle());
					close(clientSock);
					continue;
				}
				if(len < TALKBUFSIZE){
					buffer[len] = '\0';
				}else{
					packageCountERR++;
					stringstream recvError;
					recvError	<< "Could not receive unencrypted sensor data from "
								<< talk.getCurrentClientIP();
					log.putLog(1, "038", recvError.str());
					alarm(0);
					mysql_close(clientDB.getHandle());
					close(clientSock);
					continue;
				}
			}
			stringstream message;
			message	<< buffer;

			// We now have the sensordata in string message.

			// Stop timeout.
			alarm(0);

			// Create object that holds the sensordata.
			ReadData datastream;
			// Check for validity.
			if(datastream.inspectStream(message.str())){
				// Package is valid.
				// Check if client submitted the correct password.
				if(datastream.getSt() != "00" && !auth.checkHost(datastream.getHost(),
						datastream.getPass())){
					// Wrong password.
					packageCountERR++;
					stringstream wrongPasswordMsg;
					wrongPasswordMsg	<< "Host "
										<< talk.getCurrentClientIP()
										<< " sent wrong password. Skipping.";
					log.putLog(2, "039", wrongPasswordMsg.str());
					if(blacklisting){
						// Blacklist host that sent wrong password.
						stringstream blacklistMsg;
						blacklistMsg	<< "Adding host \""
										<< talk.getCurrentClientIP()
										<< "\" to blacklist!";
						log.putLog(2, "040", blacklistMsg.str());
						if(!blacklist.blackHost(talk.getCurrentClientIP()))
							log.putLog(2, "041", "Could not insert host that sent wrong password into blacklist.");
					}
					// Close connection and accept new package in next while() cycle.
					mysql_close(clientDB.getHandle());
					if(talk.getTLSUsage()){
						gnutls_bye(session, GNUTLS_SHUT_WR);
					    gnutls_deinit(session);
					}
					close(clientSock);
					continue;
				}
				// Password is okay.

				// Reset the alarm timer of st 0.
				stringstream resetZeroTimer;
				resetZeroTimer << "UPDATE sensor_conditions SET "
									"lastwarn = 0 WHERE st = 0 "
									"AND hostid = " << datastream.getHost();

				if(mysql_real_query(clientDB.getHandle(), resetZeroTimer.str().c_str(),
						strlen(resetZeroTimer.str().c_str())) != 0){
					// Query was not successful.
					log.putLog(1, "042", "Could not reset alarm timer.");
				}

				// Update IP of this host.
				stringstream queryIP4;
				queryIP4	<< "UPDATE hosts SET ip4addr = '"
							<< talk.getCurrentClientIP()
							<< "' WHERE hostid = "
							<< datastream.getHost();

				if(!clientDB.setQuery(clientDB.getHandle(), queryIP4.str()))
					log.putLog(1, "043", "Could not update IP of host in database.");

				// Get information about alarms.
				string thisLastWarn = clientDB.sGetQuery(Information::getLastWarn(
													datastream.getHost(),
													datastream.getSt()));


				// Create warning object. This keeps information of
				// e.g. severity levels of sensors.
				Warning warning(dbData);
				// Refresh warning conditions table.
				if(!warning.getConditions(datastream.getHost(), datastream.getSt())){
					// Could not refresh warning table.
					log.putLog(2, "044", "Could not refresh warning conditions table.");
				}


				// Check if sensor value is in range.
				if(datastream.getSt() != "0" && warning.checkSensor(datastream.getHost(),
						datastream.getSt(), datastream.getSv(), thisLastWarn) == 0){
					// Sensor not in range! Warn.

					string thisSensorName = clientDB.sGetQuery(Information::getSensorName(datastream.getSt()));
					string thisHostName = clientDB.sGetQuery(Information::getHostName(datastream.getHost()));
					string thisSensorCondition = clientDB.sGetQuery2(Information::getSensorCondition(
																datastream.getHost(),
																datastream.getSt()));

					stringstream warningSubj;
					stringstream warningMsg;
					bool msgError = 0;

					if(thisSensorName != "NULL" && thisHostName != "NULL"){
						warningSubj	<< "Warning! Sensor \""
									<< thisSensorName
									<< "\" on host \""
									<< thisHostName
									<< "\"";
					}else{
						warningSubj	<< "Warning! an unknown Sensor "
									<< "has a bad value.";
						msgError = 1;
					}

					if(!msgError && thisSensorName != "NULL" && thisHostName != "NULL"
							&& thisSensorCondition != "NULL"){
						// Check if the client could not collect this sensor.
						if(datastream.getSv() != "-0-"){
							// The sensor value has been collected correctly.
							warningMsg	<< "Warning! Sensor \""
										<< thisSensorName
										<< "\" on host \""
										<< thisHostName
										<< "\" has value "
										<< datastream.getSv()
										<< endl << endl
										<< "Condition to be marked as \"good\": \""
										<< thisSensorCondition
										<< "\"";
						}else{
							// The sensor could not be collected.
							warningMsg	<< "Warning! Sensor \""
										<< thisSensorName
										<< "\" on host \""
										<< thisHostName
										<< "\" could not be collected"
										<< endl
										<< endl
										<< "You can disable the collection of "
												"this sensor in the configuration "
												"of your client"
										<< endl;
						}
					}else{
							warningMsg	<< "Warning! An unknown sensor "
										<< "has a bad value. I cannot "
										<< "give more information because "
										<< "of an error that occured while "
										<< "fetching sensor information."
										<< endl
										<< endl
										<< "You should manually check your monitored "
										<< "hosts in the ScopePort interface.";
					}

					time_t warntime;
					time(&warntime);

					clientDB.setQuery(clientDB.getHandle(), Information::setLastWarn(warntime, datastream.getHost(),
									datastream.getSt()));

					// Get mail receivers and send warnings.
					if(mailData.doMailing){
						// Create mailing object.
						Mail mailing(mailData);

						// Check for mailing parameters.
						if(mailData.doMailing && !mailData.mailServer.empty() > 0 && mailData.mailPort > 0
								&& !mailData.mailHostname.empty() && !mailData.mailFrom.empty()){
							// All mailing parameters set correctly.
							mailData.doMailing = 1;
						}else{
							// Parameters missing - Disable mailing.
							mailData.doMailing = 0;
						}

						vector<string> mailRecvList;
						mailRecvList = Information::getMailWarningReceivers(clientDB.getHandle(),
													clientDB.sGetQuery(Information::getReceiverGroup(
															datastream.getHost(),
															datastream.getSt())),
													clientDB.sGetQuery(Information::getSensorSeverity(
															datastream.getHost(),
															datastream.getSt())));
						// Send a warning mail to every mail receiver.
						int mailRecvCount = 0;
						int mailRecvListSize = mailRecvList.size();
						while(mailRecvCount < mailRecvListSize){
							if(!mailRecvList[mailRecvCount].empty())
								mailing.sendMail(mailRecvList[mailRecvCount], warningSubj.str(), warningMsg.str());
							mailRecvCount++;
						}
					}

					// Get XMPP receivers and send warnings.
					if(xmppData.doXMPP){
						// Create XMPP object.
						XMPP xmpp(xmppData, dbData);

						// Check for XMPP parameters.
						if(xmppData.doXMPP && !xmppData.xmppServer.empty() > 0 && xmppData.xmppPort > 0
								&& !xmppData.xmppUser.empty() && !xmppData.xmppPass.empty()
								&& !xmppData.xmppResource.empty()){
							// All XMPP parameters set correctly.
							xmppData.doXMPP = 1;
						}else{
							// Parameters missing - Disable XMPP.
							xmppData.doXMPP = 0;
						}

						vector<string> xmppRecvList;
						xmppRecvList = Information::getXMPPWarningReceivers(clientDB.getHandle(),
														clientDB.sGetQuery(Information::getReceiverGroup(
																datastream.getHost(),
																datastream.getSt())),
														clientDB.sGetQuery(Information::getSensorSeverity(
																datastream.getHost(),
																datastream.getSt())));

			 			// Send a warning message to every XMPP receiver.
						int xmppRecvCount = 0;
						int xmppRecvListSize = xmppRecvList.size();
						while(xmppRecvCount < xmppRecvListSize){
							if(!xmppRecvList[xmppRecvCount].empty())
									xmpp.sendMessage(warningMsg.str(), xmppRecvList[xmppRecvCount]);
							xmppRecvCount++;
						}
					}

					// Get Clickatell Mobile API receivers and send warnings.
					if(clickatellData.doMobileC && mailData.doMailing){

						vector<string> mobilecRecvList;
						mobilecRecvList = Information::getMobileCWarningReceivers(clientDB.getHandle(),
														clientDB.sGetQuery(Information::getReceiverGroup(
																datastream.getHost(),
																datastream.getSt())),
														clientDB.sGetQuery(Information::getSensorSeverity(
																datastream.getHost(),
																datastream.getSt())));

						Mail mailing(mailData);
						int mobilecRecvCount = 0;
						int mobilecRecvListSize = mobilecRecvList.size();
						while(mobilecRecvCount < mobilecRecvListSize){

							if(!mobilecRecvList[mobilecRecvCount].empty()){

								// Build the message that fits to the API.
								stringstream newWarningMsg;
								newWarningMsg	<< "user:" << clickatellData.username << endl
												<< "password:" << clickatellData.password << endl
												<< "api_id:" << clickatellData.apiID << endl
												<< "to:" << mobilecRecvList[mobilecRecvCount] << endl
												<< "text: [ScopePort] " << warningMsg.str();
								mailing.sendMail(CLICKATELLMAIL, warningSubj.str(), newWarningMsg.str());
							}
							mobilecRecvCount++;
						}
					}
				}

				// We did not need to warn.

				// Store current stream in database.
				if(!clientDB.saveStream(datastream.getTimestamp(),datastream.getHost(),
						datastream.getSt(),datastream.getSv())){
					stringstream dberror;
					dberror << "Could not store sensor data in database. ("
							<< clientDB.getError()
							<< " / HostID: "
							<< datastream.getHost()
							<< ")";
					log.putLog(2, "045", dberror.str());
				}
			}else{
				// Package was not valid.
				packageCountERR++;
				stringstream notgoodmsg;
				notgoodmsg	<< "Received package from "
							<< talk.getCurrentClientIP()
							<< " which was not valid!";
				log.putLog(2, "046", notgoodmsg.str());
				// Blacklist host that sent invalid package.
				if(blacklisting){
					stringstream blacklistMsg;
					blacklistMsg	<< "Adding host \""
									<< talk.getCurrentClientIP()
									<< "\" to blacklist!";
					log.putLog(2, "047", blacklistMsg.str());
					if(datastream.getSt() != "00" && !blacklist.blackHost(talk.getCurrentClientIP()))
						log.putLog(2, "048", "Could not insert host that sent invalid package into "
								"blacklist.");
				}
			}

			packageCountOK++;

			// Close connections and accept new package in next while() cycle.
			mysql_close(clientDB.getHandle());
			if(talk.getTLSUsage()){
				gnutls_bye(session, GNUTLS_SHUT_WR);
			    gnutls_deinit(session);
			}
		}else{
			// Could not connect to database.
			log.putLog(2, "049", "Client handler: Could not connect to database! "
					"Sensor package skipped.");
		}
		close(clientSock);
	}

	// Not reached.

	exit(-1);
}

void cleanUp(int sig){
	remove(PIDFILE);
	gnutls_anon_free_server_credentials (anoncred);
	gnutls_global_deinit();
	exit(-1);
}

void logTLS(int level, const char *message){
	Log log(LOGFILE, dbData);
	stringstream logmsg;
	logmsg << message;
	log.putLog(1, "TLS", message);
}

int main(int argc, char *argv[]){

	// So jung kommen wir nicht mehr zusammen.

	// Check if we are root.
	if(geteuid() != 0){
		cout <<  "The server needs to be started as root." << endl;
		return 0;
	}

  bool debug = 0;
  
  // Find out if we are in debug mode.
  for(int i = 0; i < argc; i++){
		if(strcmp(argv[i], "--debug") == 0){
			debug = 1;
		}
	}

	// Check if config file is readable
	ifstream configtest(CONFIGFILE);
	if(configtest.fail()){
	 cout << "Terminating: Could not read config file. Check permissions and if file exists." << endl;
	 return 0;
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

	if(!configstream.fail()){
		int i = 0;
		int j = 0;

		string configline;

		while(getline(configstream,configline)){
			i = 0;
			string token;
			istringstream iss(configline);
			while(j < parsize){
				if(configline.find(parameters[j], 0 ) != string::npos){
					while(getline(iss, token, '=')){
						// Read parameters.
						if(i == 1 && !token.empty()){
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
	if(!config[0].empty()){
		port = stringToInteger(config[0].c_str());
	}else{
		cout << "Error. Port not set?" << endl;
		return 0;
	}

	if(!config[1].empty()){
		loglevel = stringToInteger(config[1].c_str());
	}else{
		cout << "Error. Loglevel not set?" << endl;
		return 0;
	}

	// Find out if we want to enable rude checks.
	bool doRudes = 0;
	if(!config[2].empty()){
		if(config[2] == "1")
			doRudes = 1;
	}else{
		cout << "Error. Rude check mode not set?" << endl;
		return 0;
	}

	// Find out if we want to enable the blacklist.
	if(!config[3].empty()){
		if(config[3] == "1")
			blacklisting = 1;
	}else{
		cout << "Error. Blacklisting mode not set?" << endl;
		return 0;
	}

	// Get MySQL host.
	if(!config[4].empty()){
		dbData.host = config[4].c_str();
	}else{
		cout << "Error. MySQL host not set?" << endl;
		return 0;
	}

	// Get MySQL database.
	if(!config[5].empty()){
		dbData.db = config[5].c_str();
	}else{
		cout << "Error. MySQL database not set?" << endl;
		return 0;
	}

	// Get MySQL user.
	if(!config[6].empty()){
		dbData.user = config[6].c_str();
	}else{
		cout << "Error. MySQL user not set?" << endl;
		return 0;
	}

	// Get MySQL password.
	if(!config[7].empty()){
		dbData.pass = config[7].c_str();
	}else{
		cout << "Error. MySQL password not set?" << endl;
		return 0;
	}

	// Get MySQL port.
	if(!config[8].empty() && numOnly(config[8])){
		dbData.port = stringToInteger(config[8].c_str());
	}else{
		cout << "Error. MySQL port not set?" << endl;
		return 0;
	}

	// Get number of processes to fork.
	int numProcs = 0;
	if(!config[9].empty() && numOnly(config[9])){
		numProcs = stringToInteger(config[9].c_str());
	}else{
		cout << "Error. Number of processes to fork not set?" << endl;
		return 0;
	}

	// Get fallback email.
	bool mailFallbackError = 0;
	if(!config[10].empty()){
		mailData.fallbackReceiver = config[10].c_str();
	}else{
		mailFallbackError = 1;
		mailData.fallbackReceiver = "";
	}

	// Get fallback JID.
	bool xmppFallbackError = 0;
	if(!config[11].empty()){
		xmppData.fallbackReceiver = config[11].c_str();
	}else{
		xmppFallbackError = 1;
		xmppData.fallbackReceiver = "";
	}

	// Get fallback mobile number for Clickatell API.
	bool mobilecFallbackError = 0;
	if(!config[12].empty()){
		clickatellData.fallbackNumber = config[12].c_str();
	}else{
		mobilecFallbackError = 1;
		clickatellData.fallbackNumber = "";
	}
	
	if(!config[13].empty()){
		nodeID = stringToInteger(config[13].c_str());
	}else{
		cout << "Error. Node ID not set?" << endl;
		return 0;
	}

	// Finished parsing of config file.
	cout << "[ OK ]" << endl;

	// Perform a selftest.
	cout << "- Selftest sequence:\t";
	cout.flush();
	if(selfTest(port, loglevel) != "1"){
		// Selftest failed!
		cout << "Terminating: " << selfTest(port, loglevel) << endl;
		return 0;
	}else{
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
  if(!debug){
  	if(daemon(0,0) < 0){
  		cout << "Terminated. Could not initialize daemon mode!" << endl;
  		return 0;
  	}
  }

	// We are a daemon from here on.

	pid_t pid = getpid();

	// Write the pidfile.
	ofstream pidwrite(PIDFILE, ios::out);
	if(!pidwrite.fail()){
		pidwrite << pid << endl;
	}else{
		log.putLog(2, "50", "Terminated. Could not create pidfile!");
		return 0;
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

	if(db.initConnection()){
		// Check database table structure.
		if(!db.checkTables()){
			cout << "Broken database table structure! Aborting." << endl;
			cout << "\t Error in table: " << db.getMissingTable() << endl;
			cout << "\t " << db.getError() << endl;
			return 0;
		}

    // Check if our node ID exists
    int noderes = Cloud::checkNodeID(nodeID, db);
    if(noderes < 0){
      cout << "Could not check for Node ID! (" << nodeID<< ") Database error." << endl;
      return 0;
    }else if(noderes == 0){
      cout << "Invalid Node ID! (" << nodeID<< ") Does not exist." << endl;
      return 0;
    }

		db.setQuery(db.getHandle(), Information::clearHealth());

		// Update notification settings.
		mailData = Mail::fetchSettings(dbData);
		xmppData = XMPP::fetchSettings(dbData);
		clickatellData = Clickatell::fetchSettings(dbData);

		// Reset the client handlers.
		if(!db.setQuery(db.getHandle(), "UPDATE services SET handler = 0")){
			// Resetting the handlers failed.
			cout	<< "Could not reset service handlers. Terminating. "
					<< "(MySQL: " << db.getError() << ")"
					<< endl;
			exit(-1);
		}

		if((servSock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
			// Socket could not be created.
			cout << "Could not create socket! Aborting." << endl;
			return 0;
		}else{
			// Socket created.
			const int y = 1;
			setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
			address.sin_family = AF_INET;
			// On which IPs to listen (INADDR_ANY -> any IP).
			address.sin_addr.s_addr = INADDR_ANY;
			// On which port to listen.
			address.sin_port = htons(port);
			if(bind (servSock,(struct sockaddr *) &address,sizeof (address)) != 0) {
				// Could not bind to socket.
				cout << "Could not bind to socket. Is the server already running? Aborting." << endl;
				return 0;
			}else{
				// Bound to socket! Listen.
				listen(servSock, 50);

				addrlen = sizeof(struct sockaddr_in);

				for(int threadCount = 0; threadCount < numProcs; threadCount++){
					// Fork client handlers.
					pid_t processID;
					if((processID = fork()) < 0){
						cout << "Could not fork client handlers." << endl;
						exit(-1);
					}else if(processID == 0){
						// This is the child process / Forking worked.
						clientHandler = 1;
						handleClient();
					}
				}

				clientHandler = 0;

				// Are rude checks enabled in config file?
				if(doRudes){
					// Start rude check thread.
					pthread_t serviceThread;
					if(pthread_create(&serviceThread, 0, serviceChecks, NULL)) {
						cout << "Terminating: Could not create service checks thread." << endl;
						return 0;
					}
				}

				// Start thread that checks if monitored hosts are still sending data.
				pthread_t onlineStateThread;
				if(pthread_create(&onlineStateThread, 0, onlineStateChecks, NULL)) {
					cout << "Terminating: Could not create online state check thread." << endl;
					return 0;
				}

				// Start maintenance thread.
				pthread_t maintThread;
				if(pthread_create(&maintThread, 0, maintenanceThread, NULL)) {
					cout << "Terminating: Could not create maintenance thread." << endl;
					return 0;
				}

				// Start thread that sends asynchronous messages.
				pthread_t messageThread;
				if(pthread_create(&messageThread, 0, messageMonkey, NULL)) {
					cout << "Terminating: Could not create message thread." << endl;
					return 0;
				}

        // Start thread for cloud communication
        pthread_t cloudStatusThread;
				if(pthread_create(&cloudStatusThread, 0, cloudStatusUpdater, NULL)) {
					cout << "Terminating: Could not create Cloud status updater." << endl;
					return 0;
				}

        if(!Cloud::setTakeoff(nodeID, db)){
          cout << "Terminating: Could not update startup timestamp." << endl;
          return 0;
        }

				// Keep the main thread running.
				//int fails = 0;
				while(1){
					XMPP xmpp(xmppData, dbData);

					// Update health statistics.
					if(!db.setQuery(db.getHandle(), Information::updateHealth(Health::getPID(),
							clientHandler,Health::getVMSize(), Health::getThreads(),
							packageCountOK, packageCountERR,
							Health::getDBSize(dbData, 1)/1024,
							Health::getDBSize(dbData, 2)/1024,
							Health::getDBSize(dbData, 3)/1024)))
						log.putLog(1, "052", "Could not update health statistics.");

					sleep(60);
				}

				close(servSock);
			}
		}
	}else{
		cout 	<< "Could not connect to database server. Aborting." << endl
				<< "\tThe following error was reported: " << mysql_error(db.getHandle()) << endl;
		log.putLog(2, "051", "Could not connect to database server. Aborting.");
	}

	// Not reached.

	return 0;
}
