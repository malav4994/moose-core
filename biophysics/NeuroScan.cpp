/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**   copyright (C) 2003-2007 Upinder S. Bhalla, Niraj Dudani and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "moose.h"
#include "../element/Neutral.h"
#include "SpikeGen.h"
#include <queue>
#include "SynInfo.h"
#include "HSolveStruct.h"
#include "SynChan.h"
#include "NeuroHub.h"
#include "NeuroScanBase.h"
#include "NeuroScan.h"

#include "HHChannel.h"

const Cinfo* initNeuroScanCinfo()
{
	// Shared message to NeuroHub
	static Finfo* hubShared[] =
	{
		new SrcFinfo( "compartment",
			Ftype1< vector< Element* >* >::global() ),
		new SrcFinfo( "channel",
			Ftype1< vector< Element* >* >::global() ),
		new SrcFinfo( "spikegen",
			Ftype1< vector< Element* >* >::global() ),
		new SrcFinfo( "synchan",
			Ftype1< vector< Element* >* >::global() ),
	};
	
	static Finfo* gateShared[] =
	{
		new SrcFinfo( "Vm",
			Ftype1< double >::global() ),
		new DestFinfo( "gate",
			Ftype2< double, double >::global(),
			RFCAST( &NeuroScan::gateFunc ) ),
	};
	
	static Finfo* neuroScanFinfos[] = 
	{
	//////////////////////////////////////////////////////////////////
	// Field definitions
	//////////////////////////////////////////////////////////////////
		new ValueFinfo( "NDiv", ValueFtype1< int >::global(),
			GFCAST( &NeuroScan::getNDiv ),
			RFCAST( &NeuroScan::setNDiv )
		),
		new ValueFinfo( "VLo", ValueFtype1< double >::global(),
			GFCAST( &NeuroScan::getVLo ),
			RFCAST( &NeuroScan::setVLo )
		),
		new ValueFinfo( "VHi", ValueFtype1< double >::global(),
			GFCAST( &NeuroScan::getVHi ),
			RFCAST( &NeuroScan::setVHi )
		),
		
	//////////////////////////////////////////////////////////////////
	// SharedFinfo definitions
	//////////////////////////////////////////////////////////////////
		new SharedFinfo( "hub", hubShared,
			sizeof( hubShared ) / sizeof( Finfo* ) ),
		new SharedFinfo( "gate", gateShared,
			sizeof( gateShared ) / sizeof( Finfo* ) ),
	//////////////////////////////////////////////////////////////////
	// SrcFinfo definitions
	//////////////////////////////////////////////////////////////////
	
	//////////////////////////////////////////////////////////////////
	// DestFinfo definitions
	//////////////////////////////////////////////////////////////////
		new DestFinfo( "hubCreate",
			Ftype0::global(),
			&NeuroScan::hubCreateFunc ),
		new DestFinfo( "readModel",
			Ftype2< Element*, double >::global(),
			RFCAST( &NeuroScan::readModelFunc ) ),
	};

	static Cinfo neuroScanCinfo(
		"NeuroScan",
		"Niraj Dudani, 2007, NCBS",
		"NeuroScan: Portal for reading in neuronal models from MOOSE object tree.",
		initNeutralCinfo(),
		neuroScanFinfos,
		sizeof( neuroScanFinfos ) / sizeof( Finfo* ),
		ValueFtype1< NeuroScan >::global()
	);

	return &neuroScanCinfo;
}

static const Cinfo* neuroScanCinfo = initNeuroScanCinfo();

static const Slot hubCompartmentSlot =
	initNeuroScanCinfo()->getSlot( "hub.compartment" );
static const Slot hubChannelSlot =
	initNeuroScanCinfo()->getSlot( "hub.channel" );
static const Slot hubSpikegenSlot =
	initNeuroScanCinfo()->getSlot( "hub.spikegen" );
static const Slot hubSynchanSlot =
	initNeuroScanCinfo()->getSlot( "hub.synchan" );
static const Slot gateVmSlot =
	initNeuroScanCinfo()->getSlot( "gate.Vm" );
static const Slot gateSlot =
	initNeuroScanCinfo()->getSlot( "gate" );

static const Finfo* gateFinfo =
	initNeuroScanCinfo()->findFinfo( "gate" );

///////////////////////////////////////////////////
// Field function definitions
///////////////////////////////////////////////////

void NeuroScan::setNDiv( const Conn* c, int NDiv )
{
	static_cast< NeuroScan* >( c->data() )->NDiv_ = NDiv;
}

int NeuroScan::getNDiv( const Element* e )
{
	return static_cast< NeuroScan* >( e->data() )->NDiv_;
}

