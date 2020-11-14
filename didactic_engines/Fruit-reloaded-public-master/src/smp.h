/* Fruit reloaded, a UCI chess playing engine derived from Fruit 2.1
 *
 * Copyright (C) 2012-2014 Daniel Mehrmann
 * Copyright (C) 2013-2014 Ryan Benitez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// smp.h

#ifndef SMP_H
#define SMP_H

// constants

const int ThreadMax = 16;
const int SplitMax = 8;
const int SlaveMax = 4;

// functions

extern void smp_init     ();
extern void smp_close    ();
extern void smp_wakeup   ();
extern void smp_sleep    ();

extern int  thread_number();

extern bool thread_is_ok (int id); // HACK: thread.h should be used

#endif // !defined SMP_H

// end of smp.h
