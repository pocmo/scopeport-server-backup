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

#include "Information.h"
#include "../internal.h"
#include "Database.h"

string Information::getSensorName(string st){
	stringstream query;
	query	<< "SELECT name FROM sensors WHERE st = "
			<< st
			<< " LIMIT 1";
	return query.str();
}

string Information::getHostName(string hostid){
	stringstream query;
	query	<< "SELECT name FROM hosts WHERE hostid = "
			<< hostid
			<< " LIMIT 1";

	return query.str();
}

string Information::getSensorCondition(string hostid, string st){
	stringstream query;
	query	<< "SELECT type,value FROM sensor_conditions WHERE hostid = "
			<< hostid
			<< " AND st = "
			<< st;
	return query.str();
}

string Information::getReceiverGroup(string hostid, string st){
	stringstream query;
	query	<< "SELECT warninggroup FROM sensor_conditions WHERE hostid = "
			<< hostid
			<< " AND st = "
			<< st;

	return query.str();
}

/*
 *
 * getServiceReceiverGroup(string serviceID, int type)
 *
 * Type 0 = RudeCheck
 * Type 1 = SoftCheck
 *
 */

string Information::getServiceReceiverGroup(unsigned int serviceID){

	stringstream query;

	// RudeCheck.
	query	<< "SELECT warninggroup FROM services WHERE id = "
			<< serviceID;

	return query.str();
}

/*
 *
 * getServiceName(string serviceID, int type)
 *
 * Type 0 = RudeCheck
 * Type 1 = SoftCheck
 *
 */

string Information::getServiceName(unsigned int serviceID){
	stringstream query;

	query	<< "SELECT name FROM services WHERE id = "
			<< serviceID;

	return query.str();
}

string Information::getLastWarn(string hostid, string st){
	stringstream query;
	query	<< "SELECT lastwarn FROM sensor_conditions WHERE hostid = "
			<< hostid
			<< " AND st = "
			<< st;

	return query.str();
}

string Information::getLastServiceWarn(unsigned int serviceID){
	stringstream query;


	// RudeCheck.
	query	<< "SELECT lastwarn FROM services WHERE id = "
			<< serviceID;

	return query.str();
}

string Information::setLastWarn(time_t lastwarn, string hostid, string st){
	stringstream query;
	query	<< "UPDATE sensor_conditions SET lastwarn = '"
			<< time(&lastwarn)
			<< "' WHERE hostid = "
			<< hostid
			<< " AND st = "
			<< st;

	return query.str();
}

/*
 *
 * setLastServiceWarn(time_t lastwarn, string serviceID, int type)
 *
 * Type 0 = RudeCheck
 * Type 1 = SoftCheck
 *
 */

string Information::setLastServiceWarn(time_t lastwarn, unsigned int serviceID){
	stringstream query;

	// SoftCheck.
	query	<< "UPDATE services SET lastwarn = '"
			<< time(&lastwarn)
			<< "' WHERE id = "
			<< serviceID;

	return query.str();
}

string Information::getSensorSeverity(string hostID, string st){
	stringstream query;
	query	<< "SELECT severity FROM sensor_conditions WHERE hostid = "
			<< hostID
			<< " AND st = "
			<< st;

	return query.str();
}

vector<string> Information::getMailWarningReceivers(MYSQL* init, string groupID,
		string sevBorder){
	stringstream query;
	query	<<	"SELECT mail FROM notificationgroups WHERE warninggroup = "
			<<	groupID
			<<	" AND sevborder <= "
			<<	sevBorder
			<<	" AND email = 1"
				" AND deleted = 0"
				" LIMIT 500";

	vector<string> result;

	if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) == 0){
		// Query successful.
		MYSQL_RES* res = mysql_store_result(init);
		MYSQL_ROW  row;
		if(mysql_num_rows(res) > 0){
			while((row = mysql_fetch_row(res))){
				result.push_back(row[0]);
			}
		}
		mysql_free_result(res);
		return result;
	}
	result.push_back("NULL");
	return result;
}

