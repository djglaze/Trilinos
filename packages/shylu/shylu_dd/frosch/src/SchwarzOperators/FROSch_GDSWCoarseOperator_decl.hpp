//@HEADER
// ************************************************************************
//
//               ShyLU: Hybrid preconditioner package
//                 Copyright 2012 Sandia Corporation
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
// Questions? Contact Alexander Heinlein (alexander.heinlein@uni-koeln.de)
//
// ************************************************************************
//@HEADER

#ifndef _FROSCH_GDSWCOARSEOPERATOR_DECL_HPP
#define _FROSCH_GDSWCOARSEOPERATOR_DECL_HPP

#include <FROSch_HarmonicCoarseOperator_def.hpp>


namespace FROSch {

    using namespace std;
    using namespace Teuchos;
    using namespace Xpetra;

    template <class SC = double,
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
              class LO = int,
              class GO = DefaultGlobalOrdinal,
#endif
              class NO = KokkosClassic::DefaultNode::DefaultNodeType>
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    class GDSWCoarseOperator : public HarmonicCoarseOperator<SC,LO,GO,NO> {
#else
    class GDSWCoarseOperator : public HarmonicCoarseOperator<SC,NO> {
#endif

    protected:

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using CommPtr                       = typename SchwarzOperator<SC,LO,GO,NO>::CommPtr;
#else
        using LO = typename Tpetra::Map<>::local_ordinal_type;
        using GO = typename Tpetra::Map<>::global_ordinal_type;
        using CommPtr                       = typename SchwarzOperator<SC,NO>::CommPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using XMapPtr                       = typename SchwarzOperator<SC,LO,GO,NO>::XMapPtr;
        using ConstXMapPtr                  = typename SchwarzOperator<SC,LO,GO,NO>::ConstXMapPtr;
        using XMapPtrVecPtr                 = typename SchwarzOperator<SC,LO,GO,NO>::XMapPtrVecPtr;
        using ConstXMapPtrVecPtr            = typename SchwarzOperator<SC,LO,GO,NO>::ConstXMapPtrVecPtr;
        using XMapPtrVecPtr2D               = typename SchwarzOperator<SC,LO,GO,NO>::XMapPtrVecPtr2D;
        using ConstXMapPtrVecPtr2D          = typename SchwarzOperator<SC,LO,GO,NO>::ConstXMapPtrVecPtr2D;
#else
        using XMapPtr                       = typename SchwarzOperator<SC,NO>::XMapPtr;
        using ConstXMapPtr                  = typename SchwarzOperator<SC,NO>::ConstXMapPtr;
        using XMapPtrVecPtr                 = typename SchwarzOperator<SC,NO>::XMapPtrVecPtr;
        using ConstXMapPtrVecPtr            = typename SchwarzOperator<SC,NO>::ConstXMapPtrVecPtr;
        using XMapPtrVecPtr2D               = typename SchwarzOperator<SC,NO>::XMapPtrVecPtr2D;
        using ConstXMapPtrVecPtr2D          = typename SchwarzOperator<SC,NO>::ConstXMapPtrVecPtr2D;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using XMatrixPtr                    = typename SchwarzOperator<SC,LO,GO,NO>::XMatrixPtr;
        using ConstXMatrixPtr               = typename SchwarzOperator<SC,LO,GO,NO>::ConstXMatrixPtr;
#else
        using XMatrixPtr                    = typename SchwarzOperator<SC,NO>::XMatrixPtr;
        using ConstXMatrixPtr               = typename SchwarzOperator<SC,NO>::ConstXMatrixPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using XMultiVectorPtr               = typename SchwarzOperator<SC,LO,GO,NO>::XMultiVectorPtr;
        using ConstXMultiVectorPtr          = typename SchwarzOperator<SC,LO,GO,NO>::ConstXMultiVectorPtr;
        using XMultiVectorPtrVecPtr         = typename SchwarzOperator<SC,LO,GO,NO>::XMultiVectorPtrVecPtr;
        using ConstXMultiVectorPtrVecPtr    = typename SchwarzOperator<SC,LO,GO,NO>::ConstXMultiVectorPtrVecPtr;
#else
        using XMultiVectorPtr               = typename SchwarzOperator<SC,NO>::XMultiVectorPtr;
        using ConstXMultiVectorPtr          = typename SchwarzOperator<SC,NO>::ConstXMultiVectorPtr;
        using XMultiVectorPtrVecPtr         = typename SchwarzOperator<SC,NO>::XMultiVectorPtrVecPtr;
        using ConstXMultiVectorPtrVecPtr    = typename SchwarzOperator<SC,NO>::ConstXMultiVectorPtrVecPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using ParameterListPtr              = typename SchwarzOperator<SC,LO,GO,NO>::ParameterListPtr;
#else
        using ParameterListPtr              = typename SchwarzOperator<SC,NO>::ParameterListPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using DDInterfacePtr                = typename SchwarzOperator<SC,LO,GO,NO>::DDInterfacePtr;
#else
        using DDInterfacePtr                = typename SchwarzOperator<SC,NO>::DDInterfacePtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using EntitySetPtr                  = typename SchwarzOperator<SC,LO,GO,NO>::EntitySetPtr;
#else
        using EntitySetPtr                  = typename SchwarzOperator<SC,NO>::EntitySetPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using SubdomainSolverPtr            = typename SchwarzOperator<SC,LO,GO,NO>::SubdomainSolverPtr;
#else
        using SubdomainSolverPtr            = typename SchwarzOperator<SC,NO>::SubdomainSolverPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using UN                            = typename SchwarzOperator<SC,LO,GO,NO>::UN;
        using UNVecPtr                      = typename SchwarzOperator<SC,LO,GO,NO>::UNVecPtr;
#else
        using UN                            = typename SchwarzOperator<SC,NO>::UN;
        using UNVecPtr                      = typename SchwarzOperator<SC,NO>::UNVecPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using LOVec                         = typename SchwarzOperator<SC,LO,GO,NO>::LOVec;
        using LOVecPtr                      = typename SchwarzOperator<SC,LO,GO,NO>::LOVecPtr;
        using LOVecPtr2D                    = typename SchwarzOperator<SC,LO,GO,NO>::LOVecPtr2D;
#else
        using LOVec                         = typename SchwarzOperator<SC,NO>::LOVec;
        using LOVecPtr                      = typename SchwarzOperator<SC,NO>::LOVecPtr;
        using LOVecPtr2D                    = typename SchwarzOperator<SC,NO>::LOVecPtr2D;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using GOVec                         = typename SchwarzOperator<SC,LO,GO,NO>::GOVec;
        using GOVecPtr                      = typename SchwarzOperator<SC,LO,GO,NO>::GOVecPtr;
        using GOVecView                     = typename SchwarzOperator<SC,LO,GO,NO>::GOVecView;
        using GOVecPtr2D                    = typename SchwarzOperator<SC,LO,GO,NO>::GOVecPtr2D;
#else
        using GOVec                         = typename SchwarzOperator<SC,NO>::GOVec;
        using GOVecPtr                      = typename SchwarzOperator<SC,NO>::GOVecPtr;
        using GOVecView                     = typename SchwarzOperator<SC,NO>::GOVecView;
        using GOVecPtr2D                    = typename SchwarzOperator<SC,NO>::GOVecPtr2D;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using SCVec                         = typename SchwarzOperator<SC,LO,GO,NO>::SCVec;
        using SCVecPtr                      = typename SchwarzOperator<SC,LO,GO,NO>::SCVecPtr;
#else
        using SCVec                         = typename SchwarzOperator<SC,NO>::SCVec;
        using SCVecPtr                      = typename SchwarzOperator<SC,NO>::SCVecPtr;
#endif

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        using BoolVecPtr                    = typename SchwarzOperator<SC,LO,GO,NO>::BoolVecPtr;
#else
        using BoolVecPtr                    = typename SchwarzOperator<SC,NO>::BoolVecPtr;
#endif

    public:

        GDSWCoarseOperator(ConstXMatrixPtr k,
                           ParameterListPtr parameterList);

        virtual int initialize()
        {
            FROSCH_ASSERT(false,"GDSWCoarseOperator cannot be built without a repeated Map");
            return 0;
        };

        int initialize(UN dimension,
                       ConstXMapPtr repeatedMap);

        int initialize(UN dimension,
                       ConstXMapPtr repeatedMap,
                       GOVecPtr dirichletBoundaryDofs);

        int initialize(UN dimension,
                       UN dofsPerNode,
                       ConstXMapPtr repeatedNodesMap,
                       ConstXMapPtrVecPtr RepeatedDofMaps);

        int initialize(UN dimension,
                       UN dofsPerNode,
                       ConstXMapPtr repeatedNodesMap,
                       ConstXMapPtrVecPtr RepeatedDofMaps,
                       GOVecPtr dirichletBoundaryDofs);

        int initialize(UN dimension,
                       UN dofsPerNode,
                       ConstXMapPtr repeatedNodesMap,
                       ConstXMapPtrVecPtr RepeatedDofMaps,
                       ConstXMultiVectorPtr nodeList);

        int initialize(UN dimension,
                       UN dofsPerNode,
                       ConstXMapPtr repeatedNodesMap,
                       ConstXMapPtrVecPtr RepeatedDofMaps,
                       GOVecPtr dirichletBoundaryDofs,
                       ConstXMultiVectorPtr nodeList);

        int initialize(UN dimension,
                       UNVecPtr dofsPerNodeVec,
                       ConstXMapPtrVecPtr repeatedNodesMapVec,
                       ConstXMapPtrVecPtr2D repeatedDofMapsVec,
                       GOVecPtr2D dirichletBoundaryDofsVec,
                       ConstXMultiVectorPtrVecPtr nodeListVec);

        void describe(FancyOStream &out,
                      const EVerbosityLevel verbLevel=Describable::verbLevel_default) const;

        string description() const;

        virtual XMapPtr BuildRepeatedMapCoarseLevel(ConstXMapPtr &nodesMap,
                                                    UN dofsPerNode,
                                                    ConstXMapPtrVecPtr dofsMaps,
                                                    UN partitionType );

    protected:

        int buildCoarseSpace(UN dimension,
                             ConstXMapPtr nodesMap);

        int buildCoarseSpace(UN dimension,
                             ConstXMapPtr nodesMap,
                             GOVecPtr dirichletBoundaryDofs); // Das kann man auch mit in den Fall davor reinnehmen ?!

        int buildCoarseSpace(UN dimension,
                             UN dofsPerNode,
                             ConstXMapPtr nodesMap,
                             ConstXMapPtrVecPtr dofsMaps);

        int buildCoarseSpace(UN dimension,
                             UN dofsPerNode,
                             ConstXMapPtr nodesMap,
                             ConstXMapPtrVecPtr dofsMaps,
                             GOVecPtr dirichletBoundaryDofs);

        int buildCoarseSpace(UN dimension,
                             UN dofsPerNode,
                             ConstXMapPtr nodesMap,
                             ConstXMapPtrVecPtr dofsMaps,
                             ConstXMultiVectorPtr nodeList);

        int buildCoarseSpace(UN dimension,
                             UN dofsPerNode,
                             ConstXMapPtr nodesMap,
                             ConstXMapPtrVecPtr dofsMaps,
                             GOVecPtr dirichletBoundaryDofs,
                             ConstXMultiVectorPtr nodeList);

        int buildCoarseSpace(UN dimension,
                             UNVecPtr dofsPerNodeVec,
                             ConstXMapPtrVecPtr repeatedNodesMapVec,
                             ConstXMapPtrVecPtr2D repeatedDofMapsVec,
                             GOVecPtr2D dirichletBoundaryDofsVec,
                             ConstXMultiVectorPtrVecPtr nodeListVec);

        virtual int resetCoarseSpaceBlock(UN blockId,
                                          UN dimension,
                                          UN dofsPerNode,
                                          ConstXMapPtr nodesMap,
                                          ConstXMapPtrVecPtr dofsMaps,
                                          GOVecPtr dirichletBoundaryDofs,
                                          ConstXMultiVectorPtr nodeList);

        DDInterfacePtr DDInterface_;
    };

}

#endif
