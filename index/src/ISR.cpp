 #include "ISR.h"
 #include "ISRSpan.h"

// ISRWord Functions


float ISR::GetHeuristicScore( Match *document )
    {
    size_t rarestlocation = 0;
    w_Occurence minOccurence = 0xFFFFFFFFFFFFFFFF;
    w_Occurence occ = 0;
    for ( size_t i = 0; i < this->GetTermNum(); ++i ) 
        {
        ISRWord *term = ( ISRWord* )*( this->GetTerms() + i );
        std::cout << "word term: ";
        std::cout << "ISR::GetHeuristicScore(): seek term number " << i << std::endl;
        Post *curPost = term->Seek( document->start );
        while ( curPost != nullptr && curPost->GetStartLocation( ) < document->end ) 
            {
            occ += 1;
            std::cout << "ISR::GetHeuristicScore(): term number " << i << " calls next\n";
            curPost = term->Next();
            }
        if ( occ == 0 ) continue;
        if ( occ < minOccurence )
            {
            minOccurence = occ;
            rarestlocation = i;
            }
        } 
    if ( minOccurence == 0xFFFFFFFFFFFFFFFF ) return 0;
    std::cout << "ISR::GetHeuristicScore(): prepare to call calculate_score with rarest term\n";
    return calculate_scores( document, ( ISRWord ** )( this->GetTerms( ) ), this->GetTermNum( ), rarestlocation, this->getWeights( ) );
//	return 0.1;
    }

Post *ISRWord::Next() 
    {
    // currIdx: the index into the posting list
    termPostingListRaw = manager->GetTermListCurrMap( term, currChunk);
    size_t numOccurence = termPostingListRaw.getHeader( )->numOfOccurence;
    if ( currIndex < numOccurence - 1 ) 
        {
        currIndex += 1;
        Location delta = termPostingListRaw.getPostAt(currIndex).delta;
        currPost.SetLocation(delta + currPost.GetStartLocation());
        } else {
        Post *post = Seek(currPost.GetStartLocation() + 1);
        if ( post == nullptr ) 
            { 
            return nullptr;
            }
        else currPost = *post;
        }
    return &currPost;
    }

 Post *ISRWord::NextNoUpdate( )
 {
     size_t numOccurence = termPostingListRaw.getHeader( )->numOfOccurence;

     if ( currIndex < numOccurence - 1 ) {
         Offset nextIndex;
         nextIndex = currIndex += 1;
         Location delta = termPostingListRaw.getPostAt(nextIndex).delta;
         nextPost.SetLocation(delta + currPost.GetStartLocation());
         return &nextPost;
     }
     else return nullptr;
 }

Post *ISRWord::NextEndDoc() 
    {
    EndDocPostingListRaw endDoc = manager->GetEndDocList(currChunk);
    vector<Location> endLocs = manager->getChunkEndLocations();
    size_t target = currPost.GetStartLocation();
    size_t chunkSize;
    size_t temp;
    size_t offset;
    if (currChunk > 0) 
        {
        chunkSize = endLocs[currChunk] - endLocs[currChunk - 1];
        offset = endLocs[currChunk - 1];
        } else {
        chunkSize = endLocs[currChunk];
        offset = 0;
        }   
    Location endLoc = seekEndDocTarget(&endDoc, target - offset, temp, chunkSize);
    if (currChunk > 0) endLoc += endLocs[currChunk - 1];
    Doc.SetLocation(endLoc);
    return &Doc;
    }

