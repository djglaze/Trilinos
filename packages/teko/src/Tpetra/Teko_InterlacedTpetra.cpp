/*
// @HEADER
// 
// ***********************************************************************
// 
//      Teko: A package for block and physics based preconditioning
//                  Copyright 2010 Sandia Corporation 
//  
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//  
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//  
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//  
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//  
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission. 
//  
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  
// Questions? Contact Eric C. Cyr (eccyr@sandia.gov)
// 
// ***********************************************************************
// 
// @HEADER

*/

#include "Teko_InterlacedTpetra.hpp"
#include "Tpetra_Import.hpp"

#include <vector>

using Teuchos::RCP;
using Teuchos::rcp;

namespace Teko {
namespace TpetraHelpers {
namespace Strided {

// this assumes that there are numGlobals with numVars each interlaced
// i.e. for numVars = 2 (u,v) then the vector is
//    [u_0,v_0,u_1,v_1,u_2,v_2, ..., u_(numGlobals-1),v_(numGlobals-1)]
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void buildSubMaps(GO numGlobals,int numVars,const Teuchos::Comm<int> & comm,std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps)
#else
void buildSubMaps(GO numGlobals,int numVars,const Teuchos::Comm<int> & comm,std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > > & subMaps)
#endif
{
   std::vector<int> vars;
   
   // build vector describing the sub maps
   for(int i=0;i<numVars;i++) vars.push_back(1);

   // build all the submaps
   buildSubMaps(numGlobals,vars,comm,subMaps);
}

// build maps to make other conversions
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void buildSubMaps(const Tpetra::Map<LO,GO,NT> & globalMap,const std::vector<int> & vars,const Teuchos::Comm<int> & comm,
                  std::vector<std::pair<int,Teuchos::RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps)
#else
void buildSubMaps(const Tpetra::Map<NT> & globalMap,const std::vector<int> & vars,const Teuchos::Comm<int> & comm,
                  std::vector<std::pair<int,Teuchos::RCP<Tpetra::Map<NT> > > > & subMaps)
#endif
{
   buildSubMaps(globalMap.getGlobalNumElements(),globalMap.getNodeNumElements(),globalMap.getMinGlobalIndex(),
                vars,comm,subMaps);
}

// build maps to make other conversions
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void buildSubMaps(GO numGlobals,const std::vector<int> & vars,const Teuchos::Comm<int> & comm,std::vector<std::pair<int,Teuchos::RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps)
#else
void buildSubMaps(GO numGlobals,const std::vector<int> & vars,const Teuchos::Comm<int> & comm,std::vector<std::pair<int,Teuchos::RCP<Tpetra::Map<NT> > > > & subMaps)
#endif
{
   std::vector<int>::const_iterator varItr;

   // compute total number of variables
   int numGlobalVars = 0;
   for(varItr=vars.begin();varItr!=vars.end();++varItr)
      numGlobalVars += *varItr;

   // must be an even number of globals
   TEUCHOS_ASSERT((numGlobals%numGlobalVars)==0);

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   Tpetra::Map<LO,GO,NT> sampleMap(numGlobals/numGlobalVars,0,rcpFromRef(comm));
#else
   Tpetra::Map<NT> sampleMap(numGlobals/numGlobalVars,0,rcpFromRef(comm));
#endif

   buildSubMaps(numGlobals,numGlobalVars*sampleMap.getNodeNumElements(),numGlobalVars*sampleMap.getMinGlobalIndex(),vars,comm,subMaps);
}

// build maps to make other conversions
void buildSubMaps(GO numGlobals,LO numMyElements,GO minMyGID,const std::vector<int> & vars,const Teuchos::Comm<int> & comm,
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
                  std::vector<std::pair<int,Teuchos::RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps)
#else
                  std::vector<std::pair<int,Teuchos::RCP<Tpetra::Map<NT> > > > & subMaps)
#endif
{
   std::vector<int>::const_iterator varItr;

   // compute total number of variables
   int numGlobalVars = 0;
   for(varItr=vars.begin();varItr!=vars.end();++varItr)
      numGlobalVars += *varItr;

   // must be an even number of globals
   TEUCHOS_ASSERT((numGlobals%numGlobalVars)==0);
   TEUCHOS_ASSERT((numMyElements%numGlobalVars)==0);
   TEUCHOS_ASSERT((minMyGID%numGlobalVars)==0);

   LO numBlocks  = numMyElements/numGlobalVars;
   GO minBlockID = minMyGID/numGlobalVars;

   subMaps.clear();

   // index into local block in strided map
   GO blockOffset = 0;
   for(varItr=vars.begin();varItr!=vars.end();++varItr) {
      LO numLocalVars = *varItr;
      GO numAllElmts = numLocalVars*numGlobals/numGlobalVars;
#ifndef NDEBUG
      LO numMyElmts = numLocalVars * numBlocks;
#endif

      // create global arrays describing the as of yet uncreated maps
      std::vector<GO> subGlobals;
      std::vector<GO> contigGlobals; // the contiguous globals

      // loop over each block of variables
      LO count = 0;
      for(LO blockNum=0;blockNum<numBlocks;blockNum++) {

         // loop over each local variable in the block
         for(LO local=0;local<numLocalVars;++local) {
            // global block number = minGID+blockNum 
            // block begin global id = numGlobalVars*(minGID+blockNum)
            // global id block offset = blockOffset+local
            subGlobals.push_back((minBlockID+blockNum)*numGlobalVars+blockOffset+local);

            // also build the contiguous IDs
            contigGlobals.push_back(numLocalVars*minBlockID+count);
            count++;
         }
      }

      // sanity check
      assert((size_t) numMyElmts==subGlobals.size());

      // create the map with contiguous elements and the map with global elements
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      RCP<Tpetra::Map<LO,GO,NT> > subMap = rcp(new Tpetra::Map<LO,GO,NT>(numAllElmts,Teuchos::ArrayView<GO>(subGlobals),0,rcpFromRef(comm)));
      RCP<Tpetra::Map<LO,GO,NT> > contigMap = rcp(new Tpetra::Map<LO,GO,NT>(numAllElmts,Teuchos::ArrayView<GO>(contigGlobals),0,rcpFromRef(comm)));
#else
      RCP<Tpetra::Map<NT> > subMap = rcp(new Tpetra::Map<NT>(numAllElmts,Teuchos::ArrayView<GO>(subGlobals),0,rcpFromRef(comm)));
      RCP<Tpetra::Map<NT> > contigMap = rcp(new Tpetra::Map<NT>(numAllElmts,Teuchos::ArrayView<GO>(contigGlobals),0,rcpFromRef(comm)));
#endif

      Teuchos::set_extra_data(contigMap,"contigMap",Teuchos::inOutArg(subMap));
      subMaps.push_back(std::make_pair(numLocalVars,subMap));

      // update the block offset
      blockOffset += numLocalVars;
   }
}

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void buildExportImport(const Tpetra::Map<LO,GO,NT> & baseMap, const std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps,
                       std::vector<RCP<Tpetra::Export<LO,GO,NT> > > & subExport,
                       std::vector<RCP<Tpetra::Import<LO,GO,NT> > > & subImport)
#else
void buildExportImport(const Tpetra::Map<NT> & baseMap, const std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > > & subMaps,
                       std::vector<RCP<Tpetra::Export<NT> > > & subExport,
                       std::vector<RCP<Tpetra::Import<NT> > > & subImport)
#endif
{
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > >::const_iterator mapItr;
#else
   std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > >::const_iterator mapItr;
#endif

   // build importers and exporters
   for(mapItr=subMaps.begin();mapItr!=subMaps.end();++mapItr) {
      // exctract basic map
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      const Tpetra::Map<LO,GO,NT> & map = *(mapItr->second);
#else
      const Tpetra::Map<NT> & map = *(mapItr->second);
#endif

      // add new elements to vectors
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      subImport.push_back(rcp(new Tpetra::Import<LO,GO,NT>(rcpFromRef(baseMap),rcpFromRef(map))));
      subExport.push_back(rcp(new Tpetra::Export<LO,GO,NT>(rcpFromRef(map),rcpFromRef(baseMap))));
#else
      subImport.push_back(rcp(new Tpetra::Import<NT>(rcpFromRef(baseMap),rcpFromRef(map))));
      subExport.push_back(rcp(new Tpetra::Export<NT>(rcpFromRef(map),rcpFromRef(baseMap))));
#endif
   }
}

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void buildSubVectors(const std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps,std::vector<RCP<Tpetra::MultiVector<ST,LO,GO,NT> > > & subVectors,int count)
#else
void buildSubVectors(const std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > > & subMaps,std::vector<RCP<Tpetra::MultiVector<ST,NT> > > & subVectors,int count)
#endif
{
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > >::const_iterator mapItr;
#else
   std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > >::const_iterator mapItr;
#endif

   // build vectors
   for(mapItr=subMaps.begin();mapItr!=subMaps.end();++mapItr) {
      // exctract basic map
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      const Tpetra::Map<LO,GO,NT> & map = *(Teuchos::get_extra_data<RCP<Tpetra::Map<LO,GO,NT> > >(mapItr->second,"contigMap"));
#else
      const Tpetra::Map<NT> & map = *(Teuchos::get_extra_data<RCP<Tpetra::Map<NT> > >(mapItr->second,"contigMap"));
#endif

      // add new elements to vectors
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      RCP<Tpetra::MultiVector<ST,LO,GO,NT> > mv = rcp(new Tpetra::MultiVector<ST,LO,GO,NT>(rcpFromRef(map),count));
#else
      RCP<Tpetra::MultiVector<ST,NT> > mv = rcp(new Tpetra::MultiVector<ST,NT>(rcpFromRef(map),count));
#endif
      Teuchos::set_extra_data(mapItr->second,"globalMap",Teuchos::inOutArg(mv)); 
      subVectors.push_back(mv);
   }
}

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void associateSubVectors(const std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps,std::vector<RCP<const Tpetra::MultiVector<ST,LO,GO,NT> > > & subVectors)
#else
void associateSubVectors(const std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > > & subMaps,std::vector<RCP<const Tpetra::MultiVector<ST,NT> > > & subVectors)
#endif
{
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > >::const_iterator mapItr;
   std::vector<RCP<const Tpetra::MultiVector<ST,LO,GO,NT> > >::iterator vecItr;
#else
   std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > >::const_iterator mapItr;
   std::vector<RCP<const Tpetra::MultiVector<ST,NT> > >::iterator vecItr;
#endif

   TEUCHOS_ASSERT(subMaps.size()==subVectors.size());

   // associate the sub vectors with the subMaps
   for(mapItr=subMaps.begin(),vecItr=subVectors.begin();mapItr!=subMaps.end();++mapItr,++vecItr)
      Teuchos::set_extra_data(mapItr->second,"globalMap",Teuchos::inOutArg(*vecItr),Teuchos::POST_DESTROY,false);
}

// build a single subblock Epetra_CrsMatrix
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
RCP<Tpetra::CrsMatrix<ST,LO,GO,NT> > buildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,LO,GO,NT> >& A,const std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps)
#else
RCP<Tpetra::CrsMatrix<ST,NT> > buildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,NT> >& A,const std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > > & subMaps)
#endif
{
   // get the number of variables families
   int numVarFamily = subMaps.size();

   TEUCHOS_ASSERT(i>=0 && i<numVarFamily);
   TEUCHOS_ASSERT(j>=0 && j<numVarFamily);

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   const Tpetra::Map<LO,GO,NT> & gRowMap = *subMaps[i].second;
   const Tpetra::Map<LO,GO,NT> & rowMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<LO,GO,NT> > >(subMaps[i].second,"contigMap");
   const Tpetra::Map<LO,GO,NT> & colMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<LO,GO,NT> > >(subMaps[j].second,"contigMap");
#else
   const Tpetra::Map<NT> & gRowMap = *subMaps[i].second;
   const Tpetra::Map<NT> & rowMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<NT> > >(subMaps[i].second,"contigMap");
   const Tpetra::Map<NT> & colMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<NT> > >(subMaps[j].second,"contigMap");
#endif
   int colFamilyCnt = subMaps[j].first;

   // compute the number of global variables
   // and the row and column block offset
   GO numGlobalVars = 0;
   GO rowBlockOffset = 0;
   GO colBlockOffset = 0;
   for(int k=0;k<numVarFamily;k++) {
      numGlobalVars += subMaps[k].first;
 
      // compute block offsets
      if(k<i) rowBlockOffset += subMaps[k].first;
      if(k<j) colBlockOffset += subMaps[k].first;
   }

   // copy all global rows to here
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   Tpetra::Import<LO,GO,NT> import(A->getRowMap(),rcpFromRef(gRowMap));
   Tpetra::CrsMatrix<ST,LO,GO,NT> localA(rcpFromRef(gRowMap),0);
#else
   Tpetra::Import<NT> import(A->getRowMap(),rcpFromRef(gRowMap));
   Tpetra::CrsMatrix<ST,NT> localA(rcpFromRef(gRowMap),0);
#endif
   localA.doImport(*A,import,Tpetra::INSERT);

   // get entry information
   LO numMyRows = rowMap.getNodeNumElements();
   LO maxNumEntries = A->getGlobalMaxNumRowEntries();

   // for extraction
   std::vector<GO> indices(maxNumEntries);
   std::vector<ST> values(maxNumEntries);

   // for counting row sizes
   std::vector<size_t> numEntriesPerRow(numMyRows,0);

   const size_t invalid = Teuchos::OrdinalTraits<size_t>::invalid();

   // Count the sizes of each row, using same logic as insertion below
   for(LO localRow=0;localRow<numMyRows;localRow++) {
      size_t numEntries = invalid;
      GO globalRow = gRowMap.getGlobalElement(localRow);
      GO contigRow = rowMap.getGlobalElement(localRow);

      TEUCHOS_ASSERT(globalRow>=0);
      TEUCHOS_ASSERT(contigRow>=0);

      // extract a global row copy
      localA.getGlobalRowCopy(globalRow, Teuchos::ArrayView<GO>(indices), Teuchos::ArrayView<ST>(values), numEntries);
      LO numOwnedCols = 0;
      for(size_t localCol=0;localCol<numEntries;localCol++) {
         GO globalCol = indices[localCol];

         // determinate which block this column ID is in
         int block = globalCol / numGlobalVars;
         
         bool inFamily = true; 
 
         // test the beginning of the block
         inFamily &= (block*numGlobalVars+colBlockOffset <= globalCol);
         inFamily &= ((block*numGlobalVars+colBlockOffset+colFamilyCnt) > globalCol);

         // is this column in the variable family
         if(inFamily) {
            numOwnedCols++;
         }
      }
      numEntriesPerRow[localRow] += numOwnedCols;
   }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   RCP<Tpetra::CrsMatrix<ST,LO,GO,NT> > mat = 
      rcp(new Tpetra::CrsMatrix<ST,LO,GO,NT>(rcpFromRef(rowMap), Teuchos::ArrayView<const size_t>(numEntriesPerRow)));
#else
   RCP<Tpetra::CrsMatrix<ST,NT> > mat = 
      rcp(new Tpetra::CrsMatrix<ST,NT>(rcpFromRef(rowMap), Teuchos::ArrayView<const size_t>(numEntriesPerRow)));
#endif

   // for insertion
   std::vector<GO> colIndices(maxNumEntries);
   std::vector<ST> colValues(maxNumEntries);

   // insert each row into subblock
   // let FillComplete handle column distribution
   for(LO localRow=0;localRow<numMyRows;localRow++) {
      size_t numEntries = invalid;
      GO globalRow = gRowMap.getGlobalElement(localRow);
      GO contigRow = rowMap.getGlobalElement(localRow);

      TEUCHOS_ASSERT(globalRow>=0);
      TEUCHOS_ASSERT(contigRow>=0);

      // extract a global row copy
      localA.getGlobalRowCopy(globalRow, Teuchos::ArrayView<GO>(indices), Teuchos::ArrayView<ST>(values), numEntries);
      LO numOwnedCols = 0;
      for(size_t localCol=0;localCol<numEntries;localCol++) {
         GO globalCol = indices[localCol];

         // determinate which block this column ID is in
         int block = globalCol / numGlobalVars;
         
         bool inFamily = true; 
 
         // test the beginning of the block
         inFamily &= (block*numGlobalVars+colBlockOffset <= globalCol);
         inFamily &= ((block*numGlobalVars+colBlockOffset+colFamilyCnt) > globalCol);

         // is this column in the variable family
         if(inFamily) {
            GO familyOffset = globalCol-(block*numGlobalVars+colBlockOffset);

            colIndices[numOwnedCols] = block*colFamilyCnt + familyOffset;
            colValues[numOwnedCols] = values[localCol];

            numOwnedCols++;
         }
      }

      // insert it into the new matrix
      colIndices.resize(numOwnedCols);
      colValues.resize(numOwnedCols);
      mat->insertGlobalValues(contigRow,Teuchos::ArrayView<GO>(colIndices),Teuchos::ArrayView<ST>(colValues));
      colIndices.resize(maxNumEntries);
      colValues.resize(maxNumEntries);
   }

   // fill it and automagically optimize the storage
   mat->fillComplete(rcpFromRef(colMap),rcpFromRef(rowMap));

   return mat;
}

