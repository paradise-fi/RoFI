#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>

#include <string>
#include <utility>
#include <vector>


inline double trim( double value, double min, double max, std::string debugName )
{
    if ( value < min )
    {
        gzwarn << "Value of " << debugName << " trimmed from " << value << " to " << min << "\n";
        return min;
    }
    if ( value > max )
    {
        gzwarn << "Value of " << debugName << " trimmed from " << value << " to " << max << "\n";
        return max;
    }
    return value;
}

inline double trim( double value, const std::pair< double, double > & limits, std::string debugName )
{
    return trim( value, limits.first, limits.second, std::move( debugName ) );
}

inline bool equal( double first, double second, double precision )
{
    return first <= second + precision && second <= first + precision;
}

inline gazebo::common::URI getURIFromModel( gazebo::physics::ModelPtr model, const std::string & postfix )
{
    std::string uriString = "data://world/" + model->GetWorld()->Name();

    std::vector< std::string > modelNames;
    for ( gazebo::physics::BasePtr elem = model; elem; elem = elem->GetParent() )
    {
        if ( elem->HasType( gazebo::physics::Base::EntityType::MODEL ) )
        {
            modelNames.push_back( elem->GetName() );
        }
    }

    for ( auto it = modelNames.rbegin(); it != modelNames.rend(); it++ )
    {
        uriString += "/model/" + *it;
    }

    return { std::move( uriString ) + "/" + postfix };
}

inline std::string getElemPath( gazebo::physics::BasePtr elem, const std::string & delim = "/" )
{
    std::vector< std::string > names;

    while ( elem )
    {
        names.push_back( elem->GetName() );
        elem = elem->GetParent();
    }

    auto it = names.rbegin();
    std::string elemPath = *it++;
    while ( it != names.rend() )
    {
        elemPath += delim + *it++;
    }

    return elemPath;
}

// Gets path delimeted with '::' and returns path delimeted by '/'
inline std::string replaceDelimeter( std::string_view sensorPath )
{
    std::vector< std::string_view > splitPath;
    int last = 0;
    for ( size_t i = 0; i < sensorPath.size() - 1; i++ )
    {
        if ( sensorPath[ i ] == ':' && sensorPath[ i + 1 ] == ':' )
        {
            splitPath.push_back( sensorPath.substr( last, i - last ) );
            last = i + 2;
        }
    }
    splitPath.push_back( sensorPath.substr( last ) );

    std::string topicName;
    for ( auto name : splitPath )
    {
        topicName += "/";
        topicName += name;
    }

    return topicName;
}

bool isRofiConnector( gazebo::physics::ModelPtr model )
{
    if ( model->GetPluginCount() == 0 )
    {
        return false;
    }

    ignition::msgs::Plugin_V plugins;
    bool success = false;
    model->PluginInfo( getURIFromModel( model, "plugin" ), plugins, success );

    if ( !success )
    {
        gzwarn << "Did not succeed in getting plugins from nested model\n";
        return false;
    }

    for ( auto & plugin : plugins.plugins() )
    {
        if ( plugin.has_filename() && plugin.filename() == "libconnectorPlugin.so" )
        {
            return true;
        }
    }

    return false;
}
