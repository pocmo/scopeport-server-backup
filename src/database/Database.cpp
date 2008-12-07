// This file is part of ScopePort (Linux server).
//
// Copyright 2007, 2008 Lennart Koopmann
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
		unsigned int num = mysql_num_rows(res);
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
	unsigned int lastmonth = time(NULL)-2678400;

	stringstream query;
	query	<< "DELETE FROM sensordata WHERE timestamp < ";
	query	<< lastmonth;

	if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) == 0){
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


bool Database::saveStream(string timestamp, string host, string st, string sv){
	if(timestamp.empty() || host.empty() || st.empty() || sv.empty())
		return "NULL";

	bool doDelQuery = 0;

	stringstream query, query2, delquery;

	if(timestamp.empty() || host.empty() || st.empty() || sv.empty()){
		error = "Missing information.";
		return 0;
	}

	// Check first digit of sensor type to find out if we have a profile sensor package.
	if(st.at(0) != '0' || st == "00"){
		// We have a sensor package.
		query	<< "INSERT INTO sensordata (timestamp,host,st,sv) VALUES ('"
				<< timestamp
				<< "','"
				<< host
				<< "','"
				<< st
				<< "','"
				<< sv
				<< "')";

		query2	<< "INSERT INTO lastsensordata (timestamp,host,st,sv) VALUES ('"
				<< timestamp
				<< "','"
				<< host
				<< "','"
				<< st
				<< "','"
				<< sv
				<< "')";

		delquery	<< "DELETE FROM lastsensordata"
					<< " WHERE host = '" << host
					<< "' AND st = '" << st << "';";

		doDelQuery = 1;

	}else{
		// We have a profile sensor package.
		query << "UPDATE hosts SET ";

		if(	st == "01" || st == "02" || st == "03" || st == "04" ||
			st == "05" || st == "06" || st == "07" || st == "08" ||
			st == "09" || st == "010" || st == "011" || st == "012" ||
			st == "013" || st == "014" || st == "016" || st == "017"){
				// Hostname.
				if(st == "01")
					query << "hostname = '";
				// Kernel version.
				if(st == "02")
					query << "linux_kernelversion = '";
				// Domain name.
				if(st == "03")
					query << "domainname = '";
				// HDA model name.
				if(st == "04")
					query << "hda_model = '";
				// HDB model name.
				if(st == "05")
					query << "hdb_model = '";
				// HDC model name.
				if(st == "06")
					query << "hdc_model = '";
				// HDD model name.
				if(st == "07")
					query << "hdd_model = '";
				// Total memory.
				if(st == "08")
					query << "total_memory = '";
				// Total swap.
				if(st == "09")
					query << "total_swap = '";
				// CPU vendor.
				if(st == "010")
					query << "cpu_vendor = '";
				// CPU model name.
				if(st == "011")
					query << "cpu_modelname = '";
				// CPU Mhz.
				if(st == "012")
					query << "cpu_mhz = '";
				// CPU cache size.
				if(st == "013")
					query << "cpu_cachesize = '";
				// Swap partitions.
				if(st == "014")
					query << "swaps = '";
				// Client version.
				if(st == "016")
					query << "clientversion = '";
				if(st == "017")
					query << "scsi = '";

				query	<< noSpaces(sv)
						<< "' WHERE hostid = '"
						<< noSpaces(host)
						<< "'";
			}else{
				// Sensor ID out of range.
				return 1;
			}
	}

	// Query and check if query was successful.
	if(mysql_real_query(init, query.str().c_str(), strlen(query.str().c_str())) != 0){
		error = mysql_error(init);
		return 0;
	}

	if(doDelQuery){
		// Delete old lastsensordata of this host/sensor and check if query was successful.
		if(mysql_real_query(init, delquery.str().c_str(), strlen(delquery.str().c_str())) != 0){
			error = mysql_error(init);
			return 0;
		}else{
			// Deletion was sucessful, insert current data.
			if(mysql_real_query(init, query2.str().c_str(), strlen(query2.str().c_str())) != 0){
				error = mysql_error(init);
				return 0;
			}
		}
	}

	return 1;
}