void NeuroScan::setVLo( const Conn* c, double VLo )
{
	static_cast< NeuroScan* >( c->data() )->VLo_ = VLo;
}

double NeuroScan::getVLo( const Element* e )
{
	return static_cast< NeuroScan* >( e->data() )->VLo_;
}

void NeuroScan::setVHi( const Conn* c, double VHi )
{
	static_cast< NeuroScan* >( c->data() )->VHi_ = VHi;
}

double NeuroScan::getVHi( const Element* e )
{
	return static_cast< NeuroScan* >( e->data() )->VHi_;
}

///////////////////////////////////////////////////
// Dest function definitions
///////////////////////////////////////////////////

/// Creating hub as solver's child.
void NeuroScan::hubCreateFunc( const Conn* c )
{
	static_cast< NeuroScan* >( c->data() )->
		innerHubCreateFunc( c->targetElement() );
}

void NeuroScan::innerHubCreateFunc( Element* scan )
{
	// Hub element's data field is owned by its parent HSolve
	// structure, so we set it's noDelFlag to 1.
	Id solve = Neutral::getParent( scan );
	Element* hub = initNeuroHubCinfo()->create( 
		Id::scratchId(), "hub",
		static_cast< void* >( &hub_ ), 1 );
	bool ret = solve()->findFinfo( "childSrc" )->
		add( solve(), hub, hub->findFinfo( "child" ) );
	assert( ret );
	
	// Setting up shared msg between scanner and hub.
	ret = scan->findFinfo( "hub" )->
		add( scan, hub, hub->findFinfo( "hub" ) );
	assert( ret );
}

void NeuroScan::readModelFunc( const Conn* c, Element* seed, double dt  )
{
	static_cast< NeuroScan* >( c->data() )->
		innerReadModelFunc( c->targetElement(), seed, dt );
}

void NeuroScan::innerReadModelFunc( Element* e, Element* seed, double dt  )
{
	scanElm_ = e;
	
	unsigned int seedLocal = logElement( seed, COMPARTMENT );
	initialize( seedLocal, dt );
	
	vector< unsigned int >::iterator i;
	vector< Element* > elist;
	for ( i = compartment_.begin(); i != compartment_.end(); ++i )
		elist.push_back( id2e_[ *i ] );
	send1< const vector< Element* >* >(
		scanElm_, hubCompartmentSlot, &elist );
	
	elist.clear();
	for ( i = channel_.begin(); i != channel_.end(); ++i )
		elist.push_back( id2e_[ *i ] );
	send1< const vector< Element* >* >(
		scanElm_, hubChannelSlot, &elist );
	
	elist.clear();
	vector< SpikeGenStruct >::iterator j;
	for ( j = spikegen_.begin(); j != spikegen_.end(); ++j )
		elist.push_back( j->elm_ );
	send1< const vector< Element* >* >(
		scanElm_, hubSpikegenSlot, &elist );
	
	elist.clear();
	vector< SynChanStruct >::iterator k;
	for ( k = synchan_.begin(); k != synchan_.end(); ++k )
		elist.push_back( k->elm_ );
	send1< const vector< Element* >* >(
		scanElm_, hubSynchanSlot, &elist );
}

void NeuroScan::gateFunc( const Conn* c, double A, double B )
{
	NeuroScan* ns = static_cast< NeuroScan* >( c->data() );
	ns->A_ = A;
	ns->B_ = B;
}

///////////////////////////////////////////////////
// Portal functions (to scan model)
///////////////////////////////////////////////////

vector< unsigned int > NeuroScan::children(
	unsigned int self, unsigned int parent )
{
	vector< unsigned int > child = neighbours( self );
	child.erase(
		remove( child.begin(), child.end(), parent ),
		child.end()
	);
	return child;
}

vector< unsigned int > NeuroScan::neighbours( unsigned int compartment )
{
	vector< Element* > neighbour;
	targets( compartment, "axial", neighbour );
	targets( compartment, "raxial", neighbour );
	return logElement( neighbour, COMPARTMENT );
}

vector< unsigned int > NeuroScan::channels( unsigned int compartment )
{
	vector< Element* > channel;
	targets( compartment, "channel", channel );
	return logElement( channel, CHANNEL );
}

vector< unsigned int > NeuroScan::gates( unsigned int channel )
{
	vector< Element* > gate;
	currentChanId_ = channel;
	targets( channel, "xGate", gate );
	targets( channel, "yGate", gate );
	targets( channel, "zGate", gate );
	return logElement( gate, GATE );
}

unsigned int NeuroScan::presyn( unsigned int compartment )
{
	vector< Element* > spikegen;
	targets( compartment, "VmSrc", spikegen );
	if ( spikegen.size() > 0 ) {
		Conn c( spikegen[ 0 ], 0 );
		ProcInfoBase p;
		SpikeGen::reinitFunc( &c, &p );
		return logElement( spikegen, SPIKEGEN )[ 0 ];
	}
	else
		return 0;
}

