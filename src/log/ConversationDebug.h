// This file is part of ScopePort (Linux server).
//
// Copyright 2009 Lennart Koopmann
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

#ifndef CONVERSATIONLOG_H_
#define CONVERSATIONLOG_H_

#include "../internal.h"
#include "../database/Database.h"

class ConversationDebug : public Database
{
  private:
    unsigned int conversationID;
    string method;
    string remoteHost;
    bool disabled;

    void create();
    bool checkIfDisabledForThisMethod();
  public:
    ConversationDebug(mySQLData myDBData, string myMethod, string myRemoteHost);

    void log(unsigned int direction, string message);
};
#endif                                            /*LOG_H_*/