// rebuild a single subblock Epetra_CrsMatrix
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void rebuildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,LO,GO,NT> > & A,const std::vector<std::pair<int,RCP<Tpetra::Map<LO,GO,NT> > > > & subMaps,Tpetra::CrsMatrix<ST,LO,GO,NT> & mat)
#else
void rebuildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,NT> > & A,const std::vector<std::pair<int,RCP<Tpetra::Map<NT> > > > & subMaps,Tpetra::CrsMatrix<ST,NT> & mat)
#endif
{
   // get the number of variables families
   int numVarFamily = subMaps.size();

   TEUCHOS_ASSERT(i>=0 && i<numVarFamily);
   TEUCHOS_ASSERT(j>=0 && j<numVarFamily);
   TEUCHOS_ASSERT(mat.isFillComplete());

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   const Tpetra::Map<LO,GO,NT> & gRowMap = *subMaps[i].second;
   const Tpetra::Map<LO,GO,NT> & rowMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<LO,GO,NT> > >(subMaps[i].second,"contigMap");
   const Tpetra::Map<LO,GO,NT> & colMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<LO,GO,NT> > >(subMaps[j].second,"contigMap");
#else
   const Tpetra::Map<NT> & gRowMap = *subMaps[i].second;
   const Tpetra::Map<NT> & rowMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<NT> > >(subMaps[i].second,"contigMap");
   const Tpetra::Map<NT> & colMap = *Teuchos::get_extra_data<RCP<Tpetra::Map<NT> > >(subMaps[j].second,"contigMap");
#endif
   int colFamilyCnt = subMaps[j].first;

   // compute the number of global variables
   // and the row and column block offset
   GO numGlobalVars = 0;
   GO rowBlockOffset = 0;
   GO colBlockOffset = 0;
   for(int k=0;k<numVarFamily;k++) {
      numGlobalVars += subMaps[k].first;
 
      // compute block offsets
      if(k<i) rowBlockOffset += subMaps[k].first;
      if(k<j) colBlockOffset += subMaps[k].first;
   }

   // copy all global rows to here
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   Tpetra::Import<LO,GO,NT> import(A->getRowMap(),rcpFromRef(gRowMap));
   Tpetra::CrsMatrix<ST,LO,GO,NT> localA(rcpFromRef(gRowMap),0);
#else
   Tpetra::Import<NT> import(A->getRowMap(),rcpFromRef(gRowMap));
   Tpetra::CrsMatrix<ST,NT> localA(rcpFromRef(gRowMap),0);
#endif
   localA.doImport(*A,import,Tpetra::INSERT);

   // clear out the old matrix
   mat.resumeFill();
   mat.setAllToScalar(0.0);

   // get entry information
   LO numMyRows = rowMap.getNodeNumElements();
   GO maxNumEntries = A->getGlobalMaxNumRowEntries();

   // for extraction
   std::vector<GO> indices(maxNumEntries);
   std::vector<ST> values(maxNumEntries);

   // for insertion
   std::vector<GO> colIndices(maxNumEntries);
   std::vector<ST> colValues(maxNumEntries);

   const size_t invalid = Teuchos::OrdinalTraits<size_t>::invalid();

   // insert each row into subblock
   // let FillComplete handle column distribution
   for(LO localRow=0;localRow<numMyRows;localRow++) {
      size_t numEntries = invalid;
      GO globalRow = gRowMap.getGlobalElement(localRow);
      GO contigRow = rowMap.getGlobalElement(localRow);

      TEUCHOS_ASSERT(globalRow>=0);
      TEUCHOS_ASSERT(contigRow>=0);

      // extract a global row copy
      localA.getGlobalRowCopy(globalRow, Teuchos::ArrayView<GO>(indices), Teuchos::ArrayView<ST>(values), numEntries);

      LO numOwnedCols = 0;
      for(size_t localCol=0;localCol<numEntries;localCol++) {
         GO globalCol = indices[localCol];

         // determinate which block this column ID is in
         int block = globalCol / numGlobalVars;
         
         bool inFamily = true; 
 
         // test the beginning of the block
         inFamily &= (block*numGlobalVars+colBlockOffset <= globalCol);
         inFamily &= ((block*numGlobalVars+colBlockOffset+colFamilyCnt) > globalCol);

         // is this column in the variable family
         if(inFamily) {
            GO familyOffset = globalCol-(block*numGlobalVars+colBlockOffset);

            colIndices[numOwnedCols] = block*colFamilyCnt + familyOffset;
            colValues[numOwnedCols] = values[localCol];

            numOwnedCols++;
         }
      }

      // insert it into the new matrix
      colIndices.resize(numOwnedCols);
      colValues.resize(numOwnedCols);
      mat.sumIntoGlobalValues(contigRow,Teuchos::ArrayView<GO>(colIndices),Teuchos::ArrayView<ST>(colValues));
      colIndices.resize(maxNumEntries);
      colValues.resize(maxNumEntries);
   }
   mat.fillComplete(rcpFromRef(colMap),rcpFromRef(rowMap));
}


