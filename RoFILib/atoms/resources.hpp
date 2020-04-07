#pragma once
#include <map>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <cstring>

// There are some APIs (e.g. VTK) which can load resources only from files. This
// APIs prevent embedding the resources. ResourceManage and ResourceFile help to
// create a temporary file from a resource embedded in the binary. Not ideal but
// probably best we can do.

// The binary embedding is isnspired by:
// https://beesbuzz.biz/code/4399-Embedding-binary-resources-with-CMake-and-C-11


class ResourceManager {
public:
    ~ResourceManager() {
        std::filesystem::remove_all( _tmpDir );
    }


    std::filesystem::path add( const char *data, int count ) {
        auto entry = _resources.find( data );
        if ( entry != _resources.end() )
            return entry->second;
        auto path = createFile( data, count );
        _resources.insert( { data, path } );
        return path;
    }

    static ResourceManager& inst() {
        static ResourceManager man;
        return man;
    }
private:
    ResourceManager() {
        auto templ = ( std::filesystem::temp_directory_path() / "XXXXXX" ).string();
        std::unique_ptr< char[] > cTempl( new char[ templ.size() ] );
        strcpy( cTempl.get(), templ.c_str() );
        if ( !mkdtemp( cTempl.get() ) )
            throw std::runtime_error( "Cannot create temporary directory" );
        _tmpDir = cTempl.get();
    }

    std::filesystem::path createFile( const char *data, int count ) {
        auto path = _tmpDir / std::to_string( _resources.size() );
        std::ofstream file( path );
        file.write( data, count );
        return path;
    }

    std::filesystem::path _tmpDir;
    std::map< const char *, std::filesystem::path > _resources;
};

class ResourceFile {
public:
    ResourceFile( const char *start, const char *end )
        : ResourceFile( start, end - start )
    {}

    ResourceFile( const char *data, int count ) :
        _path( ResourceManager::inst().add( data, count ) )
    {}

    std::filesystem::path name() const {
        return _path;
    }
private:
    std::filesystem::path _path;
};

#define LOAD_RESOURCE_FILE(x) ([]() {                                       \
        extern const char _binary_##x##_start, _binary_##x##_end;           \
        return ResourceFile(&_binary_##x##_start, &_binary_##x##_end);      \
    })()
