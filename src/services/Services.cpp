// This file is part of ScopePort (Linux server).
//
// Copyright 2008, 2009 Lennart Koopmann
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

#include "../internal.h"
#include "Services.h"
#include "../database/Database.h"
#include "../notifications/Mail.h"
#include "../notifications/XMPP.h"
#include "../notifications/Clickatell.h"
#include "../log/Log.h"
#include "../database/Information.h"
#include "../misc/Timer.h"

Services::Services(mySQLData myDBData, unsigned int myHandlerID)
			: Database(myDBData){
	dbData = myDBData;
	handlerID = myHandlerID;
	responseTime = 0;
}

void Services::setServiceID(unsigned int x){
	serviceID = x;
}

void Services::setAllowedFails(unsigned int x){
	allowedFails = x;
}

void Services::setPort(unsigned int x){
	port = x;
}

void Services::setMaximumResponse(unsigned int x){
	maximumResponse = x;
}

void Services::setHost(string x){
	host = x;
}

void Services::setServiceType(string x){
	serviceType = x;
}

void Services::setNotiGroup(string x){
	notiGroup = x;
}

void Services::setHostname(string x){
	hostname = x;
}

unsigned int Services::getServiceID(){
	return serviceID;
}

unsigned int Services::getHandlerID(){
	return handlerID;
}

unsigned int Services::getMaximumResponse(){
	return maximumResponse;
}

int Services::checkService(){
	Log log(LOGFILE, dbData);
	int sock;
	struct sockaddr_in server;
	struct hostent *host;

	// Create socket.
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		// Creating socket failed.
		close(sock);
		updateStatus(3);
		return -1;
	}

	// Verify host.
	server.sin_family = AF_INET;
	host = gethostbyname(hostname.c_str());
	if(host==(struct hostent *) 0){
		// Host unknown.
		close(sock);
		updateStatus(0);
		return 0;
	}

	// Connect to defined port.
	memcpy((char *) &server.sin_addr, (char *) host->h_addr, host->h_length);
	server.sin_port=htons(port);

	sleep(1);

	// Connect and measure time it takes to connect.
	Timer t;
	t.startTimer();
	int res = connect(sock, (struct sockaddr *) &server, sizeof server);
	responseTime = t.stopTimer();

	if(res < 0) {
		// Could not connect. - Service not running.
		close(sock);
		responseTime = 0;
		updateStatus(0);
		return 0;
	}else{
		// Connection successful! Service is running.
		if(serviceType == "none"){
			// This service needs no protocol check.
			// Service is running. Check response time.
			if(!storeResponseTime())
				log.putLog(2, "008", "Could not update service response time.");

			// Check if the response time is higher than the defined maximum.
			if(!checkResponseTime()){
				// This service has a too high response time.
				updateStatus(2);
				return 2;
			}else{
				// Everything okay.
				updateStatus(1);
				return 1;
			}
		}else if(serviceType == "smtp"){
			responseTime = checkSMTP(sock);
		}else if(serviceType == "http"){
			responseTime = checkHTTP(sock);
		}else if(serviceType == "imap"){
			responseTime = checkIMAP(sock);
		}else if(serviceType == "pop3"){
			responseTime = checkPOP3(sock);
		}else if(serviceType == "ssh"){
			responseTime = checkSSH(sock);
		}else if(serviceType == "ftp"){
			responseTime = checkFTP(sock);
		}else{
			// Unknown service type.
			close(sock);
			updateStatus(3);
			return -1;
		}

		if(responseTime > 0){
			// Service is running. Check response time.
			if(!storeResponseTime())
				log.putLog(2, "003", "Could not update service response time.");
			// Check if the response time is higher than the defined maximum.
			if(!checkResponseTime()){
				// This service has a too high response time.
				close(sock);
				updateStatus(2);
				return 2;
			}else{
				// Everything okay.
				close(sock);
				updateStatus(1);
				return 1;
			}
		}else{
			close(sock);
			updateStatus(0);
			return 0;
		}

	}

	close(sock);
	return -1;
}

