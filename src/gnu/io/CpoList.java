/*-------------------------------------------------------------------------
|   rxtx is a native interface to serial ports in java.
|   Copyright 1997, 1998, 1999 by Trent Jarvi trentjarvi@yahoo.com
|
|   This library is free software; you can redistribute it and/or
|   modify it under the terms of the GNU Library General Public
|   License as published by the Free Software Foundation; either
|   version 2 of the License, or (at your option) any later version.
|
|   This library is distributed in the hope that it will be useful,
|   but WITHOUT ANY WARRANTY; without even the implied warranty of
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|   Library General Public License for more details.
|
|   You should have received a copy of the GNU Library General Public
|   License along with this library; if not, write to the Free
|   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--------------------------------------------------------------------------*/
package gnu.io;

import javax.comm.*;
//import  java.io.*;
//import  java.util.*;


class CpoList {
/*------------------------------------------------------------------------------
        add()
        accept:
        perform:
        return:
        exceptions:
        comments:
------------------------------------------------------------------------------*/
	synchronized void add(CommPortOwnershipListener c) { }
/*------------------------------------------------------------------------------
        remove()
        accept:
        perform:
        return:
        exceptions:
        comments:
------------------------------------------------------------------------------*/
	synchronized void remove(CommPortOwnershipListener c) { }
/*------------------------------------------------------------------------------
        clonelist()
        accept:
        perform:
        return:
        exceptions:
        comments:
------------------------------------------------------------------------------*/
	synchronized CpoList clonelist() { return(this); }
/*------------------------------------------------------------------------------
        isEmpty()
        accept:
        perform:
        return:
        exceptions:
        comments:
------------------------------------------------------------------------------*/
	synchronized boolean isEmpty() {  return(false);}
/*------------------------------------------------------------------------------
        fireOwnershipEvent()
        accept:
        perform:
        return:
        exceptions:
        comments:
------------------------------------------------------------------------------*/
	synchronized void fireOwnershipEvent(int i) { }
/*------------------------------------------------------------------------------
        dump()
        accept:
        perform:
        return:
        exceptions:
        comments:
------------------------------------------------------------------------------*/
	synchronized void dump() { }
}
