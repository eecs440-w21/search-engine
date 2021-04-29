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
        // std::cout << "word term: ";
        // term->printTerm( );
        // term->Seek( document->start );
        Post *curPost = term->Seek( document->start );
        while ( curPost != nullptr && curPost->GetStartLocation( ) < document->end ) 
            {
            occ += 1;
            curPost = term->Next();
            // if ( next == nullptr ) break;
            // if ( next->GetStartLocation() >= document->end ) break;
            }
        if ( occ == 0 ) continue;
        if ( occ < minOccurence )
            {
            minOccurence = occ;
            rarestlocation = i;
            }
        } 
    if ( minOccurence == 0xFFFFFFFFFFFFFFFF ) return 0;
    return calculate_scores( document, ( ISRWord ** )( this->GetTerms( ) ), this->GetTermNum( ), rarestlocation, this->getWeights( ) );
//	return 0.1;
    }

Post *ISRWord::Next() 
    {
    // currIdx: the index into the posting list
    size_t numOccurence = termPostingListRaw.getHeader( )->numOfOccurence;
    if ( currIndex < numOccurence - 1 ) 
        {
        currIndex += 1;
        Location delta = termPostingListRaw.getPostAt(currIndex).delta;
        currPost.SetLocation(delta + currPost.GetStartLocation());
        } else {
        // std::cout << "ISRWord::Next(): Reseeking to the next chunk" << std::endl;
        Post *post = Seek(currPost.GetStartLocation() + 1);
        if ( post == nullptr ) 
            { 
            // std::cout << ' ' << "post not found after reseek" << std::endl;
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
    size_t numChunks = endLocs.size();
    // std::cout << "ISRWord::Seek: numChunks: " << numChunks << std::endl;
    size_t chunkIndex;
    Location result = -1;
    // std::cout << std::endl;
    for ( chunkIndex = 0; chunkIndex < numChunks; chunkIndex++ ) 
        {
        if ( endLocs[ chunkIndex ] >= target ) 
            break;
        }
    if ( chunkIndex >= numChunks ) 
        {
        // std::cout << "ISRWord::Seek: chunkIndex greater than num chunks: " << chunkIndex << " " << numChunks << std::endl;
        return nullptr;
        }
    // std::cout << "ISRWord::Seek: starting at chunkIndex: " << chunkIndex << " and iterating until: " << numChunks << std::endl;
    for (size_t chunk = chunkIndex; chunk < numChunks; chunk++) 
        {
        // std::cout << "Searching in chunk: " << chunk << std::endl;
        try 
            {
            TermPostingListRaw termraw = manager->GetTermList(term, chunk);
            // std::cout << "ISRWord::Seek: Found term: " << termraw.getHeader()->term << " with num doc: " <<  termraw.getHeader()->numOfDocument << std::endl;
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
                currIndex = normalize + temp;
                //currIndex = endLocs[ chunk - 1] + temp//temp;  // current index into the posting list
                break;
                }
            }
        catch ( const char *excep ) 
            {
            // std::cout << "ISRWord::Seek: Did not find term" << std::endl;
            continue;
            }
        }
    if ( result == -1 ) 
        return nullptr;
    currPost.SetLocation( result );
    // std::cout << "ISRWord::Seek: Returning from seek result, location = " << result << std::endl;
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
    size_t numChunks = manager->getChunkEndLocations().size();
    w_Occurence total = 0;
    for (int i = 0; i < numChunks; i++) 
        {
        try {
            total += manager->GetTermList(term, i).getHeader()->numOfDocument;
            }
        catch(const char* excep)
            {
            continue;
            }
        }
    return total;
    }

w_Occurence ISRWord::GetNumberOfOccurrences() 
    {
    size_t numChunks = manager->getChunkEndLocations().size();
    w_Occurence total = 0;
    for (int i = 0; i < numChunks; i++) 
        {
        try 
            {
            total += manager->GetTermList(term, i).getHeader()->numOfOccurence;
            }
        catch(const char* excep)
            {
            continue;
            }
        }
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
    // std::cout << "ISREndDoc::Seek: seek on enddoc from: " << chunkIndex << " to " << numChunks << std::endl;
    // std::cout << "ISREndDoc::Seek: manager with numChunks" << manager->getNumChunks( ) << std::endl;
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
            currIndex = normalize + temp;
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
        if(currIndex < docOccurenceAfterChunk[currChunk]) 
            {
            break;
            }
        }
    if(currChunk == docOccurenceAfterChunk.size()) 
        {
        return -1;
        }
    else {
        return manager->GetDocumentDetails(currIndex, currChunk).lengthOfDocument;
        }
    }


unsigned ISREndDoc::GetTitleLength() 
    {
    ::vector<d_Occurence> docOccurenceAfterChunk = manager->getDocCountsAfterChunk();
    
    size_t currChunk = 0;
    for(; currChunk < docOccurenceAfterChunk.size(); ++currChunk ) 
        {
        if(currIndex < docOccurenceAfterChunk[currChunk]) 
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
        return strlen(manager->GetDocumentDetails(currIndex, currChunk).title.cstr());
        }
 }

unsigned ISREndDoc::GetUrlLength() 
    {
    ::vector<d_Occurence> docOccurenceAfterChunk = manager->getDocCountsAfterChunk();    
    size_t currChunk = 0;
    for(; currChunk < docOccurenceAfterChunk.size(); ++currChunk ) 
        {
        if(currIndex < docOccurenceAfterChunk[currChunk]) 
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
        return strlen(manager->GetDocumentDetails(currIndex, currChunk).url.cstr());
        }
    }

Offset ISREndDoc::GetCurrIndex() 
    {
    return currIndex;
    }

ISR **ISREndDoc::GetTerms() 
    {
    return nullptr;
    }
