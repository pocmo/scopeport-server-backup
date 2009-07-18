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

#include "Database.h"
#include "../internal.h"
#include "../log/Log.h"

Database::Database(mySQLData sqlData){
	mysql_host = sqlData.host;
	mysql_user = sqlData.user;
	mysql_password = sqlData.pass;
	mysql_database = sqlData.db;
	mysql_port = sqlData.port;
}

bool Database::initConnection(){
	init = mysql_init(NULL);
	if(init == NULL)
		return 0;

	// Connect to mysql database.
	if(mysql_real_connect(	init,
							mysql_host,
							mysql_user,
							mysql_password,
							mysql_database,
							mysql_port,
							NULL,0) == NULL) {
		// No connection was established.
		return 0;
	}else{
		// Successfully connected to mysql database.
		return 1;
	}
	return 0;
}

bool Database::checkTables(){

	/*
	 *
	 * Define needed tables here. Everything else is
	 * done automatically!
	 *
	 * Example:
	 *  tables[] = "foo:bar1.bar2.bar3";
	 *   -- Table "foo" must have the following columns: bar1, bar2, bar3
	 *
	 */

  // TODO: bring this back alive
//	tables.push_back("hosts:ID.hostid.password.hostname.linux_kernelversion.domainname.hda_model.hdb_model.hdc_model.hdd_model.total_memory.total_swap.cpu_vendor.cpu_modelname.cpu_mhz.cpu_cachesize.swaps.disabled");
//	tables.push_back("sensordata:timestamp.host.st.sv");
//	tables.push_back("sensors:ID.st.name.description.standard_condition_type.standard_condition_value.overview.needscondition");
//	tables.push_back("sensor_conditions:ID.hostid.st.type.value.disabled");
//	tables.push_back("users:ID.username.password.deleted.name");

	int j;
	int k;
	int l;
	string thistable;
	string token;

	for(int i = 0;i < getTableCount();i++){
		j = 0;
		istringstream table(tables[i]);
		while(getline(table, token, ':')){
			if(j == 0) thistable = token;
			if(j == 1){
				// Check how many columns we have.
				istringstream testcolumns(token);
				k = 0;
				string testtoken;
				while(getline(testcolumns, testtoken, '.')){
					k++;
				}

				istringstream columns(token);
				string column;
				stringstream query;
				query << "SELECT ";
				l = 0;
				while(getline(columns, token, '.')){
					l++;
					query << token;
					// Don't add a ',' to the last column.
					if(l != k) query << ",";
				}
				query << " FROM " << thistable << " LIMIT 1";

				// Execute query.
				if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) != 0){
					// Query was not successful.
					error = mysql_error(init);
					missingcol = thistable;
					return 0;
				}
				mysql_close(init);
				initConnection();
			}
			j++;
		}
	}
	return 1;
}

bool Database::setQuery(MYSQL* init, string query){
	if(query.empty())
		return "NULL";

	if(mysql_real_query(init, query.c_str(), strlen(query.c_str())) == 0){
		// Query successful.
		return 1;
	}

	// Query failed.

	error = mysql_error(init);

	return 0;
}

string Database::sGetQuery(string query){
	if(query.empty())
		return "NULL";

	if(mysql_real_query(init, query.c_str(), strlen(query.c_str())) == 0){
		// Query successful.
		MYSQL_RES* res = mysql_store_result(init);
		MYSQL_ROW row;
		row = mysql_fetch_row(res);
		stringstream result;
		if(mysql_num_rows(res) > 0){
			result << row[0];
		}else{
			mysql_free_result(res);
			return "NULL";
		}
		mysql_free_result(res);
		return result.str();
	}
	return "NULL";
}

unsigned int Database::getNumOfResults(string query){
	if(query.empty())
		return 0;

	if(mysql_real_query(init, query.c_str(), strlen(query.c_str())) == 0){
		// Query successful.
		MYSQL_RES* res = mysql_store_result(init);
		unsigned int num = 0;
    if((num = mysql_num_rows(res)) <= 0){
      num = 0;
    }
		mysql_free_result(res);
		return num;
	}
	return 0;
}

string Database::sGetQuery2(string query){
	if(query.empty())
		return "NULL";
	if(mysql_real_query(init, query.c_str(), strlen(query.c_str())) == 0){
		// Query successful.
		MYSQL_RES* res = mysql_store_result(init);
		MYSQL_ROW row;
		row = mysql_fetch_row(res);
		stringstream result;
		if(mysql_num_rows(res) > 0){
			result	<< row[0]
					<< " "
					<< row[1];
		}else{
			mysql_free_result(res);
			return "NULL";
		}
		mysql_free_result(res);
		return result.str();
	}
	return "NULL";
}

bool Database::clearSensorData(){
	const char* query = "DELETE FROM sensorvalues WHERE TIMEDIFF(CURRENT_TIMESTAMP, created_at) < '730:00:00'";

	if(mysql_real_query(init, query, strlen(query)) == 0){
		// Query successful.
		return 1;
	}

	// Query failed.
	return 0;
}

bool Database::clearServiceData(){
	unsigned int yesterday = time(NULL)-86400;

	stringstream query;
	query	<< "DELETE FROM servicerecords WHERE timestamp < ";
	query	<< yesterday;

	if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) == 0){
		// Query successful.
		return 1;
	}

	// Query failed.
	return 0;
}

string Database::escapeString(string escapeMe){
  stringstream result;
  char query[escapeMe.length()*2+1];
  mysql_escape_string(query, escapeMe.c_str(), escapeMe.length());
  result << query;
  return result.str();
}
