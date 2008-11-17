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

//! Measure response times

#ifndef TIMER_H_
#define TIMER_H_

#include "../internal.h"

class Timer	{
	private:
		//! This holds the timer information.
		timeval stimeval;
		
		//! This fills the timeval variable.
		/*!
		 * \sa stimeval
		 */
		const timeval& getTime() const;
	public:
		//! Start the timer.
		/*!
		 * \sa stopTimer()
		 */
		void startTimer();
		
		//! Stop the timer and get time between now and start of timer.
		/*!
		 * \sa startTimer()
		 */
		unsigned int stopTimer();
};

#endif /*TIMER_H_*/
