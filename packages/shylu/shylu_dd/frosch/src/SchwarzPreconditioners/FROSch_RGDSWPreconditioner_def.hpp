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

#ifndef _FROSCH_RGDSWPRECONDITIONER_DEF_HPP
#define _FROSCH_RGDSWPRECONDITIONER_DEF_HPP

#include <FROSch_RGDSWPreconditioner_decl.hpp>


namespace FROSch {

    using namespace std;
    using namespace Teuchos;
    using namespace Xpetra;

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    RGDSWPreconditioner<SC,LO,GO,NO>::RGDSWPreconditioner(ConstXMatrixPtr k,
#else
    template <class SC,class NO>
    RGDSWPreconditioner<SC,NO>::RGDSWPreconditioner(ConstXMatrixPtr k,
#endif
                                                          ParameterListPtr parameterList) :
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    AlgebraicOverlappingPreconditioner<SC,LO,GO,NO> (k,parameterList)
#else
    AlgebraicOverlappingPreconditioner<SC,NO> (k,parameterList)
#endif
    {
        FROSCH_TIMER_START_LEVELID(rGDSWPreconditionerTime,"RGDSWPreconditioner::RGDSWPreconditioner");
        // Set the LevelID in the sublist
        parameterList->sublist("RGDSWCoarseOperator").set("Level ID",this->LevelID_);
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        CoarseOperator_.reset(new RGDSWCoarseOperator<SC,LO,GO,NO>(k,sublist(parameterList,"RGDSWCoarseOperator")));
#else
        CoarseOperator_.reset(new RGDSWCoarseOperator<SC,NO>(k,sublist(parameterList,"RGDSWCoarseOperator")));
#endif
        this->SumOperator_->addOperator(CoarseOperator_);
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(bool useDefaultParameters)
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(bool useDefaultParameters)
#endif
    {
        ConstXMapPtr repeatedMap = BuildRepeatedMap(this->K_->getCrsGraph());
        return initialize(repeatedMap,useDefaultParameters);
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(ConstXMapPtr repeatedMap,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(ConstXMapPtr repeatedMap,
#endif
                                                     bool useDefaultParameters)
    {
        if (useDefaultParameters) {
            return initialize(3,1,repeatedMap);
        } else {
            DofOrdering dofOrdering = NodeWise;
            if (!this->ParameterList_->get("DofOrdering","NodeWise").compare("NodeWise")) {
                dofOrdering = NodeWise;
            } else if (!this->ParameterList_->get("DofOrdering","NodeWise").compare("DimensionWise")) {
                dofOrdering = DimensionWise;
            } else if (!this->ParameterList_->get("DofOrdering","NodeWise").compare("Custom")) {
                dofOrdering = Custom;
            } else {
                FROSCH_ASSERT(false,"ERROR: Specify a valid DofOrdering.");
            }

            return initialize(this->ParameterList_->get("Dimension",1),this->ParameterList_->get("DofsPerNode",1),dofOrdering,this->ParameterList_->get("Overlap",1),repeatedMap);
        }
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(GOVecPtr &dirichletBoundaryDofs,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(GOVecPtr &dirichletBoundaryDofs,
#endif
                                                     bool useDefaultParameters)
    {
        ConstXMapPtr repeatedMap = BuildRepeatedMap(this->K_->getCrsGraph());
        return initialize(repeatedMap,dirichletBoundaryDofs,useDefaultParameters);
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(ConstXMapPtr repeatedMap,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(ConstXMapPtr repeatedMap,
#endif
                                                     GOVecPtr &dirichletBoundaryDofs,
                                                     bool useDefaultParameters)
    {
        if (useDefaultParameters) {
            return initialize(3,1,repeatedMap,dirichletBoundaryDofs);
        } else {
            DofOrdering dofOrdering = NodeWise;
            if (!this->ParameterList_->get("DofOrdering","NodeWise").compare("NodeWise")) {
                dofOrdering = NodeWise;
            } else if (!this->ParameterList_->get("DofOrdering","NodeWise").compare("DimensionWise")) {
                dofOrdering = DimensionWise;
            } else if (!this->ParameterList_->get("DofOrdering","NodeWise").compare("Custom")) {
                dofOrdering = Custom;
            } else {
                FROSCH_ASSERT(false,"ERROR: Specify a valid DofOrdering.");
            }

            return initialize(this->ParameterList_->get("Dimension",1),this->ParameterList_->get("DofsPerNode",1),dofOrdering,this->ParameterList_->get("Overlap",1),repeatedMap,dirichletBoundaryDofs);
        }
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(UN dimension,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(UN dimension,
#endif
                                                     int overlap)
    {
        XMapPtr repeatedMap = BuildRepeatedMap(this->K_->getCrsGraph());
        return initialize(dimension,overlap,repeatedMap);
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(UN dimension,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(UN dimension,
#endif
                                                     int overlap,
                                                     ConstXMapPtr repeatedMap)
    {
        FROSCH_TIMER_START_LEVELID(initializeTime,"RGDSWPreconditioner::initialize");
        int ret = 0;
        if (0>this->OverlappingOperator_->initialize(overlap,repeatedMap)) ret -= 1;
        if (0>CoarseOperator_->initialize(dimension,repeatedMap)) ret -= 10;

        return 0;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(UN dimension,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(UN dimension,
#endif
                                                     int overlap,
                                                     ConstXMapPtr repeatedMap,
                                                     GOVecPtr &dirichletBoundaryDofs)
    {
        FROSCH_TIMER_START_LEVELID(initializeTime,"RGDSWPreconditioner::initialize");
        int ret = 0;
        if (0>this->OverlappingOperator_->initialize(overlap,repeatedMap)) ret -= 1;
        if (0>CoarseOperator_->initialize(dimension,repeatedMap,dirichletBoundaryDofs)) ret -= 10;

        return 0;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(UN dimension,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(UN dimension,
#endif
                                                     UN dofsPerNode,
                                                     DofOrdering dofOrdering,
                                                     int overlap,
                                                     ConstXMapPtr repeatedMap)
    {
        FROSCH_TIMER_START_LEVELID(initializeTime,"RGDSWPreconditioner::initialize");
        FROSCH_ASSERT(dofOrdering == NodeWise || dofOrdering == DimensionWise,"ERROR: Specify a valid DofOrdering.");
        int ret = 0;
        if (0>this->OverlappingOperator_->initialize(overlap,repeatedMap)) ret -= 1;
        ConstXMapPtr repeatedNodesMap;
        ConstXMapPtrVecPtr repeatedDofMaps;
        if (0>BuildDofMaps(repeatedMap,dofsPerNode,dofOrdering,repeatedNodesMap,repeatedDofMaps)) ret -= 100;
        if (0>CoarseOperator_->initialize(dimension,dofsPerNode,repeatedNodesMap,repeatedDofMaps)) ret -=10;

        return ret;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(UN dimension,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(UN dimension,
#endif
                                                     UN dofsPerNode,
                                                     DofOrdering dofOrdering,
                                                     int overlap,
                                                     ConstXMapPtr repeatedMap,
                                                     GOVecPtr &dirichletBoundaryDofs)
    {
        FROSCH_TIMER_START_LEVELID(initializeTime,"RGDSWPreconditioner::initialize");
        FROSCH_ASSERT(dofOrdering == NodeWise || dofOrdering == DimensionWise,"ERROR: Specify a valid DofOrdering.");
        int ret = 0;
        if (0>this->OverlappingOperator_->initialize(overlap,repeatedMap)) ret -= 1;
        ConstXMapPtr repeatedNodesMap;
        ConstXMapPtrVecPtr repeatedDofMaps;
        if (0>BuildDofMaps(repeatedMap,dofsPerNode,dofOrdering,repeatedNodesMap,repeatedDofMaps)) ret -= 100;
        if (0>CoarseOperator_->initialize(dimension,dofsPerNode,repeatedNodesMap,repeatedDofMaps,dirichletBoundaryDofs)) ret -=10;

        return ret;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(UN dimension,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(UN dimension,
#endif
                                                     UN dofsPerNode,
                                                     DofOrdering dofOrdering,
                                                     int overlap,
                                                     ConstXMapPtr repeatedMap,
                                                     ConstXMultiVectorPtr &nodeList)
    {
        FROSCH_TIMER_START_LEVELID(initializeTime,"RGDSWPreconditioner::initialize");
        FROSCH_ASSERT(dofOrdering == NodeWise || dofOrdering == DimensionWise,"ERROR: Specify a valid DofOrdering.");
        int ret = 0;
        if (0>this->OverlappingOperator_->initialize(overlap,repeatedMap)) ret -= 1;
        ConstXMapPtr repeatedNodesMap;
        ConstXMapPtrVecPtr repeatedDofMaps;
        if (0>BuildDofMaps(repeatedMap,dofsPerNode,dofOrdering,repeatedNodesMap,repeatedDofMaps)) ret -= 100;
        if (0>CoarseOperator_->initialize(dimension,dofsPerNode,repeatedNodesMap,repeatedDofMaps,nodeList)) ret -=10;

        return ret;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::initialize(UN dimension,
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::initialize(UN dimension,
#endif
                                                     UN dofsPerNode,
                                                     DofOrdering dofOrdering,
                                                     int overlap,
                                                     ConstXMapPtr repeatedMap,
                                                     GOVecPtr &dirichletBoundaryDofs,
                                                     ConstXMultiVectorPtr &nodeList)
    {
        FROSCH_TIMER_START_LEVELID(initializeTime,"RGDSWPreconditioner::initialize");
        FROSCH_ASSERT(dofOrdering == NodeWise || dofOrdering == DimensionWise,"ERROR: Specify a valid DofOrdering.");
        int ret = 0;
        if (0>this->OverlappingOperator_->initialize(overlap,repeatedMap)) ret -= 1;

        ConstXMapPtr repeatedNodesMap;
        ConstXMapPtrVecPtr repeatedDofMaps;
        if (0>BuildDofMaps(repeatedMap,dofsPerNode,dofOrdering,repeatedNodesMap,repeatedDofMaps)) ret -= 100;
        if (0>CoarseOperator_->initialize(dimension,dofsPerNode,repeatedNodesMap,repeatedDofMaps,dirichletBoundaryDofs,nodeList)) ret -=10;

        return ret;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::compute()
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::compute()
#endif
    {
        FROSCH_TIMER_START_LEVELID(computeTime,"RGDSWPreconditioner::compute");
        int ret = 0;
        if (0>this->OverlappingOperator_->compute()) ret -= 1;
        if (0>CoarseOperator_->compute()) ret -= 10;
        return ret;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    void RGDSWPreconditioner<SC,LO,GO,NO>::describe(FancyOStream &out,
#else
    template <class SC,class NO>
    void RGDSWPreconditioner<SC,NO>::describe(FancyOStream &out,
#endif
                                                    const EVerbosityLevel verbLevel) const
    {
        FROSCH_ASSERT(false,"describe() has to be implemented properly...");
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    string RGDSWPreconditioner<SC,LO,GO,NO>::description() const
#else
    template <class SC,class NO>
    string RGDSWPreconditioner<SC,NO>::description() const
#endif
    {
        return "RGDSW Preconditioner";
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int RGDSWPreconditioner<SC,LO,GO,NO>::resetMatrix(ConstXMatrixPtr &k)
#else
    template <class SC,class NO>
    int RGDSWPreconditioner<SC,NO>::resetMatrix(ConstXMatrixPtr &k)
#endif
    {
        FROSCH_TIMER_START_LEVELID(resetMatrixTime,"RGDSWPreconditioner::resetMatrix");
        this->K_ = k;
        this->OverlappingOperator_->resetMatrix(this->K_);
        CoarseOperator_->resetMatrix(this->K_);
        return 0;
    }
}

#endif