void Services::updateStatus(int status){
	Database db(dbData);
	Log log(LOGFILE, dbData);
	if(db.initConnection()){
		time_t rawtime;
		stringstream query;
		query	<< "UPDATE services SET state = "
				<< status
				<< " , lastcheck = "
				<< time(&rawtime)
				<< " WHERE id = "
				<< serviceID;
		db.setQuery(db.getHandle(), query.str());
		mysql_close(db.getHandle());
	}else{
		log.putLog(2, "4000", "Could not update status of service");
		mysql_close(db.getHandle());
	}
}

bool Services::checkResponseTime(){
	if(responseTime == 0)
		return 1;

	// Initialize database connection.
	if(initConnection()){

		string table;

		stringstream query;
		query	<< "SELECT maxres FROM services WHERE id = "
				<< serviceID;

		setMaximumResponse(stringToInteger(sGetQuery(query.str()).c_str()));

		if(responseTime > getMaximumResponse()){
			// Higher than defined maximum!
			mysql_close(getHandle());
			return 0;
		}

		mysql_close(getHandle());
	}

	// All went fine. The response was not higher than defined maximum.
	return 1;
}

void Services::sendWarning(){
	Log log(LOGFILE, dbData);

	// Initialize database connection.
	if(initConnection()){

		mailingData mailData;
		XMPPData xmppData;
		mobilecData mobilecData;

		mailData = Mail::fetchSettings(dbData);
		xmppData = XMPP::fetchSettings(dbData);
		mobilecData = Clickatell::fetchSettings(dbData);

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

		// Create XMPP object.
		XMPP xmpp(xmppData, dbData);

		// Check for XMPP parameters.
		if(xmppData.doXMPP && !xmppData.xmppServer.empty() && xmppData.xmppPort > 0
				&& !xmppData.xmppUser.empty() && !xmppData.xmppPass.empty()
				&& !xmppData.xmppResource.empty()){
			// All XMPP parameters set correctly.
			xmppData.doXMPP = 1;
		}else{
			// Parameters missing - Disable XMPP.
			xmppData.doXMPP = 0;
		}

		// Check for Clickatell SMS parameters.
		if(mobilecData.doMobileC && !mobilecData.username.empty()
				&& !mobilecData.password.empty()
				&& !mobilecData.apiID.empty()){
			mobilecData.doMobileC = 1;
		}else{
			// Parameters missing - Disable Clickatell SMS.
			mobilecData.doMobileC = 0;
		}

		stringstream warningSubj;
		stringstream warningMsg;

		string serviceName;

		serviceName = sGetQuery(Information::getServiceName(serviceID));

		int qms = 0;

		if(responseTime == 0){
			// It is a "normal" failed service.
			warningSubj << "Warning! Service \""
						<< serviceName
						<< "\" seems to have failed!";

			warningMsg	<< "Warning! Service \""
						<< serviceName
						<< "\" ("
						<< hostname
						<< " on Port "
						<< port
						<< ") seems to have failed!";
			qms = 0;
		}else{
			// It is a warning for a too high response time.
			warningSubj << "Warning! Service \""
						<< serviceName
						<< "\" has a too high response time!";

			warningMsg	<< "Warning! Service \""
						<< serviceName
						<< "\" ("
						<< hostname
						<< " on Port "
						<< port
						<< ") has response time \""
						<< responseTime
						<< "ms\"!";
			qms = responseTime;
		}

		string lastWarn;
		lastWarn = sGetQuery(Information::getLastServiceWarn(serviceID));

		time_t rawtime;
		time(&rawtime);

		// Only send warning all 6 hours.
		if(rawtime-stringToInteger(lastWarn.c_str()) > 21600){

			// Build query for scheduled downtime check.
			stringstream checkDowntimeQuery;
			checkDowntimeQuery	<< "SELECT id FROM downtimes"
									" WHERE serviceid = " << serviceID <<
								   " AND type = 2 AND `from` < " << rawtime <<
								   " AND `to` > " << rawtime;


			if(mysql_real_query(getHandle(), checkDowntimeQuery.str().c_str(),
					strlen(checkDowntimeQuery.str().c_str())) != 0){
				// Query was not successful.
				log.putLog(1, "5678", "Could not fetch downtimes for service alarm.");
				mysql_close(getHandle());
				return;
			}else{
				// Query successful.

				time_t alarmtime;
				time(&alarmtime);

				// Check if a downtime was scheduled for this service.
				MYSQL_RES* sdres = mysql_store_result(getHandle());
				if(mysql_num_rows(sdres) > 0){
					// A downtime was scheduled. Don't warn.
					mysql_free_result(sdres);
					mysql_close(getHandle());
					return;
				}
				mysql_free_result(sdres);

				// We need to send warnings.

				// Set last warning time.
				time_t warntime;
				time(&warntime);
				setQuery(getHandle(), Information::setLastServiceWarn(warntime, serviceID));

				// Insert warning into database.
				stringstream alarmSQL;
				alarmSQL	<< "INSERT INTO alarms (alarm_type, timestamp, serviceid, ms) VALUES ('2','"
							<< alarmtime
							<< "','"
							<< serviceID
							<< "','"
							<< qms
							<< "')";

				if(!setQuery(getHandle(), alarmSQL.str()))
					log.putLog(2, "002", "Could not update alarms table.");


				if(mailData.doMailing){
					vector<string> mailRecvList = Information::getMailWarningReceivers(getHandle(),
							sGetQuery(Information::getServiceReceiverGroup(serviceID)), "4");

					int mailRecvCount = 0;
					int mailRecvListSize = mailRecvList.size();
					while(mailRecvCount < mailRecvListSize){
						if(!mailRecvList[mailRecvCount].empty())
							mailing.sendMail(mailRecvList[mailRecvCount], warningSubj.str(), warningMsg.str());
						mailRecvCount++;
					}
				}

				// Get XMPP receivers.
				if(xmppData.doXMPP){
					vector<string> xmppRecvList = Information::getXMPPWarningReceivers(getHandle(),
												sGetQuery(Information::getServiceReceiverGroup(
														serviceID)),"3");

					// Send a warning message to every XMPP receiver.
					int xmppRecvCount = 0;
					int xmppRecvListSize = xmppRecvList.size();
					while(xmppRecvCount < xmppRecvListSize){
							if(!xmppRecvList[xmppRecvCount].empty())
									xmpp.sendMessage(warningMsg.str(), xmppRecvList[xmppRecvCount]);
							xmppRecvCount++;
					}
				}

				if(mobilecData.doMobileC && mailData.doMailing){
					vector<string> mobilecRecvList = Information::getMobileCWarningReceivers(getHandle(),
							sGetQuery(Information::getServiceReceiverGroup(serviceID)), "4");

					int mobilecRecvCount = 0;
					int mobilecRecvListSize = mobilecRecvList.size();
					while(mobilecRecvCount < mobilecRecvListSize){
						if(!mobilecRecvList[mobilecRecvCount].empty()){
							// Build the message that fits to the API.
							stringstream newWarningMsg;
							newWarningMsg	<< "user:" << mobilecData.username << endl
											<< "password:" << mobilecData.password << endl
											<< "api_id:" << mobilecData.apiID << endl
											<< "to:" << mobilecRecvList[mobilecRecvCount] << endl
											<< "text:" << warningMsg.str();
							mailing.sendMail(CLICKATELLMAIL, warningSubj.str(), newWarningMsg.str());
						}
						mobilecRecvCount++;
					}
				}

			}
		}
		mysql_close(getHandle());
	}else{
		log.putLog(2, "1234", "Could not connect to database for service warning/alarm.");
	}
}

