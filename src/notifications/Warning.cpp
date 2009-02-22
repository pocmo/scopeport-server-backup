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
#include "../database/Database.h"
#include "Warning.h"
#include "../log/Log.h"

Warning::Warning(mySQLData myDBData)
		: Database(myDBData){
	dbData = myDBData;
}

bool Warning::getConditions(string hostid, string st){
	// Clear the list of conditions - Needed if this is used as a refresh.
	conditionList.clear();
	// Initialize database connection.
	if(initConnection()){
		MYSQL* init = getHandle();
		// Get all enabled hosts.
		stringstream query;
		query	<< "SELECT type,value FROM sensor_conditions "
					"WHERE disabled = 0 AND hostid = "
				<< hostid
				<< " AND st = "
				<< st;
		if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) != 0){
			// Query was not successful.
			error = mysql_error(init);
			mysql_close(init);
			return 0;
		}else{
			// Query successful.
			MYSQL_RES* res = mysql_store_result(init);
			MYSQL_ROW row;
			int i = 0;
			if(mysql_num_rows(res) > 0){
				while((row = mysql_fetch_row(res))){
					stringstream thiscondition;
					thiscondition << hostid << "." << st << ":" << row[0] << "." << row[1];
					conditionList[i] = thiscondition.str();
					i++;
				}
			}else{
				// Nothing defined in database.
				mysql_free_result(res);
				mysql_close(init);
				return 1;
			}
			mysql_free_result(res);
			mysql_close(init);
			// Just to make sure...
			if(conditionList.size() < 0)
				return 0;
			return 1;
		}
	}
	return 0;
}

/*
 * Returns	-1 if something failed
 * 			 0 if Sensor value is not in condition range.
 * 			 1 if Sensor is okay
 */

int Warning::checkSensor(string checkHostID, string checkSt, string checkSv, string lastWarn){
	// Just to make sure...
	if(conditionList.size() <= 0)
		return -1;

	int i = 0;
	int conditionSize = conditionList.size();
	while(i < conditionSize){
		string hostID;
		string st;
		string conditionType;
		string conditionValue;

		int j = 0;

		string token;
		istringstream iss(conditionList[i]);
		while(getline(iss, token, ':')){
			if(j == 0){
				int k = 0;
				// Split host ID and sensor ID.
				string token2;
				istringstream iss2(token);
				while(getline(iss2, token2, '.')){
					if(k == 0) hostID = token2;
					if(k == 1) st = token2;
					k++;
				}
			}
			if(j == 1){
				int k = 0;
				// Split sensor type and value.
				string token2;
				istringstream iss2(token);
				while(getline(iss2, token2, '.')){
					if(k == 0) conditionType = token2;
					if(k == 1) conditionValue = token2;
					k++;
				}
			}
			j++;
		}

		// Check if we got everything.
		if(hostID.empty() || st.empty() || conditionType.empty() || conditionValue.empty())
			continue;

		// Looks good. Go on.

		time_t rawtime;
		time(&rawtime);

		// Only send warning all 6 hours.
		if(rawtime-stringToInteger(lastWarn.c_str()) > 21600){

			// Find the sensor we want to compare.
			if(hostID == checkHostID && st == checkSt){
				if(conditionType == "below"){
					if(atof(checkSv.c_str()) < atof(conditionValue.c_str())){
							return 1;
						}
					updateAlarms(checkHostID, checkSt, checkSv);
					return 0;
				}

				if(conditionType == "equal"){
					if(atof(checkSv.c_str()) == atof(conditionValue.c_str())){
							return 1;
						}
					updateAlarms(checkHostID, checkSt, checkSv);
					return 0;
				}

				if(conditionType == "above"){
					if(atof(checkSv.c_str()) > atof(conditionValue.c_str())){
							return 1;
						}
					updateAlarms(checkHostID, checkSt, checkSv);
					return 0;
				}
			}
		}
		i++;
	}
	return -1;
}

void Warning::updateAlarms(string checkHostID, string checkSt, string checkSv){

	time_t rawtime;
	time(&rawtime);

	Log log(LOGFILE, dbData);

	// Insert alarm into database.
	stringstream alarmSQL;
	alarmSQL	<< "INSERT INTO alarms (type, timestamp, hostid, st, sv) VALUES ('1','"
				<< rawtime
				<< "','"
				<< checkHostID
				<< "','"
				<< checkSt
				<< "','"
				<< checkSv
				<< "')";

	if(initConnection()){
		if(!setQuery(getHandle(), alarmSQL.str()))
			log.putLog(1, "009", "Could not update alarms table.");
	}else{
		log.putLog(1, "010", "Could not update alarms table.");
	}

	mysql_close(getHandle());
}