vector<string> Information::getXMPPWarningReceivers(MYSQL* init, string groupID,
	string sevBorder){

	stringstream query;
	query	<< 	"SELECT jid FROM notificationgroups WHERE warninggroup = "
			<< 	groupID
			<< 	" AND sevborder <= "
			<< 	sevBorder
			<< 	" AND xmpp = 1"
				" AND deleted = 0"
				" LIMIT 500";

	vector<string> result;

	if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) == 0){
		// Query successful.
		MYSQL_RES* res = mysql_store_result(init);
		MYSQL_ROW  row;
		if(mysql_num_rows(res) > 0){
			while((row = mysql_fetch_row(res))){
				result.push_back(row[0]);
			}
		}
		mysql_free_result(res);
		return result;
	}
	result.push_back("NULL");
	return result;
}

vector<string> Information::getMobileCWarningReceivers(MYSQL* init, string groupID,
	string sevBorder){

	stringstream query;
	query	<< 	"SELECT numberc FROM notificationgroups WHERE warninggroup = "
			<< 	groupID
			<< 	" AND sevborder <= "
			<< 	sevBorder
			<< 	" AND mobilec = 1"
				" AND deleted = 0"
				" LIMIT 500";

	vector<string> result;

	if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) == 0){
		// Query successful.
		MYSQL_RES* res = mysql_store_result(init);
		MYSQL_ROW  row;
		if(mysql_num_rows(res) > 0){
			while((row = mysql_fetch_row(res))){
				result.push_back(row[0]);
			}
		}
		mysql_free_result(res);
		return result;
	}
	result.push_back("NULL");
	return result;
}

string Information::clearHealth(){
	return "TRUNCATE TABLE vitals";
}

string Information::updateHealth(string pid, bool clienthandler, string vmem, string threads,
		int packetsOK, int packetsERR, double dbTotalSize, double dbSensorSize,
		double dbServiceSize){
	time_t rawtime;
	time(&rawtime);

	stringstream query;

	query 	<<	"INSERT INTO vitals (pid, clienthandler, vmem, "
				"threads, timestamp, packetsOK, packetsERR";

	if(dbTotalSize >= 0){
		query	<< ",dbtotalsize, dbsensorsize, dbservicesize";
	}

	query	<<	") VALUES ('"
			<<	pid
			<<	"', '"
			<<	clienthandler
			<<	"', '"
			<<	vmem
			<<	"', '"
			<<	threads
			<<	"', '"
			<<	rawtime
			<<	"', '"
			<<	packetsOK
			<<	"', '"
			<<	packetsERR;

	if(dbTotalSize >= 0){
		query	<<	"', '"
				<<	dbTotalSize
				<<	"', '"
				<<	dbSensorSize
				<<	"', '"
				<<	dbServiceSize;
	}


	query	<<	"')"
				" ON DUPLICATE KEY"
				" UPDATE vmem = '"
			<< vmem
			<< "', threads = '"
			<< threads
			<< "', timestamp = '"
			<< rawtime
			<< "', packetsOK = '"
			<< packetsOK
			<< "', packetsERR = '"
			<< packetsERR;

	if(dbTotalSize >= 0){
		query	<< "', dbtotalsize= '"
				<< dbTotalSize
				<< "', dbsensorsize = '"
				<< dbSensorSize
				<< "', dbservicesize = '"
				<< dbServiceSize;
	}

	query	 << "'";

	return query.str();
}

/*
 *
 * Available types:
 * 	0: Description
 * 	1: Timestamp of creation
 * 	2: Severity (numeric)
 *
 */

string Information::getEmergencyInformation(int emergencyID, int type){

	string whatToFetch;

	switch(type){
		case 0:
			whatToFetch = "emergencies.description";
			break;
		case 1:
			whatToFetch = "emergencies.timestamp";
			break;
		case 2:
			whatToFetch = "emergencies.severity";
			break;
		default:
			return "NULL";
	}

	stringstream query;

	query	<<	"SELECT "
			<< whatToFetch
			<<	" FROM emergencies"
				" WHERE active = 1"
				" AND emergencies.ID = "
			<< emergencyID;

	return query.str();
}

string Information::setReceiverMark(int ID, int status){
	stringstream query;

	stringstream setStatus;

	// Find out what to set as status.
	switch(status){
		case 0:
			setStatus << "0";
			break;
		case -1:
			setStatus << "-1";
			break;
		case 1:
			time_t rawtime;
			setStatus << time(&rawtime);
			break;
		default:
			return "NULL";
	}

	query	<< "UPDATE emergencynotifications"
				" SET notifiedon = "
			<< setStatus.str()
			<< " WHERE ID = "
			<< ID;

	return query.str();
}
