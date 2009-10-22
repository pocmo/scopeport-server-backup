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

#include "Timer.h"

void Timer::startTimer()
{
  m_timeval = get_tv ();
}


unsigned int Timer::stopTimer() const
{
  timeval timeval = get_tv ();
  int returnage = ( timeval.tv_sec - m_timeval.tv_sec ) * 1000 + (( timeval.tv_usec - m_timeval.tv_usec ) / 1000);

  // Round the time up to one millisecond if it is zero.
  if(returnage == 0)
    returnage = 1;

  return returnage;
}


const timeval& Timer::get_tv() const
{
  static timeval tv;
  static struct timezone tz;
  gettimeofday ( &tv, &tz );
  return tv;
}
