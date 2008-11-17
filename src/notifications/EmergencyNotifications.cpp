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

#include "EmergencyNotifications.h"
#include "../database/Database.h"
#include "Mail.h"
#include "XMPP.h"
#include "../database/Information.h"
#include "../internal.h"

EmergencyNotifications::EmergencyNotifications(mySQLData myDBData, mailingData myMailData,
		XMPPData myXMPPData, mobilecData myMobilecData)
			: Database(myDBData){
	dbData = myDBData;
	mailData = myMailData;
	xmppData = myXMPPData;
	clickatellData = myMobilecData;
}

/*
 * Return values:
 * 	1: Emergency notifications to send fetched.  
 * 	0: Nothing to send
 * -1: Error
 */

int EmergencyNotifications::fetchTasks(){
	if(initConnection()){
		MYSQL* init = getHandle();
		// Get tasks.  
		const char* taskQuery = "SELECT email, jid, emergencyid, ID, numberc"
							" FROM emergencynotifications "
							" WHERE notifiedon = 0"
							" LIMIT 250";
		
		if(mysql_real_query(init, taskQuery, strlen(taskQuery)) != 0){
			// Query was not successful.  
			mysql_close(init);
			return 0;
		}else{
			// Query successful.  
			MYSQL_RES* res = mysql_store_result(init);
			MYSQL_ROW row;
			if(mysql_num_rows(res) > 0){
				unsigned int i = 0;
				// Got notifications to send.  
				while((row = mysql_fetch_row(res))){
					stringstream address;
					if(strlen(row[0]) > 0){
						// This is an email address.
						address	<< "mail|"
								<< row[0]
								<< "|"
								<< row[2]
								<< "|"
								<< row[3];
					}else if(strlen(row[1]) > 0){
						// This is a jid.  
						address << "jid|"
								<< row[1]
								<< "|"
								<< row[2]
								<< "|"
								<< row[3];
					}else if(strlen(row[4]) > 0){
						// This is a Clickatell mobile number.  
						address << "mobilec|"
								<< row[4]
								<< "|"
								<< row[2]
								<< "|"
								<< row[3];
					}else{
						// No email address or jid. Skip.  
						i++;
						continue;
					}
					notificationList[i] = address.str();
					i++;
				}
				mysql_free_result(res);
				mysql_close(init);
				return 1;
			}else{
				// Nothing fetched.  
				mysql_free_result(res);
				mysql_close(init);
				return 1;
			}
		}
	}
	
	return -1;
}

int EmergencyNotifications::getListSize(){
	return notificationList.size();
}

enData EmergencyNotifications::splitEmergencyInformation(int recvNum){
	enData info;
	// Split the receiver to get the information.  
	string token;
	istringstream iss(notificationList[recvNum]);
	int i = 0;
	while(getline(iss, token, '|')){
		switch(i){
			case 0:
				// The type.  
				info.type = token;
				break;
			case 1:
				// The address.  
				info.address = token;
				break;
			case 2:
				// The emergency ID.  
				info.emergencyID = atoi(token.c_str());
				break;
			case 3:
				info.ID = atoi(token.c_str());
				break;
		}
		i++;
	}
	return info;
}

int EmergencyNotifications::sendMessage(int recvNum){
	enData info = splitEmergencyInformation(recvNum);

	// Check if we got all needed data.  
	if(info.type.length() <= 0 || info.address.length() <= 0 || info.emergencyID <= 0)
		return -1;
	
	// These are used to generate a message.  
	string description;
	string timestamp;
	string severity;
	string severityName;
	
	// Connect to database.  
	if(initConnection()){
		description = sGetQuery(Information::getEmergencyInformation(info.emergencyID, 0));
		timestamp = sGetQuery(Information::getEmergencyInformation(info.emergencyID, 1));
		severity = sGetQuery(Information::getEmergencyInformation(info.emergencyID, 2));
		
		// Close database connection.  
		mysql_close(getHandle());
	}else{
		// Could not connect to database.  
		return 0;
	}
	
	// Check if needed information was fetched without errors.  
	if(description == "NULL" || timestamp == "NULL" || severity == "NULL")
		return -1;
	
	// Convert severity types to names.  
	switch(atoi(severity.c_str())){
		case 1:
			severityName = "Medium";
			break;
		case 2:
			severityName = "High";
			break;
		case 3:
			severityName = "Disaster";
			break;
		default:
			return -1;
	}
	
	// Build the message
	stringstream message;
	message	<< "ALERT: An emergency has been declared!"
			<< endl << endl
			<< "Description:"
			<< description
			<< endl << endl
			<< "Severity: "
			<< severityName;
	
	// Everything seems to be okay. Send the message.  
	if(info.type == "jid"){
		// We need to send an XMPP message.  
		XMPP xmpp(xmppData, dbData);
		if(!xmpp.sendMessage(message.str(), info.address))
			return -1;
		return 1;
	}else if(info.type == "mail"){
		// We need to send an email.  
		Mail mailing(mailData);
		if(!mailing.sendMail(info.address, "An emergency has been declared", message.str()))
			return -1;
		return 1;
	}else if(info.type == "mobilec"){
		// We need to send an SMS via the Clickatell SMTP API.  
		Mail mailing(mailData);
		// Build the message that fits to the API.  
		stringstream newMessage;
		newMessage	<< "user:" << clickatellData.username << endl
					<< "password:" << clickatellData.password << endl
					<< "api_id:" << clickatellData.apiID << endl
					<< "to:" << info.address << endl
					<< "text:" << "[ScopePort] " << noNewLineS(message.str());
		if(!mailing.sendMail(CLICKATELLMAIL, "", newMessage.str()))
			return -1;
		return 1;
	}
	
	return -1;
}

bool EmergencyNotifications::markReceiver(int recvNum, int status){
	enData info = splitEmergencyInformation(recvNum);

	// Check if we got all needed data.  
	if(info.ID <= 0)
		return 0;
	
	// Connect to database.  
	if(initConnection()){
		if(!setQuery(getHandle(), Information::setReceiverMark(info.ID, status))){
			mysql_close(getHandle());
			return 0;
		}
		// Close database connection.  
		mysql_close(getHandle());
		return 1;
	}else{
		// Could not connect to database.  
		return 0;
	}
	
	return 0;
}