// collect subvectors into a single global vector
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void many2one(Tpetra::MultiVector<ST,LO,GO,NT> & one, const std::vector<RCP<const Tpetra::MultiVector<ST,LO,GO,NT> > > & many,
                                   const std::vector<RCP<Tpetra::Export<LO,GO,NT> > > & subExport)
#else
void many2one(Tpetra::MultiVector<ST,NT> & one, const std::vector<RCP<const Tpetra::MultiVector<ST,NT> > > & many,
                                   const std::vector<RCP<Tpetra::Export<NT> > > & subExport)
#endif
{
   // std::vector<RCP<const Epetra_Vector> >::const_iterator vecItr;
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   std::vector<RCP<const Tpetra::MultiVector<ST,LO,GO,NT> > >::const_iterator vecItr;
   std::vector<RCP<Tpetra::Export<LO,GO,NT> > >::const_iterator expItr;
#else
   std::vector<RCP<const Tpetra::MultiVector<ST,NT> > >::const_iterator vecItr;
   std::vector<RCP<Tpetra::Export<NT> > >::const_iterator expItr;
#endif

   // using Exporters fill the empty vector from the sub-vectors
   for(vecItr=many.begin(),expItr=subExport.begin();
       vecItr!=many.end();++vecItr,++expItr) {

      // for ease of access to the source
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      RCP<const Tpetra::MultiVector<ST,LO,GO,NT> > srcVec = *vecItr;
#else
      RCP<const Tpetra::MultiVector<ST,NT> > srcVec = *vecItr;
#endif

      // extract the map with global indicies from the current vector
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      const Tpetra::Map<LO,GO,NT> & globalMap = *(Teuchos::get_extra_data<RCP<Tpetra::Map<LO,GO,NT> > >(srcVec,"globalMap"));
#else
      const Tpetra::Map<NT> & globalMap = *(Teuchos::get_extra_data<RCP<Tpetra::Map<NT> > >(srcVec,"globalMap"));
#endif

      // build the export vector as a view of the destination
      GO lda = srcVec->getStride();
      GO srcSize = srcVec->getGlobalLength()*srcVec->getNumVectors();
      std::vector<ST> srcArray(srcSize);
      Teuchos::ArrayView<ST> srcVals(srcArray);
      srcVec->get1dCopy(srcVals,lda);
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      Tpetra::MultiVector<ST,LO,GO,NT> exportVector(rcpFromRef(globalMap),srcVals,lda,srcVec->getNumVectors());
#else
      Tpetra::MultiVector<ST,NT> exportVector(rcpFromRef(globalMap),srcVals,lda,srcVec->getNumVectors());
#endif

      // perform the export
      one.doExport(exportVector,**expItr,Tpetra::INSERT);
   }
}

