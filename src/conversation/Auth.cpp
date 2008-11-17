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

#include "Auth.h"
#include "../internal.h"

Auth::Auth(mySQLData myDBData)
		: Database(myDBData){
	dbData = myDBData;
}

bool Auth::loadHosts(){
	// Clear the hostlist - Needed if this is used as a refresh.  
	hostList.clear();
	// Initialize database connection.  
	if(initConnection()){
		MYSQL* init = getHandle();
		// Get all enabled hosts.  
		const char* query = "SELECT hostid,password FROM hosts WHERE disabled = 0";
		if(mysql_real_query(init, query, strlen(query)) != 0){
			// Query was not successful.  
			error = mysql_error(init);
			mysql_close(init);
			return 0;
		}else{
			// Query successful.  
			MYSQL_RES* res = mysql_store_result(init);
			MYSQL_ROW row;
			if(mysql_num_rows(res) > 0){
				while((row = mysql_fetch_row(res))){
					hostList[row[0]] = row[1];
				}
			}else{
				// Nothing in the database.  
				mysql_free_result(res);
				mysql_close(init);
				return 1;
			}
			
			mysql_free_result(res);
			// Just to make sure...  
			mysql_close(init);
			return 1;
		}
	}
	return 0;
}

bool Auth::checkHost(string hostid, string pass){
	if(hostList.size() <= 0) return 0;
	if(hostList[hostid] == pass) return 1;
	return 0;
}
