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

#include "Health.h"
#include "../database/Database.h"
#include "../internal.h"

string Health::getPID(){
	string returnage = "0";

	// Open the /proc/self/status file.
	ifstream status("/proc/self/status");

	if(!status.fail()){
		// We opened the file.
		string line;

		// Go to the 4th line.
		while(getline(status, line)){
			if(line.find("Pid") != string::npos){
				status.close();
				return noSpaces(line.erase(0,4));
			}
		}
	}

	status.close();

	return returnage;
}

string Health::getVMSize(){
	string returnage = "0";

	// Open the /proc/self/status file.
	ifstream status("/proc/self/status");

	if(!status.fail()){
		// We opened the file.
		string line;

		// Go to the 12th line.
		while(getline(status, line)){
			if(line.find("VmSize") != string::npos){
				status.close();
				return noSpaces(line.erase(0,7));
			}
		}
	}

	status.close();

	return returnage;
}

string Health::getThreads(){
	string returnage = "0";

	// Open the /proc/self/status file.
	ifstream status("/proc/self/status");

	if(!status.fail()){
		// We opened the file.
		string line;

		// Go to the 21st line.
		while(getline(status, line)){
			if(line.find("Threads") != string::npos){
				status.close();
				return noSpaces(line.erase(0,8));
			}
		}
	}

	status.close();

	return returnage;
}

double Health::getDBSize(mySQLData dbData, int type){
	// Prepare database connection.
	Database db(dbData);

	if(db.initConnection()){
		MYSQL* init = db.getHandle();
		// Get tasks.
		const char* taskQuery = "SHOW TABLE STATUS";

		if(mysql_real_query(init, taskQuery, strlen(taskQuery)) != 0){
			// Query was not successful.
			mysql_close(init);
			return 0;
		}else{
			// Query successful.
			MYSQL_RES* res = mysql_store_result(init);
			MYSQL_ROW row;
			if(mysql_num_rows(res) > 0){
				double totalSize = 0;
				// Got notifications to send.
				while((row = mysql_fetch_row(res))){
					switch(type){
						case 1:
							// Sum up everything to get the total size.
							totalSize = totalSize+stringToInteger(row[6])+stringToInteger(row[8]);
							break;
						case 2:
							// Only return size of table sensorvalues.
							if(strcmp(row[0], "sensorvalues") == 0){
								// Return data length + index length.
								mysql_free_result(res);
								mysql_close(init);
								return (stringToInteger(row[6])+stringToInteger(row[8]))/1024;
							}
							break;
						case 3:
							// Only return size of table servicerecords.
							if(strcmp(row[0], "servicerecords") == 0){
								// Return data length + index length.
								mysql_free_result(res);
								mysql_close(init);
								return (stringToInteger(row[6])+stringToInteger(row[8]))/1024;
							}
							break;
						default:
							mysql_free_result(res);
							mysql_close(init);
							return 0;
					}
				}
				mysql_free_result(res);
				mysql_close(init);
				// Convert byte to kilobyte and return total size.
				return totalSize/1024;
			}else{
				// Nothing fetched.
				mysql_free_result(res);
				mysql_close(init);
				return 0;
			}
		}
	}

	return 0;
}