Post *ISRWord::Seek(size_t target) 
    {
    vector<Location> endLocs = manager->getChunkEndLocations( );
    size_t numChunks = endLocs.size( );
    size_t chunkIndex;
    Location result = -1;
    for ( chunkIndex = 0; chunkIndex < numChunks; chunkIndex++ ) 
        {
        if ( endLocs[ chunkIndex ] >= target ) 
            break;
        }
    if ( chunkIndex >= numChunks ) 
        {
        return nullptr;
        }
    for (size_t chunk = chunkIndex; chunk < numChunks; chunk++) 
        {
        try 
            {
            TermPostingListRaw termraw = manager->GetTermList(term, chunk);
            if ( termraw.getHeader( )->numOfOccurence == 0 ) 
                continue;
            else 
                {
                size_t temp;
                Offset chunkSize;
                size_t offset;
                size_t searchTarget;
                if ( chunk > 0 ) 
                    {
                    chunkSize = endLocs[ chunk ] - endLocs[ chunk - 1 ];
                    offset = endLocs[ chunk - 1 ];
                    } 
                else 
                    {
                    chunkSize = endLocs[ chunk ];
                    offset = 0;
                    }
                if ( target < offset ) 
                    searchTarget = 0;
                else 
                    searchTarget = target - offset;
                result = seekTermTarget( &termraw, searchTarget, temp, chunkSize );
                if ( result == -1 ) 
                    continue;
                if ( chunk != 0 )
                    result += endLocs[ chunk - 1 ];
                currChunk = chunk;  // current chunk
                termPostingListRaw = termraw;  // current posting list
                size_t normalize = 0;
                if (chunk > 0) {
                    normalize = endLocs[chunk - 1];
                }
                currIndex = temp;
                absoluteIndex = normalize + temp;
                //currIndex = endLocs[ chunk - 1] + temp//temp;  // current index into the posting list
                break;
                }
            }
        catch ( const char *excep ) 
            {
            continue;
            }
        }
    if ( result == -1 ) 
        return nullptr;
    currPost.SetLocation( result );
    return &currPost;
    }

Location ISRWord::GetStartLocation() 
    {
    //fetch termpostinglistraw for the first chunk
    vector<Location> endLocs = manager->getChunkEndLocations();
    size_t numChunks = endLocs.size();
    Location result = -1;
    for (size_t chunk = 0; chunk < numChunks; chunk++) 
        {
        try 
            {
            TermPostingListRaw termraw = manager->GetTermList(term, chunk);
            if (termraw.getHeader()->numOfOccurence == 0) continue;
            else {
                size_t temp;
                Offset chunkSize;
                if (chunk > 0) {
                    chunkSize = endLocs[chunk] - endLocs[chunk - 1];
                } else {
                    chunkSize = endLocs[chunk];
                }
                result = seekTermTarget(&termraw, 0, temp, chunkSize);
                if (chunk != 0)result += endLocs[chunk - 1];
                return result;
                }
            }
        catch (const char *excep) 
            {
            continue;
            }
        }
    return result;
}

Location ISRWord::GetEndLocation() 
    {
    size_t numChunks = manager->getChunkEndLocations().size();
    vector<Location> endLocs = manager->getChunkEndLocations();
    size_t result = -1;
    for (size_t chunk = numChunks - 1; chunk >= 0; chunk--) 
        {
        try 
            {
            TermPostingListRaw termraw = manager->GetTermList(term, chunk);
            if (termraw.getHeader()->numOfOccurence == 0) continue;
            else {
                size_t numOccurence = termraw.getHeader()->numOfOccurence;
                size_t temp;
                size_t chunkSize;
                if (chunk > 0) {
                    chunkSize = endLocs[chunk] - endLocs[chunk - 1];
                } else {
                    chunkSize = endLocs[chunk];
                }
                result = seekTermTarget(&termraw, 0, temp, chunkSize);
                for (int i = 1; i < numOccurence; i++) {
                    result += termraw.getPostAt(i).delta;
                }
                if (chunk != 0)result += endLocs[chunk - 1];
                return result;
                }
            }
        catch (const char *excep) {
            continue;
        }
        }
    return result;
    }

d_Occurence ISRWord::GetDocumentCount() 
    {
    std::cout << "ISRWord::GetDocumentCount():\n";
    size_t numChunks = manager->getChunkEndLocations().size();
    w_Occurence total = 0;
    for (int i = 0; i < numChunks; i++) 
        {
        try {
            total += manager->GetTermList(  term, i ).getHeader( )->numOfDocument;
            }
        catch(const char* excep)
            {
            std::cerr << "ISRWord::GetDocumentCound() exception = " << excep << std::endl;
            continue;
            }
        }
    std::cout << "ISRWord::GetDocumentCount() returning\n";
    return total;
    }