bool Services::storeResponseTime(){
	// Initialize database connection.
	if(initConnection()){
		stringstream queryLastData;
		stringstream queryAllData;

		time_t thistime;
		time(&thistime);

		queryAllData << "INSERT INTO servicerecords(timestamp, serviceid, ms) VALUES('"
						<< thistime
						<< "', '"
						<< serviceID
						<< "', '"
						<< responseTime
						<< "')";

		queryLastData	<< "UPDATE services SET responsetime = "
						<< responseTime
						<< " WHERE id = "
						<< serviceID;

		if(!setQuery(getHandle(), queryLastData.str())){
			mysql_close(getHandle());
			return 0;
		}

		if(!setQuery(getHandle(), queryAllData.str())){
			mysql_close(getHandle());
			return 0;
		}

		mysql_close(getHandle());
		return 1;
	}

	return 0;
}

#define CHECKBUFSIZE 1024

int Services::checkSMTP(int sock){

	int ms = 1;

	// Our buffer.
	char checkBuffer[CHECKBUFSIZE];

	// Will hold the length of the reply.
	int len;

	// The message to send to the server.
	const char* message = "EHLO example.org\n\r\n\r";

	// Send the message.
	if((len = send(sock,message,strlen(message),0)) <= 0)
		return 0;

	Timer t;

	t.startTimer();

	// Read the answer and keep the length of the reply.
	if((len = read(sock,checkBuffer,CHECKBUFSIZE-1)) <= 0)
		return 0;

	ms = t.stopTimer();
	// Quit message to close the connection.
	const char* quitMessage = "QUIT";

	// Send the quit message.
	if((len = send(sock,quitMessage,strlen(quitMessage),0)) <= 0)
		return 0;

	// Terminate the string if the reply was not too long.
	if(len < CHECKBUFSIZE){
		checkBuffer[len] = '\0';
	}else{
		return 0;
	}

	// Make it easier for us to substr().
	stringstream reply;
	reply << checkBuffer;

	// If the first three chars are "220", there is a SMTP server running.
	if(reply.str().substr(0,3) == "220")
		return ms;

	// The first three chars of the reply were not "220". No SMTP server running.
	return 0;
}

