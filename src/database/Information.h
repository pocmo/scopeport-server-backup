// This file is part of ScopePort (Linux server).
//
// Copyright 2008, 2009 Lennart Koopmann
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

//! Static methods that generate queries.
/*!
 * Static methods that return queries based on input data.
 */

#ifndef INFORMATION_H_
#define INFORMATION_H_

#include "../internal.h"
#include "Database.h"

class Information{
	public:
		//! Returns query to get the name of a sensor with given sensor type.
		/*!
		 * \param st Sensor type ID
		 * \return Query
		 */
		static string getSensorName(string st);

		//! Returns the query to get the name of a host with given host ID
		/*!
		 * \param hostid Host ID
		 * \return Query
		 */
		static string getHostName(string hostid);

		//! Returns the query to get the sensor condition of a given sensor on a host.
		/*!
		 * \param hostid Host ID
		 * \param st Sensor type ID
		 * \return Query
		 */
		static string getSensorCondition(string hostid, string st);

		//! Returns query to get the notification group that needs to be informed for warnings about given host and sensor type.
		/*!
		 * \param hostid Host ID.
		 * \param st Sensor type ID.
		 * \return Query
		 */
		static string getReceiverGroup(string hostid, string st);

		//! Returns query to get the notification group that needs to be informed for warnings about given service.
		/*!
		 * \param serviceID The service ID
		 * \return Query
		 */
		static string getServiceReceiverGroup(unsigned int serviceID);

		//! Returns query to get the name of service with given ID.
		/*!
		 * \param serviceID The service ID
		 * \return Query
		 */
		static string getServiceName(unsigned int serviceID);

		//! Returns query to get the timestamp of when the last notification for a sensor was sent.
		/*!
		 * \param hostid Host ID
		 * \param st Sensor type ID
		 * \return Query
		 */
		static string getLastWarn(string hostid, string st);

		//! Returns query to get the timestamp of when the last notification for a service was sent.
		/*!
		 * \param serviceID the service ID
		 * \return Query
		 */
		static string getLastServiceWarn(unsigned int serviceID);

		//! Returns query to set the timestamp of when the last notification for a sensor was sent.
		/*!
		 * \param lastwarn Timestamp to set
		 * \param hostid Host ID
		 * \param st Sensor type ID
		 * \return Query
		 */
		static string setLastWarn(time_t lastwarn, string hostid, string st);

		//! Returns query to set the timestamp of when the last notification for a service was sent.
		/*!
		 * \param lastwarn Timestamp to set
		 * \param serviceID Service ID
		 * \return Query
		 */
		static string setLastServiceWarn(time_t lastwarn, unsigned int serviceID);

		//! Returns query to get the defined severity of a sensor.
		/*!
		 * \param hostid Host ID
		 * \param st Sensor type ID
		 * \return Query
		 */
		static string getSensorSeverity(string hostid, string st);

		//! Returns vector of all mail addresses in a notification group.
		/*!
		 * \param init MySQL handle
		 * \param groupID ID of notification group.
		 * \param sevBorder Severity level of this alert. Used for selection of receivers because Receivers can have a minimum severity level.
		 * \return vector of email adresses.
		 */
		static vector<string> getMailWarningReceivers(MYSQL* init, string groupID, string sevBorder);

		//! Returns vector of all JIDs in a notification group.
		/*!
		 * \param init MySQL handle
		 * \param groupID ID of notification group.
		 * \param sevBorder Severity level of this alert. Used for selection of receivers because Receivers can have a minimum severity level.
		 * \return vector of JIDs.
		 */
		static vector<string> getXMPPWarningReceivers(MYSQL* init, string groupID, string sevBorder);

		//! Returns vector of all mobile phone numbers that use the Clickatell API in a notification group.
		/*!
		 * \param init MySQL handle
		 * \param groupID ID of notification group.
		 * \param sevBorder Severity level of this alert. Used for selection of receivers because Receivers can have a minimum severity level.
		 * \return vector of phone numbers in international format.
		 */
		static vector<string> getMobileCWarningReceivers(MYSQL* init, string groupID, string sevBorder);

		//! Returns query to delete all health data of this node from database.
		static string clearHealth(unsigned int nodeID);

		//! Returns query to update health data of given PID in database.
		/*!
     * \param nodeID The ID of this node
		 * \param pid The PID. Used as primary key.
		 * \param clienthandler Bool. Is this process a clientHandler?
		 * \param vmem This process' size of virtual memory
		 * \param threads This process' number of threads
		 * \param packetsOK The number of packages this process successful received.
		 * \param packetsERR The number of packages this process refused or failed to handle.
		 * \param dbTotalSize Total size of database (Megabyte)
		 * \param dbSensorSize Size of "sensorvalues" table (Megabyte)
		 * \param dbServiceSize Size of "servicerecords" table (Megabyte)
		 * \sa clearHealth()
		 * \return Query
		 */
		static string updateHealth(unsigned int nodeID, string pid, bool clienthandler, string vmem, string threads,
				int packetsOK, int packetsERR, double dbTotalSize, double dbSensorSize,
				double dbServiceSize);

		//! Returs information about emergency.
		/*!
		 * \param emergencyID ID of emergency
		 * \param type  0: Description, 1: Timestamp of creation, 2: Severity (numeric)
		 * \return Query
		 */
		static string getEmergencyInformation(int emergencyID, int type);

		//! Set the status of an emergency notification receiver.
		/*!
		 * \param ID ID of receiver in queue table.
		 * \param status Status to set (0: not yet sent, 1: sent, -1: error)
		 * \return Query
		 */
		static string setReceiverMark(int ID, int status);
};

#endif /*INFORMATION_H_*/