w_Occurence ISRWord::GetNumberOfOccurrences() 
    {
    std::cout << "ISRWord::GetNumberOfOccurences()\n";
    size_t numChunks = manager->getChunkEndLocations( ).size( );
    w_Occurence total = 0;
    for ( int i = 0; i < numChunks; i++ ) 
        {
        try 
            {
            total += manager->GetTermList( term, i ).getHeader( )->numOfOccurence;
            }
        catch(const char* excep)
            {
            std::cerr << "ISRWord::GetNumberOfOccurences() exception = " << excep << std::endl;
            continue;
            }
        }
    std::cout << "ISRWord::GetNumberOfOccurences(): returning with res = " << total << std::endl;
    return total;
    }

Post *ISRWord::GetCurrentPost() 
    {
    return &currPost;
    }

ISR **ISRWord::GetTerms() 
    {
    return nullptr;
    }


// ISREndDoc Functions
Post *ISREndDoc::GetCurrentPost(){
    return &currPost;
}

Post *ISREndDoc::Next() 
    {
                     

    endDocPostingListRaw = manager->GetEndDocListCurrMap( currChunk );
    size_t numDoc = endDocPostingListRaw.getHeader()->numOfDocument;
    if (currIndex < numDoc - 1) 
        {
        currIndex += 1;
        Location delta = endDocPostingListRaw.getPostAt(currIndex).delta;
        currPost.SetLocation(delta + currPost.GetStartLocation());
        } else {
        Post *post = Seek(currPost.GetStartLocation() + 1);
        if (post == nullptr) return nullptr;
        else currPost = *post;
        }
    return &currPost;
    }

Post *ISREndDoc::NextEndDoc(){
    return Next();
}

Post *ISREndDoc::Seek(Location target) 
    {
    vector<Location> endLocs = manager->getChunkEndLocations();
    vector<d_Occurence> docCounts = manager->getDocCountsAfterChunk();
    size_t numChunks = endLocs.size();
    size_t chunkIndex;
    Location result = -1;
    bool containFlag = false;
    for ( chunkIndex = 0; chunkIndex < numChunks; chunkIndex++ ) // 3925
        {
        if ( endLocs[ chunkIndex ] >= target ) 
            {
            containFlag = true;
            break;
            }
        }
    if ( !containFlag )  // check chunkIndex 
        return nullptr;
    std::cout << "ISREndDoc::Seek: seek on enddoc from: " << chunkIndex << " to " << numChunks << std::endl;
    std::cout << "ISREndDoc::Seek: manager with numChunks" << manager.getNumChunks( ) << std::endl;
    for ( size_t chunk = chunkIndex; chunk < numChunks; chunk++ ) 
        {
        try 
            {
            // std::cout << "ISREndDoc::Seek(): trying to get chunk at " << chunk << std::endl;
            EndDocPostingListRaw docraw = manager->GetEndDocList( chunk );
            size_t temp;
            Offset chunkSize;
            size_t offset;
            size_t searchTarget;
            if (chunk > 0) {
                chunkSize = endLocs[chunk] - endLocs[chunk - 1];
                offset = endLocs[chunk - 1];
            } else {
                chunkSize = endLocs[chunk];
                offset = 0;
            }
            if (target < offset) searchTarget = 0;
            else searchTarget = target - offset;
            result = seekEndDocTarget(&docraw, searchTarget, temp, chunkSize);
            if (result == -1) 
                continue;
            if (chunk != 0)result += endLocs[chunk - 1];
            currChunk = chunk;
            endDocPostingListRaw = docraw;
            size_t normalize = 0;
            if (chunk > 0) {
                normalize = docCounts[chunk - 1];
            }
            currIndex =  temp;
            absoluteIndex = normalize + temp;
            break;
            }
        catch ( const char * excep ) 
            {
            // std::cout << "ISREndDoc::Seek(): Exception received " << excep << std::endl;
            continue;
            }
        }
    if (result == -1) 
        return nullptr;
    currPost.SetLocation( result );
    return &currPost;
    }