int Services::checkHTTP(int sock){

	int ms = 1;

	// Our buffer.
	char checkBuffer[CHECKBUFSIZE];

	// Will hold the length of the reply.
	int len;

	const char* message = "HEAD / HTTP/1.0\n\r\n\r";

	// Send the message.
	if((len = send(sock,message,strlen(message),0)) <= 0)
		return 0;

	Timer t;

	t.startTimer();

	// Read the answer and keep the length of the reply.
	if((len = read(sock,checkBuffer,CHECKBUFSIZE-1)) <= 0)
		return 0;

	ms = t.stopTimer();

	// Terminate the string if the reply was not too long.
	if(len < CHECKBUFSIZE){
		checkBuffer[len] = '\0';
	}else{
		return 0;
	}

	// Make it easier for us to substr().
	stringstream reply;
	reply << checkBuffer;

	// If the first five chars are "HTTP/", there is a HTTP server running.
	if(reply.str().substr(0,5) == "HTTP/")
		return ms;

	// The first five chars of the reply were not "HTTP/". No HTTP server running.
	return 0;
}

int Services::checkIMAP(int sock){

	int ms = 1;

	// Our buffer.
	char checkBuffer[CHECKBUFSIZE];

	// Will hold the length of the reply.
	int len;

	Timer t;

	t.startTimer();

	// Read the answer and keep the length of the reply.
	if((len = read(sock,checkBuffer,CHECKBUFSIZE-1)) <= 0)
		return 0;

	ms = t.stopTimer();

	// Terminate the string if the reply was not too long.
	if(len < CHECKBUFSIZE){
		checkBuffer[len] = '\0';
	}else{
		return 0;
	}

	// Make it easier for us to substr().
	stringstream reply;
	reply << checkBuffer;

	// If the first four chars are "* OK", there is an IMAP server running.
	if(reply.str().substr(0,4) == "* OK")
		return ms;

	// The first four chars of the reply were not "* OK". No IMAP server running.
	return 0;
}

