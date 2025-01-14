#include <iostream>
#include <fstream>
#include "../include/constraint_solver.h"
#include "../include/abstractISR.h"
#include "../../index/include/IndexConstructor.h"
#include "../../utility/include/mString.h"
#include <filesystem>
#include <string>
#define ASSERT(left,operator,right) { if(!((left) operator (right))){ std::cerr << "ASSERT FAILED: " << #left << #operator << #right << " @ " << __FILE__ << " (" << __LINE__ << "). " << #left << "=" << (left) << "; " << #right << "=" << (right) << std::endl; } }
using namespace std;

int main(int argc, char *argv[]) {
   filesystem::remove_all(CHUNK_DIRECTORY);
   filesystem::create_directory(CHUNK_DIRECTORY);
   int doccount = 0;
   ifstream docs;
   docs.open("testdoc.txt");
   IndexConstructor ic(0);
   String term;
   char term0[10];
   char* pos=term0;
   char charr;
   while (docs.get(charr))
      {
      if (charr == '#')
         break;
      else if (charr == '/') 
         {
         ic.Insert("cat_title", "cat.com");
         doccount++;
         // cout<<"URL Inserted!\n";
         }
      else if (charr != '\t')
         *pos++ = charr;
      else if (pos != term0)
         {
         *pos = 0;
         pos = term0;
         String term(term0);
         ic.Insert(term, Body);
         // cout<<"Inserted "<<term0<<"\n";
         // // std::cout << "Metadta chunk ends in TEST>CPP: " << std::endl;
         ::vector<Location> chunkEnds = ic.fileManager.getChunkEndLocations();
         // for( unsigned int i = 0; i < chunkEnds.size(); ++i) {
         //    // std::cout << chunkEnds[i] ;
         // }
         // // std::cout << std::endl;
         }
      }
   ic.FinishConstruction();
   FileManager manager = ic.fileManager;
   Dictionary dict(0);
   // char word[] = {'c', 'a', 't', '\0'};
   // ISRWord isrword = *dict.OpenISRWord(word);

   // helper: create three ISRWord items
   ISREndDoc *EndDoc = dict.OpenISREndDoc();
   char quick[] = {'q', 'u', 'i', 'c', 'k', '\0'};
   char brown[] = {'b', 'r', 'o', 'w', 'n', '\0'};
   char fox[] = {'f', 'o', 'x', '\0'};

   ISRWord *word_quick = dict.OpenISRWord(quick);
   ISRWord *word_brown = dict.OpenISRWord(brown);
   ISRWord *word_fox = dict.OpenISRWord(fox);

   //// std::cout << word_quick->GetCurrentPost()->GetStartLocation() << std::endl;
   // while(word_quick->Next()) {
   //    // std::cout << word_quick->GetCurrentPost()->GetStartLocation() << std::endl;
   // }

   Post* res = nullptr;
   ::vector<Match*>* res_ptr = nullptr;
// query 1: quick | fox
   cout << "q1 Results:" << endl;
   ISR *terms_q1[] = {word_quick, word_fox};
   ISROr *q1 = new ISROr(terms_q1, 2);

   cout<<"ISROr Seek tests:"<<endl;
   res = q1->Seek(0);
   ASSERT(res->GetStartLocation(), ==, 0);
   res = q1->Seek(2);
   ASSERT(res->GetStartLocation(), ==, 2);
   res = q1->Seek(4);
   ASSERT(res->GetStartLocation(), ==, 5);
   res = q1->Seek(10);
   ASSERT(res->GetStartLocation(), ==, 12);
   res = q1->Seek(20);
   ASSERT(res->GetStartLocation(), ==, 21);
   res = q1->Seek(30);
   ASSERT(res, ==, nullptr);

   cout<<"constraint solver output: (see comment under line " << __LINE__ << " for correct output)"<<endl;
   // (0,0,3)(1,5,10)(2,12,16)(3,18,22)(4,24,26)
   res_ptr = ConstraintSolver(EndDoc, q1);
   ::vector<Match*> result1 = *res_ptr;
   for (unsigned i = 0; i < result1.size(); ++i)
      {
      cout << result1[i]->id<<' '<<result1[i]->start<<' ' <<result1[i]->end<< endl;
      delete result1[i];
      }
   delete res_ptr;


// query 2: "quick brown quick"
   ISR *terms_q2[] = {word_quick, word_brown, word_quick};
   ISRPhrase *q2 = new ISRPhrase(terms_q2, 3);

   cout << "q2 Results:" << endl;
   cout<<"ISRPhrase Seek tests:"<<endl;
   res = q2->Seek(0);
   ASSERT(res->GetStartLocation(), ==, 5);
   ASSERT(res->GetEndLocation(), ==, 7);
   res = q2->Seek(4);
   ASSERT(res->GetStartLocation(), ==, 5);
   ASSERT(res->GetEndLocation(), ==, 7);
   res = q2->Seek(6);
   ASSERT(res->GetStartLocation(), ==, 7);
   ASSERT(res->GetEndLocation(), ==, 9);
   res = q2->Seek(7);
   ASSERT(res->GetStartLocation(), ==, 7);
   ASSERT(res->GetEndLocation(), ==, 9);
   res = q2->Seek(9);
   ASSERT(res->GetStartLocation(), ==, 19);
   ASSERT(res->GetEndLocation(), ==, 21);
   res = q2->Seek(20);
   ASSERT(res, ==, nullptr);

   cout<<"constraint solver output: (see comment under line " << __LINE__ << " for correct output)"<<endl;
   // [1,3]
   res_ptr = ConstraintSolver(EndDoc, q2);
   ::vector<Match*> result2 = *res_ptr;
   for (unsigned i = 0; i < result2.size(); ++i)
      {
      cout << result2[i]->id<<' '<<result2[i]->start<<' ' <<result2[i]->end<< endl;
      delete result2[i];
      }
   delete res_ptr;
      

// query 3: quick fox
   cout << "q3 Results:" << endl;
   ISR *terms_q3[] = {word_quick, word_fox};
   ISRAnd *q3 = new ISRAnd(terms_q3, 2, &dict);

   cout<<"ISRAnd Seek tests:"<<endl;
   res = q3->Seek(0);
   ASSERT(res->GetStartLocation(), ==, 0);
   ASSERT(res->GetEndLocation(), ==, 2);
   res = q3->Seek(4);
   ASSERT(res->GetStartLocation(), ==, 12);
   ASSERT(res->GetEndLocation(), ==, 14);
   res = q3->Seek(12);
   ASSERT(res->GetStartLocation(), ==, 12);
   ASSERT(res->GetEndLocation(), ==, 14);
   res = q3->Seek(13);
   ASSERT(res->GetStartLocation(), ==, 14);
   ASSERT(res->GetEndLocation(), ==, 15);
   res = q3->Seek(15);
   ASSERT(res->GetStartLocation(), ==, 18);
   ASSERT(res->GetEndLocation(), ==, 19);
   res = q3->Seek(19);
   ASSERT(res, ==, nullptr);
   res = q3->Seek(29);
   ASSERT(res, ==, nullptr);

   cout<<"constraint solver output: (see comment under line " << __LINE__ << " for correct output)"<<endl;
   // (0,0,3)(2,12,16)(3,18,22)
   res_ptr = ConstraintSolver(EndDoc, q3);
   ::vector<Match*> result3 = *res_ptr;
   for (unsigned i = 0; i < result3.size(); ++i)
      {
      cout << result3[i]->id<<' '<<result3[i]->start<<' ' <<result3[i]->end<< endl;
      delete result3[i];
      }
   delete res_ptr;

// // query 4: quick
//    ISR *terms_q4[] = {word_quick};
//    ISROr *q4 = new ISROr(terms_q4, 1);
//    ::vector<Post*> result4 = *ConstraintSolver(EndDoc, q4);
//    cout << "q4 Results:" << endl;
//    for (unsigned i = 0; i < result4.size(); ++i)
//       {
//       cout << result4[i]->GetStartLocation() << " " << result4[i]->GetEndLocation() << endl;
//       delete result4[i];
//       }
   // delete(result1);
   // delete(result2);
   // delete(result3);
   // delete(q1);
   // delete (q2);
   // delete (q3);
   //delete (q4);

   delete (word_quick);
   delete (word_brown);
   delete (word_fox);

   return 0;
   }
