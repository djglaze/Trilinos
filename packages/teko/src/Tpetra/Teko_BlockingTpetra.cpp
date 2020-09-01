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

#include "Teko_BlockingTpetra.hpp"
#include "Teko_Utilities.hpp"

#include "Tpetra_Vector.hpp"
#include "Tpetra_Map.hpp"

using Teuchos::RCP;
using Teuchos::rcp;

namespace Teko {
namespace TpetraHelpers {
namespace Blocking {

/** Build maps to make other conversions. This function builds a map 
  * using a vector of global ids local to this processor.  It also builds
  * a seperate map that (globally) starts from zero. For instance if the following
  * GIDs are passed in PID = 0, GID = [0 2 4 6 8] and PID = 1, GID = [10 12 14]
  * the the two maps created are
  *    Global Map = [(PID=0,GID=[0 2 4 6 8]), (PID=1,GID=[10 12 14])]
  *    Contiguous Map = [(PID=0,GID=[0 1 2 3 4]), (PID=1,GID=[5 6 7])]
  *
  * \param[in] gid Local global IDs to use
  * \param[in] comm Communicator to use in construction of the maps
  *
  * \returns A pair of maps: (Global Map, Contiguous Map)
  */
const MapPair buildSubMap(const std::vector< GO > & gid, const Teuchos::Comm<int> &comm)
{
   using GST = Tpetra::global_size_t;
   const GST invalid = Teuchos::OrdinalTraits<GST>::invalid();
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   Teuchos::RCP<Tpetra::Map<LO,GO,NT> > gidMap = rcp(new Tpetra::Map<LO,GO,NT>(invalid,Teuchos::ArrayView<const GO>(gid),0,rcpFromRef(comm)));
   Teuchos::RCP<Tpetra::Map<LO,GO,NT> > contigMap = rcp(new Tpetra::Map<LO,GO,NT>(invalid,gid.size(),0,rcpFromRef(comm)));
#else
   Teuchos::RCP<Tpetra::Map<NT> > gidMap = rcp(new Tpetra::Map<NT>(invalid,Teuchos::ArrayView<const GO>(gid),0,rcpFromRef(comm)));
   Teuchos::RCP<Tpetra::Map<NT> > contigMap = rcp(new Tpetra::Map<NT>(invalid,gid.size(),0,rcpFromRef(comm)));
#endif

   return std::make_pair(gidMap,contigMap); 
}

/** Build the Export/Import objects that take the single vector global map and 
  * build individual sub maps.
  *
  * \param[in] baseMap Single global vector map.
  * \param[in] maps Pair of maps containing the global sub map and the contiguous sub map.
  *                 These come directly from <code>buildSubMap</code>.
  *
  * \returns A pair containing pointers to the Import/Export objects.
  */
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
const ImExPair buildExportImport(const Tpetra::Map<LO,GO,NT> & baseMap,const MapPair & maps)
#else
const ImExPair buildExportImport(const Tpetra::Map<NT> & baseMap,const MapPair & maps)
#endif
{
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   return std::make_pair(rcp(new Tpetra::Import<LO,GO,NT>(rcpFromRef(baseMap),maps.first)),
                         rcp(new Tpetra::Export<LO,GO,NT>(maps.first,rcpFromRef(baseMap))));
#else
   return std::make_pair(rcp(new Tpetra::Import<NT>(rcpFromRef(baseMap),maps.first)),
                         rcp(new Tpetra::Export<NT>(maps.first,rcpFromRef(baseMap))));
#endif
}

/** Using a list of map pairs created by <code>buildSubMap</code>, buidl the corresponding
  * multi-vector objects.
  *
  * \param[in] maps Map pairs created by <code>buildSubMap</code>
  * \param[in,out] vectors Vector objects created using the Contiguous maps
  * \param[in] count Number of multivectors to build.
  */
void buildSubVectors(const std::vector<MapPair> & maps,
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
                     std::vector<RCP<Tpetra::MultiVector<ST,LO,GO,NT> > > & vectors,int count)
#else
                     std::vector<RCP<Tpetra::MultiVector<ST,NT> > > & vectors,int count)
#endif
{
   std::vector<MapPair>::const_iterator mapItr;
   
   // loop over all maps
   for(mapItr=maps.begin();mapItr!=maps.end();mapItr++) {
      // add new elements to vectors
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      RCP<Tpetra::MultiVector<ST,LO,GO,NT> > mv = rcp(new Tpetra::MultiVector<ST,LO,GO,NT>((*mapItr).second,count));
#else
      RCP<Tpetra::MultiVector<ST,NT> > mv = rcp(new Tpetra::MultiVector<ST,NT>((*mapItr).second,count));
#endif
      vectors.push_back(mv);
   } 
}

/** Copy the contents of a global vector into many sub-vectors created by <code>buildSubVectors</code>.
  *
  * \param[in,out] many Sub-vector to be filled by this operation created by <code>buildSubVectors</code>.
  * \param[in] one The source vector.
  * \param[in] subImport A list of import objects to use in copying.
  */
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void one2many(std::vector<RCP<Tpetra::MultiVector<ST,LO,GO,NT> > > & many, const Tpetra::MultiVector<ST,LO,GO,NT> & single,
                                                            const std::vector<RCP<Tpetra::Import<LO,GO,NT> > > & subImport)
#else
void one2many(std::vector<RCP<Tpetra::MultiVector<ST,NT> > > & many, const Tpetra::MultiVector<ST,NT> & single,
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
      const Tpetra::Map<LO,GO,NT> & globalMap = *(*impItr)->getTargetMap();
#else
      const Tpetra::Map<NT> & globalMap = *(*impItr)->getTargetMap();
#endif

      // build the import vector as a view on the destination
      GO lda = destVec->getStride();
      GO destSize = destVec->getGlobalLength()*destVec->getNumVectors();
      std::vector<ST> destArray(destSize);
      Teuchos::ArrayView<ST> destVals(destArray);
      destVec->get1dCopy(destVals,lda);
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      Tpetra::MultiVector<ST,LO,GO,NT> importVector(rcpFromRef(globalMap),destVals,lda,destVec->getNumVectors());
#else
      Tpetra::MultiVector<ST,NT> importVector(rcpFromRef(globalMap),destVals,lda,destVec->getNumVectors());
#endif

      // perform the import
      importVector.doImport(single,**impItr,Tpetra::INSERT);

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      Tpetra::Import<LO,GO,NT> importer(destVec->getMap(),destVec->getMap());
#else
      Tpetra::Import<NT> importer(destVec->getMap(),destVec->getMap());
#endif
      importVector.replaceMap(destVec->getMap());
      destVec->doExport(importVector,importer,Tpetra::INSERT);
   }
}

/** Copy the contents of many sub vectors (created from a contigous sub maps) to a single global
  * vector. This should have the map used to create the Export/Import objects in the <code>buildExportImport</code>
  * function. If more then one sub vector contains values for a particular GID in the single vector
  * then the value in the final vector will be that of the last sub vector (in the list <code>many</code>).
  *
  * \param[in,out] one The single vector to be filled by this operation.
  * \param[in] many Sub-vectors created by <code>buildSubVectors</code> used to fill <code>one</code>.
  * \param[in] subExport A list of export objects to use in copying.
  */
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
      const Tpetra::Map<LO,GO,NT> & globalMap = *(*expItr)->getSourceMap();
#else
      const Tpetra::Map<NT> & globalMap = *(*expItr)->getSourceMap();
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

/** This function will return an IntVector that is constructed with a column map.
  * The vector will be filled with -1 if there is not a corresponding entry in the
  * sub-block row map. The other columns will be filled with the contiguous row map
  * values.
  */
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
RCP<Tpetra::Vector<GO,LO,GO,NT> > getSubBlockColumnGIDs(const Tpetra::CrsMatrix<ST,LO,GO,NT> & A,const MapPair & mapPair)
#else
RCP<Tpetra::Vector<GO,NT> > getSubBlockColumnGIDs(const Tpetra::CrsMatrix<ST,NT> & A,const MapPair & mapPair)
#endif
{
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   RCP<const Tpetra::Map<LO,GO,NT> > blkGIDMap = mapPair.first;
   RCP<const Tpetra::Map<LO,GO,NT> > blkContigMap = mapPair.second;
#else
   RCP<const Tpetra::Map<NT> > blkGIDMap = mapPair.first;
   RCP<const Tpetra::Map<NT> > blkContigMap = mapPair.second;
#endif

   // fill index vector for rows
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   Tpetra::Vector<GO,LO,GO,NT> rIndex(A.getRowMap(),true);
#else
   Tpetra::Vector<GO,NT> rIndex(A.getRowMap(),true);
#endif
   for(size_t i=0;i<A.getNodeNumRows();i++) {
      // LID is need to get to contiguous map
      LO lid = blkGIDMap->getLocalElement(A.getRowMap()->getGlobalElement(i)); // this LID makes me nervous
      if(lid>-1)
         rIndex.replaceLocalValue(i,blkContigMap->getGlobalElement(lid));
      else
         rIndex.replaceLocalValue(i,-1);
   }

   // get relavant column indices
   
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   Tpetra::Import<LO,GO,NT> import(A.getRowMap(),A.getColMap());
   RCP<Tpetra::Vector<GO,LO,GO,NT> > cIndex = rcp(new Tpetra::Vector<GO,LO,GO,NT>(A.getColMap(),true));
#else
   Tpetra::Import<NT> import(A.getRowMap(),A.getColMap());
   RCP<Tpetra::Vector<GO,NT> > cIndex = rcp(new Tpetra::Vector<GO,NT>(A.getColMap(),true));
#endif
   cIndex->doImport(rIndex,import,Tpetra::INSERT);

   return cIndex;
}

// build a single subblock Epetra_CrsMatrix
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
RCP<Tpetra::CrsMatrix<ST,LO,GO,NT> > buildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,LO,GO,NT> >& A,const std::vector<MapPair> & subMaps)
#else
RCP<Tpetra::CrsMatrix<ST,NT> > buildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,NT> >& A,const std::vector<MapPair> & subMaps)
#endif
{
   // get the number of variables families
   int numVarFamily = subMaps.size();

   TEUCHOS_ASSERT(i>=0 && i<numVarFamily);
   TEUCHOS_ASSERT(j>=0 && j<numVarFamily);
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   const RCP<const Tpetra::Map<LO,GO,NT> > gRowMap = subMaps[i].first; // new GIDs
   const RCP<const Tpetra::Map<LO,GO,NT> > rowMap = subMaps[i].second; // contiguous GIDs
   const RCP<const Tpetra::Map<LO,GO,NT> > colMap = subMaps[j].second;
#else
   const RCP<const Tpetra::Map<NT> > gRowMap = subMaps[i].first; // new GIDs
   const RCP<const Tpetra::Map<NT> > rowMap = subMaps[i].second; // contiguous GIDs
   const RCP<const Tpetra::Map<NT> > colMap = subMaps[j].second;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   const RCP<Tpetra::Vector<GO,LO,GO,NT> > plocal2ContigGIDs = getSubBlockColumnGIDs(*A,subMaps[j]);
   Tpetra::Vector<GO,LO,GO,NT> & local2ContigGIDs = *plocal2ContigGIDs;
#else
   const RCP<Tpetra::Vector<GO,NT> > plocal2ContigGIDs = getSubBlockColumnGIDs(*A,subMaps[j]);
   Tpetra::Vector<GO,NT> & local2ContigGIDs = *plocal2ContigGIDs;
#endif


   // get entry information
   LO numMyRows = rowMap->getNodeNumElements();
   LO maxNumEntries = A->getNodeMaxNumRowEntries();

   // for extraction
   std::vector<LO> indices(maxNumEntries);
   std::vector<ST> values(maxNumEntries);

   // for insertion
   std::vector<GO> colIndices(maxNumEntries);
   std::vector<ST> colValues(maxNumEntries);

   // for counting
   std::vector<size_t> nEntriesPerRow(numMyRows);

   const size_t invalid = Teuchos::OrdinalTraits<size_t>::invalid();

   // insert each row into subblock
   // Count the number of entries per row in the new matrix
   for(LO localRow=0;localRow<numMyRows;localRow++) {
      size_t numEntries = invalid;
      GO globalRow = gRowMap->getGlobalElement(localRow);
      LO lid = A->getRowMap()->getLocalElement(globalRow);
      TEUCHOS_ASSERT(lid>-1);

      A->getLocalRowCopy(lid, Teuchos::ArrayView<LO>(indices), Teuchos::ArrayView<ST>(values),numEntries);

      LO numOwnedCols = 0;
      for(size_t localCol=0;localCol<numEntries;localCol++) {
         TEUCHOS_ASSERT(indices[localCol]>-1);

         // if global id is not owned by this column

         GO gid = local2ContigGIDs.getData()[indices[localCol]];
         if(gid==-1) continue; // in contiguous row
         numOwnedCols++;
      }
      nEntriesPerRow[localRow] = numOwnedCols;
   }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   RCP<Tpetra::CrsMatrix<ST,LO,GO,NT> > mat = rcp(new Tpetra::CrsMatrix<ST,LO,GO,NT>(rowMap,Teuchos::ArrayView<size_t>(nEntriesPerRow)));
#else
   RCP<Tpetra::CrsMatrix<ST,NT> > mat = rcp(new Tpetra::CrsMatrix<ST,NT>(rowMap,Teuchos::ArrayView<size_t>(nEntriesPerRow)));
#endif

   // insert each row into subblock
   // let FillComplete handle column distribution
   for(LO localRow=0;localRow<numMyRows;localRow++) {
      size_t numEntries = invalid;
      GO globalRow = gRowMap->getGlobalElement(localRow);
      LO lid = A->getRowMap()->getLocalElement(globalRow);
      GO contigRow = rowMap->getGlobalElement(localRow);
      TEUCHOS_ASSERT(lid>-1);

      A->getLocalRowCopy(lid, Teuchos::ArrayView<LO>(indices), Teuchos::ArrayView<ST>(values),numEntries);

      LO numOwnedCols = 0;
      for(size_t localCol=0;localCol<numEntries;localCol++) {
         TEUCHOS_ASSERT(indices[localCol]>-1);

         // if global id is not owned by this column

         GO gid = local2ContigGIDs.getData()[indices[localCol]];
         if(gid==-1) continue; // in contiguous row

         colIndices[numOwnedCols] = gid;
         colValues[numOwnedCols] = values[localCol];
         numOwnedCols++;
      }

      // insert it into the new matrix
      colIndices.resize(numOwnedCols);
      colValues.resize(numOwnedCols);
      mat->insertGlobalValues(contigRow,Teuchos::ArrayView<GO>(colIndices),Teuchos::ArrayView<ST>(colValues));
      colIndices.resize(maxNumEntries);
      colValues.resize(maxNumEntries);
   }

   // fill it and automagically optimize the storage
   mat->fillComplete(colMap,rowMap);

   return mat;
}