int Services::checkPOP3(int sock){

	int ms = 1;

	// Our buffer.
	char checkBuffer[CHECKBUFSIZE];

	// Will hold the length of the reply.
	int len;

	Timer t;

	t.startTimer();

	// Read the answer and keep the length of the reply.
	if((len = read(sock,checkBuffer,CHECKBUFSIZE-1)) <= 0)
		return 0;

	ms = t.stopTimer();

	// Quit message to close the connection.
	const char* message = "QUIT\n";

	// Send the quit message.
	if((len = send(sock,message,strlen(message),0)) <= 0)
		return 0;

	// Terminate the string if the reply was not too long.
	if(len < CHECKBUFSIZE){
		checkBuffer[CHECKBUFSIZE-1] = '\0';
	}else{
		return 0;
	}

	// Make it easier for us to substr().
	stringstream reply;
	reply << checkBuffer;

	// If the first three chars are "* OK", there is an POP3 server running.
	if(reply.str().substr(0,3) == "+OK")
		return ms;

	// The first three chars of the reply were not "+OK". No POP3 server running.
	return 0;
}

int Services::checkSSH(int sock){

	int ms = 1;

	// Our buffer.
	char checkBuffer[CHECKBUFSIZE];

	// Will hold the length of the reply.
	int len;

	Timer t;

	t.startTimer();

	// Read the answer and keep the length of the reply.
	if((len = read(sock,checkBuffer, CHECKBUFSIZE-1)) <= 0)
		return 0;

	ms = t.stopTimer();

	// Terminate the string if the reply was not too long.
	if(len < CHECKBUFSIZE){
		checkBuffer[len] = '\0';
	}else{
		return 0;
	}

	// Make it easier for us to substr().
	stringstream reply;
	reply << checkBuffer;

	// If the first three chars are "SSH", there is an SSH server running.
	if(reply.str().substr(0,3) == "SSH")
		return ms;

	// The first three chars of the reply were not "SSH". No SSH server running.
	return 0;
}

int Services::checkFTP(int sock){

	/*
	 * Because the FTP protocol is very similar to the SMTP
	 * protocol, we need to do some more checks.
	 * We will check the first reply - If it begins with 220
	 * there might be a FTP server running. Then we try to login.
	 * If the next reply begins with 331, there is a very good
	 * chance to have a FTP server.
	 *
	 */

	int ms = 1;

	// Our buffer.
	char checkBuffer[CHECKBUFSIZE];

	// Will hold the length of the reply.
	int len;

	Timer t;

	t.startTimer();

	// Read the answer and keep the length of the reply.
	if((len = read(sock,checkBuffer, CHECKBUFSIZE-1)) <= 0)
		return 0;

	ms = t.stopTimer();

	// Terminate the string if the reply was not too long.
	if(len < CHECKBUFSIZE){
		checkBuffer[len] = '\0';
	}else{
		return 0;
	}

	// Make it easier for us to substr().
	stringstream reply;
	reply << checkBuffer;

	// If the first three chars are "220", there could be a FTP server running.
	if(reply.str().substr(0,3) != "220")
		return 0;

	len = 0;

	// Try to login to make sure there is a FTP server running.
	const char* testLoginMessage = "USER scopeport-service-check\n";

	// Send the quit message.
	if((len = send(sock,testLoginMessage,strlen(testLoginMessage),0)) <= 0)
		return 0;

	// Read the answer and keep the length of the reply.
	if((len = read(sock,checkBuffer, CHECKBUFSIZE-1)) <= 0)
		return 0;

	// Terminate the string if the reply was not too long.
	if(len < CHECKBUFSIZE){
		checkBuffer[CHECKBUFSIZE-1] = '\0';
	}else{
		return 0;
	}

	// Make it easier for us to substr().
	stringstream reply2;
	reply2 << checkBuffer;

	// If the first three chars are "331", there is a FTP server running.
	if(reply2.str().substr(0,3) != "331")
		return 0;

	const char* quitMessage = "QUIT\n";

	// Send the quit message.
	len = send(sock,quitMessage,strlen(quitMessage),0);

	// All tests performed. There is a FTP server running.
	return ms;
}
