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

#include "ReadData.h"
#include "../internal.h"

bool ReadData::inspectStream(string clientput){
	
	int i = 0;
	string token;
	if(clientput.length() > 0){
		// Split the received data and save it to correct variables.  
	 	istringstream stream(clientput);
		while(getline(stream, token, ';')){
			switch (i){
		    	case 0:
		    		host = token;
		    		break;
		    	case 1:
		    		pass = token;
		    		break;
		    	case 2:
		    		st = token;
		    		break;
		    	case 3:
		    		sv = token;
		    		break;
		    }
		    i++;
		 }
		
		// Create a timestamp.  
		time_t rawtime;
		time(&rawtime);	
		stringstream rawtimestamp;
		rawtimestamp << rawtime;
		timestamp = rawtimestamp.str().c_str();
		
	  	// Check for validity of package (sets errors to true if any error occured).  
	  	if(numOnly(host) && numOnly(st) && !timestamp.empty()
	  		&& !host.empty() && !st.empty() && !pass.empty() && !sv.empty()){
	  		return 1;
		}else{
			return 0;
		}
	}
	
  	return 0;
}