// distribute one global vector into a many subvectors
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void one2many(std::vector<RCP<Tpetra::MultiVector<ST,LO,GO,NT> > > & many,const Tpetra::MultiVector<ST,LO,GO,NT> & single,
                                   const std::vector<RCP<Tpetra::Import<LO,GO,NT> > > & subImport)
#else
void one2many(std::vector<RCP<Tpetra::MultiVector<ST,NT> > > & many,const Tpetra::MultiVector<ST,NT> & single,
                                   const std::vector<RCP<Tpetra::Import<NT> > > & subImport)
#endif
{
   // std::vector<RCP<Epetra_Vector> >::const_iterator vecItr;
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   std::vector<RCP<Tpetra::MultiVector<ST,LO,GO,NT> > >::const_iterator vecItr;
   std::vector<RCP<Tpetra::Import<LO,GO,NT> > >::const_iterator impItr;
#else
   std::vector<RCP<Tpetra::MultiVector<ST,NT> > >::const_iterator vecItr;
   std::vector<RCP<Tpetra::Import<NT> > >::const_iterator impItr;
#endif

   // using Importers fill the sub vectors from the mama vector
   for(vecItr=many.begin(),impItr=subImport.begin();
       vecItr!=many.end();++vecItr,++impItr) {
      // for ease of access to the destination
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      RCP<Tpetra::MultiVector<ST,LO,GO,NT> > destVec = *vecItr;
#else
      RCP<Tpetra::MultiVector<ST,NT> > destVec = *vecItr;
#endif

      // extract the map with global indicies from the current vector
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      const Tpetra::Map<LO,GO,NT> & globalMap = *(Teuchos::get_extra_data<RCP<Tpetra::Map<LO,GO,NT> > >(destVec,"globalMap"));
#else
      const Tpetra::Map<NT> & globalMap = *(Teuchos::get_extra_data<RCP<Tpetra::Map<NT> > >(destVec,"globalMap"));
#endif

      // build the import vector as a view on the destination
      GO destLDA = destVec->getStride();
      GO destSize = destVec->getGlobalLength()*destVec->getNumVectors();
      std::vector<ST> destArray(destSize);
      Teuchos::ArrayView<ST> destVals(destArray);
      destVec->get1dCopy(destVals,destLDA);
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      Tpetra::MultiVector<ST,LO,GO,NT> importVector(rcpFromRef(globalMap),destVals,destLDA,destVec->getNumVectors());
#else
      Tpetra::MultiVector<ST,NT> importVector(rcpFromRef(globalMap),destVals,destLDA,destVec->getNumVectors());
#endif

      // perform the import
      importVector.doImport(single,**impItr,Tpetra::INSERT);

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      Tpetra::Import<LO,GO,NT> importer(destVec->getMap(),destVec->getMap());
#else
      Tpetra::Import<NT> importer(destVec->getMap(),destVec->getMap());
#endif
      importVector.replaceMap(destVec->getMap());
      destVec->doImport(importVector,importer,Tpetra::INSERT);

   }
}

}
} // end namespace Tpetra 
} // end namespace Teko
