// This file is part of ScopePort (Linux server).
//
// Copyright 2008 Lennart Koopmann
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
#include "../log/Log.h"
#include "../database/Information.h"

Services::Services(mySQLData myDBData)
			: Database(myDBData){
	dbData = myDBData;
}

bool Services::checkResponseTime(string checkID, int ms){
	// Initialize database connection.
	if(initConnection()){

		string table;

		stringstream query;
		query	<< "SELECT maxres FROM services WHERE id = "
				<< checkID;

		string maxres = sGetQuery(query.str());

		if(ms > atoi(maxres.c_str())){
			// Higher than defined maximum!
			mysql_close(getHandle());
			return 0;
		}

		mysql_close(getHandle());
	}

	// All went fine. The response was not higher than defined maximum.
	return 1;
}

void Services::sendWarning(string checkID, string recpGroup, string hostname,
		int port, int ms){
	// Exit if the receiver is emtpy.
	if(recpGroup.empty())
		return;

	// Initialize database connection.
	if(initConnection()){

		mailingData mailData;
		XMPPData xmppData;
		mobilecData mobilecData;

		// Get config parameters from database.
		const char* getSettingsSQL = "SELECT mail_enabled, mail_useauth, mail_server,"
								"mail_port, mail_user, mail_pass, mail_hostname,"
								"mail_from, xmpp_enabled, xmpp_server, xmpp_port,"
								"xmpp_user, xmpp_pass, xmpp_resource, doMobileClickatell,"
								"mobilecUsername, mobilecPassword, mobilecAPIID FROM settings";

		if(mysql_real_query(getHandle(), getSettingsSQL, strlen(getSettingsSQL)) == 0){
			// Query successful. Fetch result pointer.
			MYSQL_RES* res = mysql_store_result(getHandle());
			MYSQL_ROW row;
			// Fill row variable with data from result.
			row = mysql_fetch_row(res);
			stringstream result;
			// Were rows fetched?
			if(mysql_num_rows(res) > 0){
				// Yes. Go on.

				// mail_enabled.
				if(strcmp(row[0],"1") == 0){
					mailData.doMailing = 1;
				}else{
					mailData.doMailing = 0;
				}

				// mail_useauth.
				if(strcmp(row[1],"1") == 0){
					mailData.mailUseAuth = 1;
				}else{
					mailData.mailUseAuth = 0;
				}

				// mail_server.
				mailData.mailServer = row[2];

				// mail_port.
				mailData.mailPort = atoi(row[3]);

				// mail_user.
				mailData.mailUser = row[4];

				// mail_pass.
				mailData.mailPass = row[5];

				// mail_hostname.
				mailData.mailHostname = row[6];

				// mail_from.
				mailData.mailFrom = row[7];

				// xmpp_enabled.
				xmppData.doXMPP = row[8];

				// xmpp_server.
				xmppData.xmppServer = row[9];

				// xmpp_port.
				xmppData.xmppPort = atoi(row[10]);

				// xmpp_user.
				xmppData.xmppUser = row[11];

				// xmpp_pass.
				xmppData.xmppPass = row[12];

				// xmpp_resource.
				xmppData.xmppResource = row[13];

				// doMobileClickatell.
				mobilecData.doMobileC = row[14];

				// mobilecUsername.
				mobilecData.username = row[15];

				// mobilecPassword.
				mobilecData.password = row[16];

				// mobilecAPIID.
				mobilecData.apiID = row[17];
			}else{
				// No rows fetched. Disable mailing.
				mailData.doMailing = 0;
			}
			mysql_free_result(res);
		}else{
			// Query failed. Disable mailing.
				mailData.doMailing = 0;
		}

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

		serviceName = sGetQuery(Information::getServiceName(checkID));

		int qms = 0;

		if(ms == 0){
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
						<< ms
						<< "ms\"!";
			qms = 1;
		}

		string lastWarn;
		lastWarn = sGetQuery(Information::getLastServiceWarn(checkID));

		time_t rawtime;
		time(&rawtime);

		// Only send warning all 6 hours.
		if(rawtime-atoi(lastWarn.c_str()) > 21600){

			// Build query for scheduled downtime check.
			stringstream checkDowntimeQuery;
			checkDowntimeQuery	<< "SELECT ID FROM downtimes"
									" WHERE serviceid = " << checkID <<
								   " AND type = 2 AND `from` < " << rawtime <<
								   " AND `to` > " << rawtime;


			if(mysql_real_query(getHandle(), checkDowntimeQuery.str().c_str(),
					strlen(checkDowntimeQuery.str().c_str())) != 0){
				// Query was not successful.
				mysql_close(getHandle());
				return;
			}else{
				// Query successful.

				time_t alarmtime;
				time(&alarmtime);

				Log log(LOGFILE, dbData);

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
				setQuery(getHandle(), Information::setLastServiceWarn(warntime, checkID));

				// Insert warning into database.
				stringstream alarmSQL;
				alarmSQL	<< "INSERT INTO alarms (type, timestamp, checkid, ms) VALUES ('2','"
							<< alarmtime
							<< "','"
							<< checkID
							<< "','"
							<< qms
							<< "')";

				if(!setQuery(getHandle(), alarmSQL.str()))
					log.putLog(2, "002", "Could not update alarms table.");


				if(mailData.doMailing){
					vector<string> mailRecvList = Information::getMailWarningReceivers(getHandle(),
							sGetQuery(Information::getServiceReceiverGroup(checkID)), "4");

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
														checkID)),"3");

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
							sGetQuery(Information::getServiceReceiverGroup(checkID)), "4");

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
	}
}

bool Services::storeResponseTime(string checkid, int ms){
	// Initialize database connection.
	if(initConnection()){
		stringstream queryLastData;
		stringstream queryAllData;

		time_t thistime;
		time(&thistime);

		queryAllData << "INSERT INTO servicedata(timestamp, serviceid, type, ms) VALUES('"
						<< thistime
						<< "', '"
						<< checkid
						<< "', '"
						<< 1
						<< "', '"
						<< ms
						<< "')";

		queryLastData	<< "UPDATE services SET responsetime = "
						<< ms
						<< " WHERE id = "
						<< checkid;

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
