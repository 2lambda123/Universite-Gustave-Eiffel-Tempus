/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_PUBLIC_TRANSPORT_GRAPH_HH
#define TEMPUS_PUBLIC_TRANSPORT_GRAPH_HH

#include "common.hh"
#include "road_graph.hh"

namespace Tempus {
/**
   A PublicTransport::Graph is a made of PublicTransport::Stop and PublicTransport::Section

   It generally maps to the database's schema: one class exists for each table.
   Tables with 1<->N arity are represented by STL containers (vectors or lists)
   External keys are represented by pointers to other classes or by vertex/edge descriptors.

   PublicTransport::Stop and PublicTransport::Section classes are used to build a BGL public transport graph.
*/
namespace PublicTransport {
struct Network : public Base {
    DECLARE_RW_PROPERTY( name, std::string );

public:
    /// Transport types provided by this network
    /// It is a ORed combination of TransportType IDs (power of two)
    // FIXME to remove
    db_id_t provided_transport_types;
};

///
/// storage types used to make a road graph
typedef boost::vecS VertexListType;
typedef boost::vecS EdgeListType;

///
/// To make a long line short: VertexDescriptor is either typedef'd to size_t or to a pointer,
/// depending on VertexListType and EdgeListType used to represent lists of vertices (vecS, listS, etc.)
typedef boost::mpl::if_<boost::detail::is_random_access<VertexListType>::type, size_t, void*>::type Vertex;
/// see adjacency_list.hpp
typedef boost::detail::edge_desc_impl<boost::directed_tag, Vertex> Edge;

struct Stop;
struct Section;
///
/// Definition of a public transport graph
typedef boost::adjacency_list<VertexListType, EdgeListType, boost::directedS, Stop, Section> Graph;

///
/// Used as a vertex in a PublicTransportGraph.
/// Refers to the 'pt_stop' DB's table
struct Stop : public Base {
public:
    Stop() : graph_(0) {}
    explicit Stop( const Graph* g ) : Base(), graph_(g) {}
private:
    const Graph* graph_;

public:
    const Graph* graph() const { return graph_; }

    /// This is a shortcut to the vertex index in the corresponding graph, if any.
    /// Needed to speedup access to a graph's vertex from a Node.
    /// Can be null
    DECLARE_RW_PROPERTY( vertex, OrNull<Vertex> );

    DECLARE_RW_PROPERTY( name, std::string );
    DECLARE_RW_PROPERTY( is_station, bool );

    ///
    /// link to a possible parent station, or null
    DECLARE_RW_PROPERTY( parent_station, OrNull<Vertex> );

    /// link to a road edge
    DECLARE_RW_PROPERTY( road_edge, Road::Edge );

    ///
    /// optional link to the opposite road edge
    /// Can be null
    DECLARE_RW_PROPERTY( opposite_road_edge, OrNull<Road::Edge> );

    ///
    /// Number between 0 and 1 : position of the stop on the main road section
    DECLARE_RW_PROPERTY( abscissa_road_section, double );

    ///
    /// Fare zone ID of this stop
    DECLARE_RW_PROPERTY( zone_id, int );
};

///
/// used as an Edge in a PublicTransportGraph
struct Section {
    /// This is a shortcut to the edge index in the corresponding graph, if any.
    /// Needed to speedup access to a graph's edge from a Section
    /// Can be null
    Edge edge;
    const Graph* graph;

    // A Section has no proper id, but a stop_from and a stop_to
    db_id_t stop_from;
    db_id_t stop_to;

    /// must not be null
    db_id_t network_id;
};

typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;

///
/// Refers to the 'pt_calendar' table
struct Calendar : public Base {
    bool monday;
    bool tuesday;
    bool wednesday;
    bool thursday;
    bool friday;
    bool saturday;
    bool sunday;

    Date start_date, end_date;

    ///
    /// Refers to the 'pt_calendar_date' table.
    /// It represents exceptions to the regular service
    struct Exception : public Base {
        Date calendar_date;

