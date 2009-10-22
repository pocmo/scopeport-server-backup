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

//! Holds sensor conditions and can check if sensor values are in range

#ifndef WARNING_H_
#define WARNING_H_

#include "../internal.h"
#include "../database/Database.h"

class Warning : public Database
{
  private:
    //! This map holds all sensor conditions.
    /*!
     * \sa getConditions()
     */
    map<int,string> conditionList;

    //! Holds the last error message.
    /*!
     * \sa getError()
     */
    string error;

    //! Holds login and connection information of MySQL database.
    mySQLData dbData;

  public:
    Warning(mySQLData myDBData);

    //!  Fetches sensor conditions and fills conditionList.
    /*!
     * \return False in case of error, true if everything went fine.
     * \param hostid1
     * \param st1
     * \sa conditionList
     */
    bool getConditions(string hostid, string st);

    //! Checks if a sensor is in range and if a notification needs to be sent.
    /*!
     * \param hostid Host ID
     * \param st Sensor type ID
     * \param checkSv Sensor value
     * \param lastWarn Timestamp of last warning
     * \return  -1: something failed, 0: sensor value is not in condition range, 1: Sensor is okay
     */
    int checkSensor(string hostid, string st, string checkSv, string lastWarn);

    //! Inserts alarm into alarms table.
    /*!
     * \param checkHostID Host ID
     * \param checkSt Sensor type ID
     * \param checkSv Sensor value
     */
    void updateAlarms(string checkHostID, string checkSt, string checkSv);

    //! Returns last error.
    /*!
     * \sa error
     */
    string getError(){ return error; }
};
#endif                                            /*WARNING_H_*/
