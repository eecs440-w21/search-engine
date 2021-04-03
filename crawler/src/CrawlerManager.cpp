#include "CrawlerManager.h"

CrawlerManager::CrawlerManager( 
    const char *dir, size_t numq, size_t pqsize, 
    const char *filename, int num_objects, double false_positive_rate,
    size_t numCrawlers ) 
    : frontier( dir, numq, pqsize ),   
      visited( filename, num_objects, false_positive_rate ),
      crawlers( numCrawlers )
    { 
    }

void CrawlerManager::start()
    {
    for ( size_t i = 0; i < crawlers.size(); ++i )
        {
        crawlers[i].setParameters( i, &frontier, &visited );
        crawlers[i].Start();
        }
    }

void CrawlerManager::halt()
    {
    for ( size_t i = 0; i < crawlers.size(); ++i )
        {
        crawlers[i].Kill();
        }
    }

// int main( int argc, char **argv )
//     {
//     if ( argc != 4 )
//         {
//         std::cout << "Usage: " << argv[0] << " seedFile urlQueueFile numCrawlers \n";
//         return 1; 
//         }
//     CrawlerManager crawlerManager( argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]) );
//     char response;
//     std::cout << "Start crawling? (Y/N): ";
//     std::cin >> response;
//     if ( tolower( response ) == 'y' )
//         crawlerManager.start();
//     }
