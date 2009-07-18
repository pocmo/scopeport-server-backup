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

//!  MySQL database connection and handling
/*!
 * Connects to database and provides some general database methods for queries and table structure checks.
*/

#ifndef DATABASE_H_
#define DATABASE_H_

#include "../internal.h"

class Database {
	private:
		//! Holds all names of tables that are checked in checkTables().
		/*!
		 * \sa checkTables()
		 */
		vector<string>	tables;

		//! The MySQL handle.
		MYSQL			*init;

		//! Holds the last error message.
		string			error;

		//! Holds the name of a missing table if found by checkTables().
		/*!
		 * \sa checkTables()
		 */
		string			missingcol;

		//! The host the MySQL database is running on.
		const char*		mysql_host;

		//! The user we want to connect with to the MySQL database.
		const char*		mysql_user;

		//! The MySQL user's password.
		const char*		mysql_password;

		//! The ScopePort database on the MySQL host.
		const char*		mysql_database;

		//! The port we want to use for MySQL connection. (0 means standard port)
		int				mysql_port;
	public:
		//! Constructor. Converts sqlData values to single variables.
		Database(mySQLData sqlData);

		//! Checks if tables defined in vector<string> tables exist.
		/*!
		 * \sa checkTables()
		 * \sa tables
		 * \return False if not all tables were found or another error occured, true if all tables were found.
		 */
		bool	checkTables();

		//! Opens connection to MySQL server
		/*!
		 * \return False if connection could not be established or creating the handle failed, true if connected to database.
		 */
		bool	initConnection();

		//! Performs query and does not give back any data. Useful for INSERT, UPDATE or DELETE queries.
		/*!
		 * \param init A MySQL handle.
		 * \param query The query string.
		 * \return True if the query has been executed successfully or false if an error occured.
		 */
		bool	setQuery(MYSQL* init, string query);

		//! Performs query and gives back result string of first requested field.
		/*
		 * \param query The query string.
		 * \return "NULL" if an error occured (or nothing was fetched) or the result string.
		 */
		string	sGetQuery(string query);

		//! Performs query and gives back number of fetched rows.
		/*
		 * \param query The query string.
		 * \return The number of fetched rows.
		 */
		unsigned int getNumOfResults(string query);

		//! Performs query and gives back result string of first and second requested field separated by a whitespace.
		/*
		 * \param query The query string.
		 * \return "NULL" if an error occured (or nothing was fetched) or the result string.
		 */
		string	sGetQuery2(string query);

		//! Deletes data from table "sensordata" that is older than 31 days.
		/*
		 * \return False in case of error, true if everything went fine.
		 */
		bool	clearSensorData();

		//! Deletes data from table "servicedata" that is older than 24 hours.
		/*
		 * \return False in case of error, true if everything went fine.
		 */
		bool	clearServiceData();

		//! Returns the MySQL handle.
		/*!
		 * \sa init
		 * \sa initConnection()
		 */
		MYSQL*	getHandle() { return init; }

		//! Returns the last error message.
		/*!
		 * \sa error
		 */
		string	getError() { return error; }

		//! Returns the missing table reported by checkTables()
		/*!
		 * \sa checkTables()
		 */
		string	getMissingTable() { return missingcol; }

		//! Returns the number of tables that should be checked.
		/*!
		 * \sa checkTables()
		 */
		int		getTableCount() { return tables.size(); }

    static string escapeString(string escapeMe);
};

#endif /*DATABASE_H_*/