vector< unsigned int > NeuroScan::postsyn( unsigned int compartment )
{
	vector< Element* > synchan;
	targets( compartment, "channel", synchan );
	ProcInfoBase p;
	p.dt_ = dt_;
	vector< unsigned int > ret = logElement( synchan, SYNCHAN );
	vector< unsigned int >::iterator isyn;
	Element* syn;
	for ( isyn = ret.begin(); isyn != ret.end(); ++isyn ) {
		syn = id2e_[ *isyn ];
		Conn c( syn, 0 );
		SynChan::reinitFunc( &c, &p );
	}
	return ret;
}

void NeuroScan::field(
	unsigned int object,
	string field,
	double& value )
{
	if ( eclass_[ object ] == GATE ) {
		const GateInfo& gi = gateInfo_[ object ];
		object = gi.chanId;
		
		if ( field == "power" ) {
			string chanField[] = { "Xpower", "Ypower", "Zpower" };
			field = chanField[ gi.xIndex ];
		}
		else if ( field == "state" ) {
			string chanField[] = { "X", "Y", "Z" };
			field = chanField[ gi.xIndex ];
		}
	}
	
	get< double >( id2e_[ object ], field, value );
}

void NeuroScan::synchanFields(
	unsigned int synchan,
	SynChanStruct& scs )
{
	Element* e = id2e_[ synchan ];
	Conn c( e, 0 );
	ProcInfoBase p;
	p.dt_ = dt_;
	
	SynChan::reinitFunc( &c, &p );
	set< SynChanStruct* >( e, "scan", &scs );
}

void NeuroScan::rates(
	unsigned int gate,
	double Vm, double& A, double& B )
{
	unsigned int connIndex =
		scanElm_->connSrcBegin( gateSlot.msg() ) -
		scanElm_->lookupConn( 0 ) +
		gateInfo_[ gate ].rIndex;
	
	sendTo1< double >(
		scanElm_, gateVmSlot,
		connIndex, Vm );
	A = A_;
	B = B_;
}

Element* NeuroScan::elm( unsigned int id )
{
	return id2e_[ id ];
}
///////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////

vector< unsigned int > NeuroScan::logElement(
	const vector< Element* >& el, EClass eclass )
{
	vector< unsigned int > id;
	for ( unsigned int i = 0; i < el.size(); ++i ) {
		if ( eclass == GATE )
			currentXIndex_ = i;
		if ( eclass == SYNCHAN && type( el[ i ] ) != SYNCHAN )
			continue;
		if ( eclass == CHANNEL && type( el[ i ] ) != CHANNEL )
			continue;
		id.push_back(
			logElement( el[ i ], eclass )
		);
	}
	
	return id;
}

unsigned int NeuroScan::logElement( Element* el, EClass eclass )
{
	if ( e2id_.find( el ) != e2id_.end() )
		return e2id_[ el ];
	
	unsigned int id = static_cast< unsigned int >( id2e_.size() );
	assert( id == id2e_.size() );
	e2id_[ el ] = id;
	id2e_.push_back( el );
	eclass_.push_back( eclass );
	
	if ( eclass == CHANNEL ) {
		Conn c( el, 0 );
		ProcInfoBase p;
		p.dt_ = dt_;
		HHChannel::reinitFunc( &c, &p );
	}
	
	if ( eclass == GATE )
	{
		bool ret = gateFinfo->add(
			scanElm_, el, el->findFinfo( "gate" ) );
		assert( ret );
		
		GateInfo& gi = gateInfo_[ id ];
		gi.chanId = currentChanId_;
		gi.xIndex = currentXIndex_;
		gi.rIndex = gateFinfo->numOutgoing( scanElm_ ) - 1;
	}
	
	return id;
}

NeuroScan::EClass NeuroScan::type( Element* e )
{
	const Cinfo* cinfo = e->cinfo();
	if ( cinfo->isA( Cinfo::find( "SynChan" ) ) )
		return SYNCHAN;
	if ( cinfo->isA( Cinfo::find( "HHChannel" ) ) )
		return CHANNEL;
	return NONE;
}

void NeuroScan::targets(
	unsigned int id,
	const string& msg,
	vector< Element* >& target ) const
{
	vector< Conn > c;
	vector< Conn >::iterator ic;
	
	Element* e = id2e_[ id ];
	e->findFinfo( msg )->outgoingConns( e, c );
	for ( ic = c.begin(); ic != c.end(); ++ic )
		target.push_back( ic->targetElement() );
}

///////////////////////////////////////////////////
// Unit tests
///////////////////////////////////////////////////
