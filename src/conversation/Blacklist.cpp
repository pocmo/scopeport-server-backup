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

#include "Blacklist.h"
#include "../internal.h"
#include "../database/Database.h"

Blacklist::Blacklist(mySQLData myDBData)
			: Database(myDBData){
	dbData = myDBData;
}

bool Blacklist::checkHost(string host){
	// Initialize database connection.
	if(initConnection()){
		MYSQL* init = getHandle();
		stringstream query;
		query	<< "SELECT id FROM blacklisted_hosts WHERE host = '"
				<< host
				<< "'";
		if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) != 0){
			// Query was not successful.
			mysql_close(init);
			return 0;
		}else{
			// Query successful.
			MYSQL_RES* res = mysql_store_result(init);
			if(mysql_num_rows(res) > 0){
				// IP was found.
				mysql_free_result(res);
				mysql_close(init);
				return 0;
			}
			// IP was not found.
			mysql_free_result(res);
			mysql_close(init);
			return 1;
		}
	}
	return 0;
}

bool Blacklist::blackHost(string host){
	// Initialize database connection.
	if(initConnection()){
		MYSQL* init = getHandle();
		stringstream query;
		query	<< "INSERT INTO blacklisted_hosts(host,created_at) VALUES('"
				<< host
				<< "', NOW())";
		if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) != 0){
			// Query was not successful.
			mysql_close(init);
			return 0;
		}
		mysql_close(init);
		return 1;
	}
	return 0;
}
