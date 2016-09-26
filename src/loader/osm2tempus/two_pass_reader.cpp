#include "osm2tempus.h"
#include "writer.h"
#include "section_splitter.h"

#include <unordered_set>

struct PbfReaderPass1
{
public:
    void node_callback( uint64_t osmid, double lon, double lat, const osm_pbf::Tags &/*tags*/ )
    {
        points_.insert( osmid, Point(lon, lat) );
    }

    void way_callback( uint64_t /*osmid*/, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        for ( uint64_t node: nodes ) {
            auto it = points_.find( node );
            if ( it != points_.end() ) {
                int uses = it->second.uses();
                if ( uses < 2 )
                    it->second.set_uses( uses + 1 );
            }
        }
    }
    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

    PointCache& points() { return points_; }
private:
    PointCache points_;
};

struct PbfReaderPass2
{
    PbfReaderPass2( PointCache& points, Writer& writer ):
        points_( points ), writer_( writer ), section_splitter_( points_ )
    {}
    
    void node_callback( uint64_t /*osmid*/, double /*lon*/, double /*lat*/, const osm_pbf::Tags &/*tags*/ )
    {
    }

    void way_callback( uint64_t way_id, const osm_pbf::Tags& tags, const std::vector<uint64_t>& nodes )
    {
        // ignore ways that are not highway
        if ( tags.find( "highway" ) == tags.end() )
            return;

        // split the way on intersections (i.e. node that are used more than once)
        bool section_start = true;
        uint64_t old_node = nodes[0];
        if ( points_.find( old_node ) == points_.end() )
            return;
        uint64_t node_from;
        std::vector<uint64_t> section_nodes;
        for ( size_t i = 1; i < nodes.size(); i ++ ) {
            uint64_t node = nodes[i];
            auto it = points_.find( node );
            if ( it == points_.end() ) {
                // ignore ways with unknown nodes
                section_start = true;
                continue;
            }

            const Point& pt = it->second;
            if ( section_start ) {
                section_nodes.clear();
                section_nodes.push_back( old_node );
                node_from = old_node;
                section_start = false;
            }
            section_nodes.push_back( node );
            if ( i == nodes.size() - 1 || pt.uses() > 1 ) {
                //split_into_sections( way_id, node_from, node, section_nodes, tags );
                section_splitter_( way_id, node_from, node, section_nodes, tags,
                                   [&](uint64_t lway_id, uint64_t lsection_id, uint64_t lnode_from, uint64_t lnode_to, const std::vector<Point>& lpts, const osm_pbf::Tags& ltags)
                                   {
                                       writer_.write_section( lway_id, lsection_id, lnode_from, lnode_to, lpts, ltags );
                                   });
                section_start = true;
            }
            old_node = node;
        }
    }

    void relation_callback( uint64_t /*osmid*/, const osm_pbf::Tags &/*tags*/, const osm_pbf::References & /*refs*/ )
    {
    }

private:
    PointCache& points_;

    Writer& writer_;

    // structure used to detect multi edges
    std::unordered_set<node_pair> way_node_pairs;

    SectionSplitter<PointCache> section_splitter_;
};

void two_pass_pbf_read( const std::string& filename, Writer& writer )
{
    std::cout << "first pass" << std::endl;
    PbfReaderPass1 p1;
    osm_pbf::read_osm_pbf<PbfReaderPass1, StdOutProgressor>( filename, p1 );
    std::cout << p1.points().size() << " nodes cached" << std::endl;
    std::cout << "second pass" << std::endl;
    PbfReaderPass2 p2( p1.points(), writer );
    osm_pbf::read_osm_pbf<PbfReaderPass2, StdOutProgressor>( filename, p2 );
}









