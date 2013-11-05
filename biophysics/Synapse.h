/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _SYNAPSE_H
#define _SYNAPSE_H

class Synapse
{
	public:
		Synapse();
		void setWeight( double v );
		void setDelay( double v );

		double getWeight() const;
		double getDelay() const;

		void setBuffer( SpikeRingBuffer* buf );

		void addSpike( double time );
		static void addMsgCallback( 
					const Eref& e, const string& finfoName, 
					ObjId msg, unsigned int msgLookup );
		static void dropMsgCallback( 
					const Eref& e, const string& finfoName, 
					ObjId msg, unsigned int msgLookup );
		static const Cinfo* initCinfo();
	private:
		double weight_;
		double delay_;
		SpikeRingBuffer* buffer_;
};

#endif // _SYNAPSE_H
