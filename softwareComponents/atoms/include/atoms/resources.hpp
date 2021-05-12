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


/**
 * \brief Manager of embedded resources
 *
 * ResourceManager allows the user to register a resource and obtain a file name
 * for it. The class is a singleton, obtain instance by calling
 * ResourceManager::inst().
 *
 * This class is usually not needed when working with resources specified via
 * CMake macro. Look at ResourceFile and LOAD_RESOURCE_FILE instead.
 */
class ResourceManager {
public:
    ~ResourceManager() {
        std::filesystem::remove_all( _tmpDir );
    }

    /**
     * \brief Add a resource and obtain a valid file path for it
     *
     * If the resource on given address was already added, existing file is
     * returned. The resource is specified by address and its size in bytes.
     */
    std::filesystem::path add( const char *data, int count ) {
        auto entry = _resources.find( data );
        if ( entry != _resources.end() )
            return entry->second;
        auto path = createFile( data, count );
        _resources.insert( { data, path } );
        return path;
    }

    /**
     * \brief Get an instance of the manager
     */
    static ResourceManager& inst() {
        static ResourceManager man;
        return man;
    }
private:
    ResourceManager() {
        auto templ = ( std::filesystem::temp_directory_path() / "XXXXXX" ).string();
        std::unique_ptr< char[] > cTempl( new char[ templ.size() + 1] );
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

/**
 * \brief Represents a single resource with na associated filename.
 *
 * If you are using resources specified by the CMake macro, do not create
 * instances of the class directly, but use macro LOAD_RESOURCE_FILE.
 */
class ResourceFile {
public:
    ResourceFile( const char *start, const char *end )
        : ResourceFile( start, end - start )
    {}

    ResourceFile( const char *data, int count ) :
        _path( ResourceManager::inst().add( data, count ) )
    {}

    /**
     * \brief Get filename associated with given resource
     */
    std::filesystem::path name() const {
        return _path;
    }
private:
    std::filesystem::path _path;
};

/**
 * \brief Obtain an function that returns ResourceFile for given resource
 * specified by the CMake macro.
 *
 * Specify the resource name directly, without string quotes, and replace
 * special symbols by and underscore. E.g., for resource `test/file.bmp` use
 * `test_file_bmp`.
 */
#define LOAD_RESOURCE_FILE_LAZY(x) ([]() {                             \
    extern const char _binary_##x##_start, _binary_##x##_end;           \
    return ResourceFile(&_binary_##x##_start, &_binary_##x##_end);      \
})

/**
 * \brief Obtain an instance of ResourceFile for given resource specified by the
 * CMake macro.
 *
 * Specify the resource name directly, without string quotes, and replace
 * special symbols by and underscore. E.g., for resource `test/file.bmp` use
 * `test_file_bmp`.
 */
#define LOAD_RESOURCE_FILE(x) ([]() {                                       \
        extern const char _binary_##x##_start, _binary_##x##_end;           \
        return ResourceFile(&_binary_##x##_start, &_binary_##x##_end);      \
    })()