// build a single subblock Epetra_CrsMatrix
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void rebuildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,LO,GO,NT> >& A,const std::vector<MapPair> & subMaps,Tpetra::CrsMatrix<ST,LO,GO,NT> & mat)
#else
void rebuildSubBlock(int i,int j,const RCP<const Tpetra::CrsMatrix<ST,NT> >& A,const std::vector<MapPair> & subMaps,Tpetra::CrsMatrix<ST,NT> & mat)
#endif
{
   // get the number of variables families
   int numVarFamily = subMaps.size();

   TEUCHOS_ASSERT(i>=0 && i<numVarFamily);
   TEUCHOS_ASSERT(j>=0 && j<numVarFamily);

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   const Tpetra::Map<LO,GO,NT> & gRowMap = *subMaps[i].first; // new GIDs
   const Tpetra::Map<LO,GO,NT> & rowMap = *subMaps[i].second; // contiguous GIDs
   const Tpetra::Map<LO,GO,NT> & colMap = *subMaps[j].second;
#else
   const Tpetra::Map<NT> & gRowMap = *subMaps[i].first; // new GIDs
   const Tpetra::Map<NT> & rowMap = *subMaps[i].second; // contiguous GIDs
   const Tpetra::Map<NT> & colMap = *subMaps[j].second;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
   const RCP<Tpetra::Vector<GO,LO,GO,NT> > plocal2ContigGIDs = getSubBlockColumnGIDs(*A,subMaps[j]);
   Tpetra::Vector<GO,LO,GO,NT> & local2ContigGIDs = *plocal2ContigGIDs;
#else
   const RCP<Tpetra::Vector<GO,NT> > plocal2ContigGIDs = getSubBlockColumnGIDs(*A,subMaps[j]);
   Tpetra::Vector<GO,NT> & local2ContigGIDs = *plocal2ContigGIDs;
#endif

   mat.resumeFill();
   mat.setAllToScalar(0.0);

   // get entry information
   LO numMyRows = rowMap.getNodeNumElements();
   LO maxNumEntries = A->getNodeMaxNumRowEntries();

   // for extraction
   std::vector<LO> indices(maxNumEntries);
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
      LO lid = A->getRowMap()->getLocalElement(globalRow);
      GO contigRow = rowMap.getGlobalElement(localRow);
      TEUCHOS_ASSERT(lid>-1);

      A->getLocalRowCopy(lid, Teuchos::ArrayView<LO>(indices), Teuchos::ArrayView<ST>(values), numEntries);

      LO numOwnedCols = 0;
      for(size_t localCol=0;localCol<numEntries;localCol++) {
         TEUCHOS_ASSERT(indices[localCol]>-1);
         
         // if global id is not owned by this column
         GO gid = local2ContigGIDs.getData()[indices[localCol]];
         if(gid==-1) continue; // in contiguous row

         colIndices[numOwnedCols] = gid;
         colValues[numOwnedCols] = values[localCol];
         numOwnedCols++;
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

} // end Blocking
} // end Epetra
} // end Teko