        enum ExceptionType {
            ServiceAdded = 1,
            ServiceRemoved
        };
        ExceptionType exception_type;
    };

    std::vector<Exception> service_exceptions;
};

///
/// Trip, Route, StopTime and Frequencies classes
///
struct Trip : public Base {
    std::string short_name;

    ///
    /// Refers to the 'pt_stop_time' table
    struct StopTime : public Base {
        ///
        /// Link to the Stop. Must not be null.
        /// Represents the link part of the "stop_sequence" field
        PublicTransport::Vertex stop;

        Time arrival_time;
        Time departure_time;
        std::string stop_headsign;

        int pickup_type;
        int drop_off_type;
        double shape_dist_traveled;
    };

    ///
    /// Refers to the 'pt_frequency' table
    struct Frequency : public Base {
        Time start_time, end_time;
        int headways_secs;
    };

    /// This is the definition of a list of stop times for a trip.
    /// The list of stop times has to be ordered to represent the sequence of stops
    /// (based on the "stop_sequence" field of the corresponding "stop_times" table
    typedef std::list< std::vector< StopTime > > StopTimes;

    typedef std::list<Frequency> Frequencies;

    ///
    /// List of all stop times. Can be a subset of those stored in the database.
    StopTimes stop_times;
    ///
    /// List of frequencies for this trip
    Frequencies frequencies;

    ///
    /// Link to the dates when service is available.
    /// Must not be null.
    Calendar* service;

protected:
    bool check_consistency_() {
        REQUIRE( service != 0 );
        return true;
    }
};

///
/// refers to the 'pt_route' DB's table
struct Route : public Base {
    ///
    /// public transport network
    db_id_t network_id;

    std::string short_name;
    std::string long_name;

    enum RouteType {
        TypeTram = 0,
        TypeSubway,
        TypeRail,
        TypeBus,
        TypeFerry,
        TypeCableCar,
        TypeSuspendedCar,
        TypeFunicular
    };
    int route_type;

    std::vector<Trip> trips;

protected:
    bool check_consistency_() {
        REQUIRE( ( route_type >= TypeTram ) && ( route_type <= TypeFunicular ) );
        return true;
    }
};

///
/// Refers to the 'pt_fare_rule' table
struct FareRule : public Base {
    Route* route;

    typedef std::vector<int> ZoneIdList;
    ZoneIdList origins;
    ZoneIdList destinations;
    ZoneIdList contains;
};

struct FareAttribute : public Base {
    char currency_type[4]; ///< ISO 4217 codes
    double price;

    enum TransferType {
        NoTransferAllowed = 0,
        OneTransferAllowed,
        TwoTransfersAllowed,
        UnlimitedTransfers = -1
    };
    int transfers;

    int transfers_duration; ///< in seconds

    typedef std::vector<FareRule> FareRulesList;
    FareRulesList fare_rules;

protected:
    bool check_consistency_() {
        REQUIRE( ( ( transfers >= NoTransferAllowed ) && ( transfers <= TwoTransfersAllowed ) ) || ( transfers == -1 ) );
        return true;
    }

    FareAttribute() {
        strncpy( currency_type, "EUR", 3 ); ///< default value
    };
};

struct Transfer : public Base {
    ///
    /// Link between two stops.
    /// Must not be null
    Vertex from_stop, to_stop;

    enum TranferType {
        NormalTransfer = 0,
        TimedTransfer,
        MinimalTimedTransfer,
        ImpossibleTransfer
    };
    int transfer_type;

    /// Must be positive not null.
    /// Expressed in seconds
    int min_transfer_time;

protected:
    bool check_consistency_() {
        REQUIRE( ( transfer_type >= NormalTransfer ) && ( transfer_type <= ImpossibleTransfer ) );
        REQUIRE( min_transfer_time > 0 );
        return true;
    }
};

} // PublicTransport namespace
} // Tempus namespace

#endif