Location ISREndDoc::GetStartLocation() 
    {
    //fetch termpostinglistraw for the first chunk
    vector<Location> endLocs = manager->getChunkEndLocations();
    size_t numChunks = endLocs.size();
    Location result = -1;
    for (size_t chunk = 0; chunk < numChunks; chunk++) 
        {
        try 
            {
            EndDocPostingListRaw docRaw = manager->GetEndDocList(chunk);
            size_t temp;
            Offset chunkSize;
            if (chunk > 0) {
                chunkSize = endLocs[chunk] - endLocs[chunk - 1];
            } else {
                chunkSize = endLocs[chunk];
            }
            result = seekEndDocTarget(&docRaw, 0, temp, chunkSize);
            if (chunk != 0)result += endLocs[chunk - 1];
            return result;
            }
        catch (const char* exception) 
            {
            continue;
            }
        }
    return result;
    }

Location ISREndDoc::GetEndLocation( ) 
    {
    size_t numChunks = manager->getChunkEndLocations( ).size( );
    vector< Location > endLocs = manager->getChunkEndLocations( );
    size_t result = -1;
    for ( size_t chunk = numChunks - 1; chunk >= 0; chunk-- ) 
        {
        try 
            {
            EndDocPostingListRaw docraw = manager->GetEndDocList( chunk );
            size_t temp;
            size_t chunkSize;
            size_t numOccurence = docraw.header->numOfDocument;
            if ( chunk > 0 ) 
                {
                chunkSize = endLocs[ chunk ] - endLocs[ chunk - 1 ];
                } else 
                {
                chunkSize = endLocs[ chunk ];
                }
            result = seekEndDocTarget(&docraw, 0, temp, chunkSize);
            for (int i = 1; i < numOccurence; i++) 
                {
                result += docraw.getPostAt(i).delta;
                }
            if (chunk != 0)result += endLocs[chunk - 1];
            return result;
            }
        catch (const char * exception) 
            {
            continue;
            }
        }
    return result;
    }

unsigned ISREndDoc::GetDocumentLength() 
    {
    ::vector<d_Occurence> docOccurenceAfterChunk = manager->getDocCountsAfterChunk();
    
    size_t currChunk = 0;
    for(; currChunk < docOccurenceAfterChunk.size(); ++currChunk ) 
        {
        if(absoluteIndex < docOccurenceAfterChunk[currChunk]) 
            {
            break;
            }
        }
    if(currChunk == docOccurenceAfterChunk.size()) 
        {
        return -1;
        }
    else {
        return manager->GetDocumentDetails(absoluteIndex, currChunk).lengthOfDocument;
        }
    }


unsigned ISREndDoc::GetTitleLength() 
    {
    ::vector<d_Occurence> docOccurenceAfterChunk = manager->getDocCountsAfterChunk();
    
    size_t currChunk = 0;
    for(; currChunk < docOccurenceAfterChunk.size(); ++currChunk ) 
        {
        if(absoluteIndex < docOccurenceAfterChunk[currChunk]) 
            {
            break;
            }
        }
    if(currChunk == docOccurenceAfterChunk.size()) 
        {
        return -1;
        }
    else 
        {
        return strlen(manager->GetDocumentDetails(absoluteIndex, currChunk).title.cstr());
        }
 }

unsigned ISREndDoc::GetUrlLength() 
    {
    ::vector<d_Occurence> docOccurenceAfterChunk = manager->getDocCountsAfterChunk();    
    size_t currChunk = 0;
    for(; currChunk < docOccurenceAfterChunk.size(); ++currChunk ) 
        {
        if(absoluteIndex < docOccurenceAfterChunk[currChunk]) 
            {
            break;
            }
        }
    if(currChunk == docOccurenceAfterChunk.size()) 
        {
        return -1;
        }
    else 
        {
        return strlen(manager->GetDocumentDetails(absoluteIndex, currChunk).url.cstr());
        }
    }

Offset ISREndDoc::GetCurrIndex() 
    {
    return absoluteIndex;
    }

ISR **ISREndDoc::GetTerms() 
    {
    return nullptr;
    }
