#include "./Frontier.h"

// to use this function, compile with HashTable.cpp
extern uint32_t fvnHash( const char * );

bool Frontier::url_t::operator== ( const url_t& other ) const
    {
    // TODO: insert more comparisons here
    return this->url == other.url;
    }

bool Frontier::url_t::operator< ( const url_t& other ) const
    {
    // TODO: insert more comparisons here
    return this->url.size() > other.url.size( );  // shorter urls first
    }

bool Frontier::url_t::operator<= ( const url_t& other ) const
    {
    return ( *this < other ) || ( *this == other );
    }

Frontier::url_t::url_t( const String& param ) : url( param )
    {
    }

int Frontier::randomSelect( ) const
    {
    return rand( ) % urlPool.size( );
    }

void Frontier::linkToRoot( const char *root )
    {
    String rtDir( root );
    rtDir += '/';
    DIR *handle = opendir( root );
    if ( handle )
        {
        struct dirent *entry;
        while ( entry == readdir( handle ) )
            {
            // if entry is the "dot" files
            if ( DotName( entry->d_name ) )
                continue;
            // if entry is not directory
            String childName = rtDir;
            childName += entry->d_name;
            struct stat statbuf;
            if ( !stat( childName.cstr( ), &statbuf ) )
                if ( ( statbuf.st_mode & S_IFMT ) != S_IFDIR )
                    {
                    std::cerr << root << " contains non-directory inode " << entry->d_name << std::endl;
                    continue;
                    }
            else
                std::cerr << "Cannot stat file " << childName << " with errno = " << errno << std::endl;
            // link to the disk queue
            int qIdx = atoi( entry->d_name );
            if ( qIdx > urlPool.size( ) )
                {
                std::cerr << "Disk queue folders have names exceed the urlPool size" << std::endl;
                continue;
                }
            urlPool[ qIdx ] = new DiskQueue( childName.cstr( ) );
            }
        closedir( handle );
        }
    else
        std::cerr << "Cannot open root dir: " << root << " with errno = " << errno << std::endl;
    }

Frontier::Frontier( const char *root, size_t numq, size_t pqSize_, 
    int ( *f )( const Link& ) ) 
    : rootDir( root ), priorityCalculator( f ), pqSize( pqSize_ )
    {
    // initialize urlPool
    urlPool.reserve( numq );
    for ( size_t i = 0; i < numq; ++i )
        urlPool.pushBack( nullptr );
    linkToRoot( root );
    // create directories for non-existing queues
    String pathName( root );
    pathName += '/';
    for ( size_t i = 0; i < numq; ++i )
        if ( !urlPool[ i ] )
            {
            pathName += ltos( i );
            if ( mkdir( pathName.cstr( ), S_IRWXU ) )
                {
                std::cerr << "Cannot make directory " << pathName << " with errno = " << errno << std::endl;
                exit( 1 );
                }
            urlPool[ i ] = new DiskQueue( pathName.cstr( ) );
            }
    // initialize poolMutexes
    for ( size_t i = 0; i < numq; ++i )
        {
        mutex_t *mtx = new mutex_t;
        MutexInit( mtx, nullptr );
        poolMutexes.pushBack( mtx );
        }
    // initialize pqMutex
    MutexInit( &pqMutex, nullptr );
    }

Frontier::~Frontier( )
    {
    for ( size_t i = 0; i < urlPool.size( ); ++i )
        {
        delete urlPool[ i ];
        MutexDestroy( poolMutexes[ i ] );
        delete poolMutexes[ i ];
        }
    }

void Frontier::PushUrl( Link& link )
    {
    size_t dqIdx = 0;  // disk queue index
    if ( priorityCalculator )  // use priority calculator if provided
        dqIdx = priorityCalculator( link );
    else  // other wise simply hash and map
        dqIdx = fvnHash( link.URL.cstr( ) ) % urlPool.size( );
    Lock( poolMutexes[ dqIdx ] );
    urlPool[ dqIdx ]->PushBack( link.URL );
    Unlock( poolMutexes[ dqIdx ]);
    }

String Frontier::PopUrl( )
    {
    Lock( &pqMutex );
    // if the pq is empty, refills it
    if ( urlPq.empty( ) )
        {
        while ( urlPq.size( ) < pqSize )
            {
            int poolIdx = randomSelect( );
            Lock( poolMutexes[ poolIdx ] );
            if ( urlPool[ poolIdx ]->empty( ) )
                {
                // TODO: busy waiting here, may need to optimize
                // largely depends on the random algorithm to generate a new index that is not empty
                Unlock( poolMutexes[ poolIdx ] );
                continue;
                }
            else
                {
                String poppedUrl = urlPool[ poolIdx ]->PopFront( );
                urlPq.Push( poppedUrl );
                Unlock( poolMutexes[ poolIdx ] );
                }
            }
        }
    // not empty, pop a url off and return
    String nextUrl = urlPq.Top( ).url;
    urlPq.Pop( );
    Unlock( &pqMutex );
    return nextUrl;
    }