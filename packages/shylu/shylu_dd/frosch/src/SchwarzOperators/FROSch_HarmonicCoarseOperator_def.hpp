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

#ifndef _FROSCH_HARMONICCOARSEOPERATOR_DEF_HPP
#define _FROSCH_HARMONICCOARSEOPERATOR_DEF_HPP

#include <FROSch_HarmonicCoarseOperator_decl.hpp>


namespace FROSch {

    using namespace std;
    using namespace Teuchos;
    using namespace Xpetra;

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    HarmonicCoarseOperator<SC,LO,GO,NO>::HarmonicCoarseOperator(ConstXMatrixPtr k,
#else
    template <class SC,class NO>
    HarmonicCoarseOperator<SC,NO>::HarmonicCoarseOperator(ConstXMatrixPtr k,
#endif
                                                                ParameterListPtr parameterList) :
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    CoarseOperator<SC,LO,GO,NO> (k,parameterList),
#else
    CoarseOperator<SC,NO> (k,parameterList),
#endif
    ExtensionSolver_ (),
    InterfaceCoarseSpaces_ (0),
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    AssembledInterfaceCoarseSpace_ (new CoarseSpace<SC,LO,GO,NO>(this->MpiComm_,this->SerialComm_)),
#else
    AssembledInterfaceCoarseSpace_ (new CoarseSpace<SC,NO>(this->MpiComm_,this->SerialComm_)),
#endif
    Dimensions_ (0),
    DofsPerNode_ (0),
    GammaDofs_ (0),
    IDofs_ (0),
    DofsMaps_ (0),
    NumberOfBlocks_ (0),
    MaxNumNeigh_(0)
    {
        FROSCH_TIMER_START_LEVELID(harmonicCoarseOperatorTime,"HarmonicCoarseOperator::HarmonicCoarseOperator");
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    typename HarmonicCoarseOperator<SC,LO,GO,NO>::XMapPtr HarmonicCoarseOperator<SC,LO,GO,NO>::computeCoarseSpace(CoarseSpacePtr coarseSpace)
#else
    template <class SC,class NO>
    typename HarmonicCoarseOperator<SC,NO>::XMapPtr HarmonicCoarseOperator<SC,NO>::computeCoarseSpace(CoarseSpacePtr coarseSpace)
#endif
    {
        FROSCH_TIMER_START_LEVELID(computeCoarseSpaceTime,"HarmonicCoarseOperator::computeCoarseSpace");
        XMapPtr repeatedMap = AssembleSubdomainMap(NumberOfBlocks_,DofsMaps_,DofsPerNode_);

        // Build local saddle point problem
        ConstXMatrixPtr repeatedMatrix = ExtractLocalSubdomainMatrix(this->K_.getConst(),repeatedMap.getConst()); // AH 12/11/2018: Should this be in initalize?

        // Extract submatrices
        GOVec indicesGammaDofsAll(0);
        GOVec indicesIDofsAll(0);
        LO tmp = 0;

        for (UN i=0; i<NumberOfBlocks_; i++) {
            for (UN j=0; j<GammaDofs_[i].size(); j++) {
                indicesGammaDofsAll.push_back(tmp+GammaDofs_[i][j]);
            }
            for (UN j=0; j<IDofs_[i].size(); j++) {
                indicesIDofsAll.push_back(tmp+IDofs_[i][j]);
            }
            tmp += GammaDofs_[i].size()+IDofs_[i].size();
        }

        XMatrixPtr kII;
        XMatrixPtr kIGamma;
        XMatrixPtr kGammaI;
        XMatrixPtr kGammaGamma;

        BuildSubmatrices(repeatedMatrix,indicesIDofsAll(),kII,kIGamma,kGammaI,kGammaGamma);

        //Detect linear dependencies
        if (!this->ParameterList_->get("Skip DetectLinearDependencies",false)) {
            LOVecPtr linearDependentVectors = detectLinearDependencies(indicesGammaDofsAll(),this->K_->getRowMap(),this->K_->getRangeMap(),repeatedMap,this->ParameterList_->get("Threshold Phi",1.e-8));
            // cout << this->MpiComm_->getRank() << " " << linearDependentVectors.size() << endl;
            AssembledInterfaceCoarseSpace_->zeroOutBasisVectors(linearDependentVectors());
        }

        // Build the saddle point harmonic extensions
        XMultiVectorPtr localCoarseSpaceBasis;
        if (AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements()) {
            localCoarseSpaceBasis = computeExtensions(repeatedMatrix->getRowMap(),indicesGammaDofsAll(),indicesIDofsAll(),kII,kIGamma);

            coarseSpace->addSubspace(AssembledInterfaceCoarseSpace_->getBasisMap(),AssembledInterfaceCoarseSpace_->getBasisMapUnique(),localCoarseSpaceBasis);
        } else {
            FROSCH_NOTIFICATION("FROSch::HarmonicCoarseOperator",this->Verbose_,"The Coarse Space is empty. No extensions are computed.");
        }

        return repeatedMap;
    }


#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int HarmonicCoarseOperator<SC,LO,GO,NO>::assembleInterfaceCoarseSpace()
#else
    template <class SC,class NO>
    int HarmonicCoarseOperator<SC,NO>::assembleInterfaceCoarseSpace()
#endif
    {
        FROSCH_TIMER_START_LEVELID(assembleInterfaceCoarseSpaceTime,"HarmonicCoarseOperator::assembleInterfaceCoarseSpace");
        LO ii=0;
        for (UN i=0; i<NumberOfBlocks_; i++) {
            if (!InterfaceCoarseSpaces_[i].is_null()) {
                if (InterfaceCoarseSpaces_[i]->hasBasisMap()) {
                    FROSCH_ASSERT(InterfaceCoarseSpaces_[i]->hasBasisMapUnique(),"FROSch::HarmonicCoarseOperator : ERROR: !InterfaceCoarseSpaces_[i]->hasAssembledBasis()");
                    this->AssembledInterfaceCoarseSpace_->addSubspace(InterfaceCoarseSpaces_[i]->getBasisMap(),InterfaceCoarseSpaces_[i]->getBasisMapUnique(),InterfaceCoarseSpaces_[i]->getAssembledBasis(),ii);
                }
            }
            ii += InterfaceCoarseSpaces_[i]->getAssembledBasis()->getLocalLength();
            InterfaceCoarseSpaces_[i].reset();
        }
        return this->AssembledInterfaceCoarseSpace_->assembleCoarseSpace();
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int HarmonicCoarseOperator<SC,LO,GO,NO>::addZeroCoarseSpaceBlock(ConstXMapPtr dofsMap)
#else
    template <class SC,class NO>
    int HarmonicCoarseOperator<SC,NO>::addZeroCoarseSpaceBlock(ConstXMapPtr dofsMap)
#endif
    {
        FROSCH_TIMER_START_LEVELID(addZeroCoarseSpaceBlockTime,"HarmonicCoarseOperator::addZeroCoarseSpaceBlock");
        // Das könnte man noch ändern
        GammaDofs_->resize(GammaDofs_.size()+1);
        IDofs_->resize(IDofs_.size()+1);
        InterfaceCoarseSpaces_->resize(InterfaceCoarseSpaces_.size()+1);
        DofsMaps_->resize(DofsMaps_.size()+1);
        DofsPerNode_->resize(DofsPerNode_.size()+1);

        NumberOfBlocks_++;

        /////
        int blockId = NumberOfBlocks_-1;

        // Process the parameter list
        stringstream blockIdStringstream;
        blockIdStringstream << blockId+1;
        string blockIdString = blockIdStringstream.str();
        RCP<ParameterList> coarseSpaceList = sublist(sublist(this->ParameterList_,"Blocks"),blockIdString.c_str());

        bool useForCoarseSpace = coarseSpaceList->get("Use For Coarse Space",true);

        GammaDofs_[blockId] = LOVecPtr(0);

        XMultiVectorPtr mVPhiGamma;
        XMapPtr blockCoarseMap;
        if (useForCoarseSpace) {
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
            InterfaceCoarseSpaces_[blockId].reset(new CoarseSpace<SC,LO,GO,NO>(this->MpiComm_,this->SerialComm_));
#else
            InterfaceCoarseSpaces_[blockId].reset(new CoarseSpace<SC,NO>(this->MpiComm_,this->SerialComm_));
#endif

            //Epetra_SerialComm serialComm;
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
            XMapPtr serialGammaMap = MapFactory<LO,GO,NO>::Build(dofsMap->lib(),dofsMap->getNodeNumElements(),0,this->SerialComm_);
            mVPhiGamma = MultiVectorFactory<LO,GO,NO>::Build(serialGammaMap,dofsMap->getNodeNumElements());
#else
            XMapPtr serialGammaMap = MapFactory<NO>::Build(dofsMap->lib(),dofsMap->getNodeNumElements(),0,this->SerialComm_);
            mVPhiGamma = MultiVectorFactory<NO>::Build(serialGammaMap,dofsMap->getNodeNumElements());
#endif
        }

        for (int i=0; i<dofsMap->getNodeNumElements(); i++) {
            GammaDofs_[blockId]->push_back(i);

            if (useForCoarseSpace) {
                mVPhiGamma->replaceLocalValue(i,i,ScalarTraits<SC>::one());
            }
        }

        IDofs_[blockId] = LOVecPtr(0);

        if (useForCoarseSpace) {
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
            blockCoarseMap = MapFactory<LO,GO,NO>::Build(dofsMap->lib(),-1,GammaDofs_[blockId](),0,this->MpiComm_);
#else
            blockCoarseMap = MapFactory<NO>::Build(dofsMap->lib(),-1,GammaDofs_[blockId](),0,this->MpiComm_);
#endif

            InterfaceCoarseSpaces_[blockId]->addSubspace(blockCoarseMap,mVPhiGamma);
            InterfaceCoarseSpaces_[blockId]->assembleCoarseSpace();
        }

        DofsMaps_[blockId] = XMapPtrVecPtr(0);
        DofsMaps_[blockId].push_back(dofsMap);

        DofsPerNode_[blockId] = 1;

        return 0;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    int HarmonicCoarseOperator<SC,LO,GO,NO>::computeVolumeFunctions(UN blockId,
#else
    template <class SC,class NO>
    int HarmonicCoarseOperator<SC,NO>::computeVolumeFunctions(UN blockId,
#endif
                                                                    UN dimension,
                                                                    ConstXMapPtr nodesMap,
                                                                    ConstXMultiVectorPtr nodeList,
                                                                    EntitySetConstPtr interior)
    {
        FROSCH_TIMER_START_LEVELID(computeVolumeFunctionsTime,"HarmonicCoarseOperator::computeVolumeFunctions");
        // Process the parameter list
        stringstream blockIdStringstream;
        blockIdStringstream << blockId+1;
        string blockIdString = blockIdStringstream.str();
        RCP<ParameterList> coarseSpaceList = sublist(sublist(this->ParameterList_,"Blocks"),blockIdString.c_str());

        bool useForCoarseSpace = coarseSpaceList->get("Use For Coarse Space",true);
        bool useRotations = coarseSpaceList->get("Rotations",true);
        if (useRotations && nodeList.is_null()) {
            useRotations = false;
            FROSCH_WARNING("FROSch::HarmonicCoarseOperator",this->Verbose_,"Rotations cannot be used since nodeList.is_null().");
        }

        this->GammaDofs_[blockId] = LOVecPtr(this->DofsPerNode_[blockId]*interior->getEntity(0)->getNumNodes());
        this->IDofs_[blockId] = LOVecPtr(0);
        for (UN k=0; k<this->DofsPerNode_[blockId]; k++) {
            for (UN i=0; i<interior->getEntity(0)->getNumNodes(); i++) {
                this->GammaDofs_[blockId][this->DofsPerNode_[blockId]*i+k] = interior->getEntity(0)->getLocalDofID(i,k);
            }
        }

        if (useForCoarseSpace) {
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
            InterfaceCoarseSpaces_[blockId].reset(new CoarseSpace<SC,LO,GO,NO>(this->MpiComm_,this->SerialComm_));
#else
            InterfaceCoarseSpaces_[blockId].reset(new CoarseSpace<SC,NO>(this->MpiComm_,this->SerialComm_));
#endif

            interior->buildEntityMap(nodesMap);

            XMultiVectorPtrVecPtr translations = computeTranslations(blockId,interior);
            for (UN i=0; i<translations.size(); i++) {
                this->InterfaceCoarseSpaces_[blockId]->addSubspace(interior->getEntityMap(),null,translations[i]);
            }
            if (useRotations) {
                XMultiVectorPtrVecPtr rotations = computeRotations(blockId,dimension,nodeList,interior);
                for (UN i=0; i<rotations.size(); i++) {
                    this->InterfaceCoarseSpaces_[blockId]->addSubspace(interior->getEntityMap(),null,rotations[i]);
                }
            }

            InterfaceCoarseSpaces_[blockId]->assembleCoarseSpace();

            // Count entities
            GO numEntitiesGlobal = interior->getEntityMap()->getMaxAllGlobalIndex();
            if (interior->getEntityMap()->lib()==UseEpetra || interior->getEntityMap()->getGlobalNumElements()>0) {
                numEntitiesGlobal += 1;
            }

            if (this->Verbose_) {
                cout
                << "\n" << setw(FROSCH_INDENT) << " "
                << setw(89) << "-----------------------------------------------------------------------------------------"
                << "\n" << setw(FROSCH_INDENT) << " "
                << "| "
                << left << setw(74) << "RGDSW coarse space " << right << setw(8) << "(Level " << setw(2) << this->LevelID_ << ")"
                << " |"
                << "\n" << setw(FROSCH_INDENT) << " "
                << setw(89) << "========================================================================================="
                << "\n" << setw(FROSCH_INDENT) << " "
                << "| " << left << setw(20) << "Volumes " << " | " << setw(19) << " Translations" << right
                << " | " << setw(41) << boolalpha << useForCoarseSpace << noboolalpha
                << " |"
                << "\n" << setw(FROSCH_INDENT) << " "
                << "| " << left << setw(20) << "Volumes " << " | " << setw(19) << " Rotations" << right
                << " | " << setw(41) << boolalpha << useRotations << noboolalpha
                << " |"
                << "\n" << setw(FROSCH_INDENT) << " "
                << setw(89) << "-----------------------------------------------------------------------------------------"
                << endl;
            }
        }
        return 0;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    typename HarmonicCoarseOperator<SC,LO,GO,NO>::XMultiVectorPtrVecPtr HarmonicCoarseOperator<SC,LO,GO,NO>::computeTranslations(UN blockId,
#else
    template <class SC,class NO>
    typename HarmonicCoarseOperator<SC,NO>::XMultiVectorPtrVecPtr HarmonicCoarseOperator<SC,NO>::computeTranslations(UN blockId,
#endif
                                                                                                                                 EntitySetConstPtr entitySet)
    {
        FROSCH_TIMER_START_LEVELID(computeTranslationsTime,"HarmonicCoarseOperator::computeTranslations");
        XMultiVectorPtrVecPtr translations(this->DofsPerNode_[blockId]);
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        XMapPtr serialGammaMap = MapFactory<LO,GO,NO>::Build(this->K_->getRangeMap()->lib(),this->GammaDofs_[blockId].size(),0,this->SerialComm_);
#else
        XMapPtr serialGammaMap = MapFactory<NO>::Build(this->K_->getRangeMap()->lib(),this->GammaDofs_[blockId].size(),0,this->SerialComm_);
#endif
        for (UN i=0; i<this->DofsPerNode_[blockId]; i++) {
            if (entitySet->getNumEntities()>0) {
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
                translations[i] = MultiVectorFactory<SC,LO,GO,NO>::Build(serialGammaMap,entitySet->getNumEntities());
#else
                translations[i] = MultiVectorFactory<SC,NO>::Build(serialGammaMap,entitySet->getNumEntities());
#endif
            } else {
                translations[i] = null;
            }
        }

        for (UN k=0; k<this->DofsPerNode_[blockId]; k++) {
            for (UN i=0; i<entitySet->getNumEntities(); i++) {
                for (UN j=0; j<entitySet->getEntity(i)->getNumNodes(); j++) {
                    translations[k]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,k),i,ScalarTraits<SC>::one());
                }
            }
        }
        return translations;
    }


#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    typename HarmonicCoarseOperator<SC,LO,GO,NO>::XMapPtr HarmonicCoarseOperator<SC,LO,GO,NO>::assembleCoarseMap()
#else
    template <class SC,class NO>
    typename HarmonicCoarseOperator<SC,NO>::XMapPtr HarmonicCoarseOperator<SC,NO>::assembleCoarseMap()
#endif
    {
        FROSCH_TIMER_START_LEVELID(assembleCoarseMapTime,"HarmonicCoarseOperator::assembleCoarseMap");
        GOVec mapVector(0);
        GO tmp = 0;
        for (UN i=0; i<NumberOfBlocks_; i++) {
            if (!InterfaceCoarseSpaces_[i].is_null()) {
                if (InterfaceCoarseSpaces_[i]->hasBasisMap()) {
                    for (UN j=0; j<InterfaceCoarseSpaces_[i]->getBasisMap()->getNodeNumElements(); j++) {
                        mapVector.push_back(InterfaceCoarseSpaces_[i]->getBasisMap()->getGlobalElement(j)+tmp);
                    }
                    if (InterfaceCoarseSpaces_[i]->getBasisMap()->getMaxAllGlobalIndex()>=0) {
                        tmp += InterfaceCoarseSpaces_[i]->getBasisMap()->getMaxAllGlobalIndex()+1;
                    }
                }
            }
        }
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        return MapFactory<LO,GO,NO>::Build(DofsMaps_[0][0]->lib(),-1,mapVector(),0,this->MpiComm_);
#else
        return MapFactory<NO>::Build(DofsMaps_[0][0]->lib(),-1,mapVector(),0,this->MpiComm_);
#endif
    }


#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    typename HarmonicCoarseOperator<SC,LO,GO,NO>::XMultiVectorPtrVecPtr HarmonicCoarseOperator<SC,LO,GO,NO>::computeRotations(UN blockId,
#else
    template <class SC,class NO>
    typename HarmonicCoarseOperator<SC,NO>::XMultiVectorPtrVecPtr HarmonicCoarseOperator<SC,NO>::computeRotations(UN blockId,
#endif
                                                                                                                              UN dimension,
                                                                                                                              ConstXMultiVectorPtr nodeList,
                                                                                                                              EntitySetConstPtr entitySet,
                                                                                                                              UN discardRotations)
    {
        FROSCH_TIMER_START_LEVELID(computeRotationsTime,"HarmonicCoarseOperator::computeRotations");
        FROSCH_ASSERT(nodeList->getNumVectors()==dimension,"FROSch::HarmonicCoarseOperator : ERROR: Dimension of the nodeList is wrong.");
        FROSCH_ASSERT(dimension==this->DofsPerNode_[blockId],"FROSch::HarmonicCoarseOperator : ERROR: Dimension!=this->DofsPerNode_[blockId]");

        UN rotationsPerEntity = 0;
        switch (dimension) {
            case 1:
                return null;
                break;
            case 2:
                rotationsPerEntity = 1;
                break;
            case 3:
                rotationsPerEntity = 3;
                break;
            default:
                FROSCH_ASSERT(false,"FROSch::HarmonicCoarseOperator : ERROR: The dimension is neither 2 nor 3!");
                break;
        }

        XMultiVectorPtrVecPtr rotations(rotationsPerEntity);
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        XMapPtr serialGammaMap = MapFactory<LO,GO,NO>::Build(this->K_->getRangeMap()->lib(),this->GammaDofs_[blockId].size(),0,this->SerialComm_);
#else
        XMapPtr serialGammaMap = MapFactory<NO>::Build(this->K_->getRangeMap()->lib(),this->GammaDofs_[blockId].size(),0,this->SerialComm_);
#endif
        for (UN i=0; i<rotationsPerEntity; i++) {
            if (entitySet->getNumEntities()>0) {
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
                rotations[i] = MultiVectorFactory<SC,LO,GO,NO>::Build(serialGammaMap,entitySet->getNumEntities());
#else
                rotations[i] = MultiVectorFactory<SC,NO>::Build(serialGammaMap,entitySet->getNumEntities());
#endif
            } else {
                rotations[i] = null;
            }
        }

        SC x,y,z,rx,ry,rz;
        SCVec rx0(3,0.0),ry0(3,0.0),rz0(3,0.0),errx(3,0.0),erry(3,0.0),errz(3,0.0);
        for (UN i=0; i<entitySet->getNumEntities(); i++) {
            // Compute values for the first node to check if rotation is constant
            x = nodeList->getData(0)[entitySet->getEntity(i)->getLocalNodeID(0)];
            y = nodeList->getData(1)[entitySet->getEntity(i)->getLocalNodeID(0)];

            rx0[0] = y;
            ry0[0] = -x;
            if (dimension == 3) {
                z = nodeList->getData(2)[entitySet->getEntity(i)->getLocalNodeID(0)];

                rz0[0] = 0;

                rx0[1] = -z;
                ry0[1] = 0;
                rz0[1] = x;

                rx0[2] = 0;
                ry0[2] = z;
                rz0[2] = -y;
            }
            for (UN j=0; j<entitySet->getEntity(i)->getNumNodes(); j++) {
                x = nodeList->getData(0)[entitySet->getEntity(i)->getLocalNodeID(j)];
                y = nodeList->getData(1)[entitySet->getEntity(i)->getLocalNodeID(j)];

                ////////////////
                // Rotation 1 //
                ////////////////
                rx = y;
                ry = -x;
                rotations[0]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,0),i,rx);
                rotations[0]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,1),i,ry);

                // Compute difference (Euclidean norm of error to constant function)
                errx[0] += (rx0[0]-rx)*(rx0[0]-rx);
                erry[0] += (ry0[0]-ry)*(ry0[0]-ry);

                if (dimension == 3) {
                    z = nodeList->getData(2)[entitySet->getEntity(i)->getLocalNodeID(j)];

                    rz = 0;
                    rotations[0]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,2),i,rz);

                    // Compute difference (Euclidean norm of error to constant function)
                    errz[0] += (rz0[0]-rz)*(rz0[0]-rz);

                    ////////////////
                    // Rotation 2 //
                    ////////////////
                    rx = -z;
                    ry = 0;
                    rz = x;
                    rotations[1]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,0),i,rx);
                    rotations[1]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,1),i,ry);
                    rotations[1]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,2),i,rz);

                    // Compute difference (Euclidean norm of error to constant function)
                    errx[1] += (rx0[1]-rx)*(rx0[1]-rx);
                    erry[1] += (ry0[1]-ry)*(ry0[1]-ry);
                    errz[1] += (rz0[1]-rz)*(rz0[1]-rz);

                    ////////////////
                    // Rotation 3 //
                    ////////////////
                    rx = 0;
                    ry = z;
                    rz = -y;
                    rotations[2]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,0),i,rx);
                    rotations[2]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,1),i,ry);
                    rotations[2]->replaceLocalValue(entitySet->getEntity(i)->getGammaDofID(j,2),i,rz);

                    // Compute difference (Euclidean norm of error to constant function)
                    errx[2] += (rx0[2]-rx)*(rx0[2]-rx);
                    erry[2] += (ry0[2]-ry)*(ry0[2]-ry);
                    errz[2] += (rz0[2]-rz)*(rz0[2]-rz);
                }
            }

            // If error to constant function is almost zero => scale rotation with zero
            SC err;
            UN numZeroRotations = 0;
            switch (dimension) {
                case 2:
                    err = errx[0]+erry[0];
                    err = ScalarTraits<SC>::squareroot(err);
                    if (fabs(err)<1.0e-12) {
                        FROSCH_ASSERT(false,"FROSch::HarmonicCoarseOperator : ERROR: In 2D, no rotation can be constant!");
                        rotations[0]->getVectorNonConst(i)->scale(ScalarTraits<SC>::zero());
                        numZeroRotations++;
                    }
                    break;
                case 3:
                    for (UN j=0; j<3; j++) {
                        err = errx[j]+erry[j]+errz[j];
                        err = ScalarTraits<SC>::squareroot(err);
                        if (fabs(err)<1.0e-12) {
                            rotations[j]->getVectorNonConst(i)->scale(ScalarTraits<SC>::zero());
                            numZeroRotations++;
                        }
                    }
                    break;
                default:
                    FROSCH_ASSERT(false,"FROSch::HarmonicCoarseOperator : ERROR: The dimension is neither 1 nor 2 nor 3!");
                    break;
            }
            // If necessary, discard additional rotations
            UN rotationsToDiscard = discardRotations - numZeroRotations;
            if (rotationsToDiscard<0) {
                FROSCH_WARNING("FROSch::HarmonicCoarseOperator",this->Verbose_,"More rotations have been discarded than expected.");
            } else if (rotationsToDiscard>0) {
                UN it=0;
                UN rotationsDiscarded=0;
                while (rotationsDiscarded<rotationsToDiscard) {
                    if (rotations[it]->getVector(i)->norm2()>1.0e-12) {
                        rotations[it]->getVectorNonConst(i)->scale(ScalarTraits<SC>::zero());
                        rotationsDiscarded++;
                    }
                    it++;
                }
            }
        }
        return rotations;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    typename HarmonicCoarseOperator<SC,LO,GO,NO>::LOVecPtr HarmonicCoarseOperator<SC,LO,GO,NO>::detectLinearDependencies(GOVecView indicesGammaDofsAll,
#else
    template <class SC,class NO>
    typename HarmonicCoarseOperator<SC,NO>::LOVecPtr HarmonicCoarseOperator<SC,NO>::detectLinearDependencies(GOVecView indicesGammaDofsAll,
#endif
                                                                                                                         ConstXMapPtr rowMap,
                                                                                                                         ConstXMapPtr rangeMap,
                                                                                                                         ConstXMapPtr repeatedMap,
                                                                                                                         SC treshold)
    {
        FROSCH_TIMER_START_LEVELID(detectLinearDependenciesTime,"HarmonicCoarseOperator::detectLinearDependencies");
        LOVecPtr linearDependentVectors(AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements()); //if (this->Verbose_) cout << AssembledInterfaceCoarseSpace_->getAssembledBasis()->getNumVectors() << " " << AssembledInterfaceCoarseSpace_->getAssembledBasis()->getLocalLength() << " " << indicesGammaDofsAll.size() << endl;
        if (AssembledInterfaceCoarseSpace_->getAssembledBasis()->getNumVectors()>0 && AssembledInterfaceCoarseSpace_->getAssembledBasis()->getLocalLength()>0) {
            //Construct matrix phiGamma
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
            XMatrixPtr phiGamma = MatrixFactory<SC,LO,GO,NO>::Build(rowMap,AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
#else
            XMatrixPtr phiGamma = MatrixFactory<SC,NO>::Build(rowMap,AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
#endif

            LO iD;
            SC valueTmp;
            for (UN i=0; i<AssembledInterfaceCoarseSpace_->getAssembledBasis()->getLocalLength(); i++) {
                GOVec indices;
                SCVec values;
                for (UN j=0; j<AssembledInterfaceCoarseSpace_->getAssembledBasis()->getNumVectors(); j++) {
                    valueTmp=AssembledInterfaceCoarseSpace_->getAssembledBasis()->getData(j)[i];
                    if (fabs(valueTmp)>treshold) {
                        indices.push_back(AssembledInterfaceCoarseSpace_->getBasisMap()->getGlobalElement(j));
                        values.push_back(valueTmp);
                    }
                }
                iD = repeatedMap->getGlobalElement(indicesGammaDofsAll[i]);

                if (rowMap->getLocalElement(iD)!=-1) { // This should prevent duplicate entries on the interface
                    phiGamma->insertGlobalValues(iD,indices(),values());
                }
            }
            phiGamma->fillComplete(AssembledInterfaceCoarseSpace_->getBasisMapUnique(),rangeMap);

            //Compute Phi^T * Phi
            RCP<FancyOStream> fancy = fancyOStream(rcpFromRef(cout));
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
            XMatrixPtr phiTPhi = MatrixMatrix<SC,LO,GO,NO>::Multiply(*phiGamma,true,*phiGamma,false,*fancy); //phiGamma->describe(*fancy,VERB_EXTREME); phiTPhi->describe(*fancy,VERB_EXTREME); //AssembledInterfaceCoarseSpace_->getBasisMap()->describe(*fancy,VERB_EXTREME);
#else
            XMatrixPtr phiTPhi = MatrixMatrix<SC,NO>::Multiply(*phiGamma,true,*phiGamma,false,*fancy); //phiGamma->describe(*fancy,VERB_EXTREME); phiTPhi->describe(*fancy,VERB_EXTREME); //AssembledInterfaceCoarseSpace_->getBasisMap()->describe(*fancy,VERB_EXTREME);
#endif

            // Extract local part of the matrix
            ConstXMatrixPtr repeatedPhiTPhi = ExtractLocalSubdomainMatrix(phiTPhi.getConst(),AssembledInterfaceCoarseSpace_->getBasisMap());
            //Communicate matrix to the repeated map
            // repeatedPhiTPhi = MatrixFactory<SC,LO,GO,NO>::Build(AssembledInterfaceCoarseSpace_->getBasisMap());
            // XExportPtr exporter = ExportFactory<LO,GO,NO>::Build(rowMap,repeatedMap);
            // repeatedPhiTPhi->doExport(*phiTPhi,*exporter,INSERT);

            UN numRows = repeatedPhiTPhi->getRowMap()->getNodeNumElements();
            TSerialDenseMatrixPtr denseRepeatedPhiTPhi(new SerialDenseMatrix<LO,SC>(numRows,repeatedPhiTPhi->getColMap()->getNodeNumElements()));
            for (UN i=0; i<numRows; i++) {
                ConstLOVecView indices;
                ConstSCVecView values;
                repeatedPhiTPhi->getLocalRowView(i,indices,values);
                for (UN j=0; j<indices.size(); j++) {
                    (*denseRepeatedPhiTPhi)(i,indices[j]) = values[j];
                }
            }
            // if (this->MpiComm_->getRank()==3) {
            //     for (LO i=0; i<denseRepeatedPhiTPhi->numRows(); i++) {
            //         for (LO j=0; j<denseRepeatedPhiTPhi->numCols(); j++) {
            //             cout << (*denseRepeatedPhiTPhi)(i,j) << " ";
            //         }
            //         cout << endl;
            //     }
            // }

            //Compute local QR factorization
            TSerialQRDenseSolverPtr qRSolver(new SerialQRDenseSolver<LO,SC>());
            qRSolver->setMatrix(denseRepeatedPhiTPhi);
            qRSolver->factor();
            qRSolver->formQ();
            qRSolver->formR();

            //Find rows of R approx. zero
            TSerialDenseMatrixPtr r = qRSolver->getR();
            LO tmp = 0;
            for (LO i=0; i<r->numRows(); i++) {
                SC normRow = 0.0;
                for (LO j=0; j<r->numCols(); j++) {
                    normRow += (*r)(i,j)*(*r)(i,j);
                }
                if (sqrt(normRow)<treshold) {
                    //cout << this->MpiComm_->getRank() << " " << i << " " << AssembledInterfaceCoarseSpace_->getBasisMap()->getGlobalElement(i) << " " << sqrt(normRow) << std::endl;
                    linearDependentVectors[tmp] = i;
                    tmp++;
                }
            }
            linearDependentVectors.resize(tmp);
        }

        FROSCH_TIMER_START_LEVELID(printStatisticsTime,"print statistics");
        // Statistics on linear dependencies
        GO global = AssembledInterfaceCoarseSpace_->getBasisMap()->getMaxAllGlobalIndex();
        if (AssembledInterfaceCoarseSpace_->getBasisMap()->lib()==UseEpetra || AssembledInterfaceCoarseSpace_->getBasisMap()->getGlobalNumElements()>0) {
            global += 1;
        }
        LOVec localVec(3);
        LOVec sumVec(3);
        SCVec avgVec(3);
        LOVec minVec(3);
        LOVec maxVec(3);

        LO numLocalBasisFunctions = AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements();
        LO numLocalLinearDependencies = linearDependentVectors.size();
        LO numLocalLinearIndependencies = numLocalBasisFunctions-numLocalLinearDependencies;

        sumVec[0] = AssembledInterfaceCoarseSpace_->getBasisMap()->getGlobalNumElements();
        avgVec[0] = max(sumVec[0]/double(this->MpiComm_->getSize()),0.0);
        reduceAll(*this->MpiComm_,REDUCE_MIN,numLocalBasisFunctions,ptr(&minVec[0]));
        reduceAll(*this->MpiComm_,REDUCE_MAX,numLocalBasisFunctions,ptr(&maxVec[0]));

        reduceAll(*this->MpiComm_,REDUCE_SUM,numLocalLinearDependencies,ptr(&sumVec[1]));
        avgVec[1] = max(sumVec[1]/double(this->MpiComm_->getSize()),0.0);
        reduceAll(*this->MpiComm_,REDUCE_MIN,numLocalLinearDependencies,ptr(&minVec[1]));
        reduceAll(*this->MpiComm_,REDUCE_MAX,numLocalLinearDependencies,ptr(&maxVec[1]));

        reduceAll(*this->MpiComm_,REDUCE_SUM,numLocalLinearIndependencies,ptr(&sumVec[2]));
        avgVec[2] = max(sumVec[2]/double(this->MpiComm_->getSize()),0.0);
        reduceAll(*this->MpiComm_,REDUCE_MIN,numLocalLinearIndependencies,ptr(&minVec[2]));
        reduceAll(*this->MpiComm_,REDUCE_MAX,numLocalLinearIndependencies,ptr(&maxVec[2]));

        if (this->Verbose_) {
            cout
            << "\n" << setw(FROSCH_INDENT) << " "
            << setw(89) << "-----------------------------------------------------------------------------------------"
            << "\n" << setw(FROSCH_INDENT) << " "
            << "| "
            << left << setw(74) << "Local linear dependencies of coarse basis functions statistics " << right << setw(8) << "(Level " << setw(2) << this->LevelID_ << ")"
            << " |"
            << "\n" << setw(FROSCH_INDENT) << " "
            << setw(89) << "========================================================================================="
            << "\n" << setw(FROSCH_INDENT) << " "
            << "| " << left << setw(20) << " " << right
            << " | " << setw(10) << "total"
            << " | " << setw(10) << "avg"
            << " | " << setw(10) << "min"
            << " | " << setw(10) << "max"
            << " | " << setw(10) << "global sum"
            << " |"
            << "\n" << setw(FROSCH_INDENT) << " "
            << setw(89) << "-----------------------------------------------------------------------------------------"
            << "\n" << setw(FROSCH_INDENT) << " "
            << "| " << left << setw(20) << "Basis functions" << right
            << " | " << setw(10) << global
            << " | " << setw(10) << setprecision(5) << avgVec[0]
            << " | " << setw(10) << minVec[0]
            << " | " << setw(10) << maxVec[0]
            << " | " << setw(10) << sumVec[0]
            << " |"
            << "\n" << setw(FROSCH_INDENT) << " "
            << "| " << left << setw(20) << "Dependent" << right
            << " | " << setw(10) << " "
            << " | " << setw(10) << setprecision(5) << avgVec[1]
            << " | " << setw(10) << minVec[1]
            << " | " << setw(10) << maxVec[1]
            << " | " << setw(10) << sumVec[1]
            << " |"
            << "\n" << setw(FROSCH_INDENT) << " "
            << "| " << left << setw(20) << "Independent" << right
            << " | " << setw(10) << " "
            << " | " << setw(10) << setprecision(5) << avgVec[2]
            << " | " << setw(10) << minVec[2]
            << " | " << setw(10) << maxVec[2]
            << " | " << setw(10) << sumVec[2]
            << " |"
            << "\n" << setw(FROSCH_INDENT) << " "
            << setw(89) << "-----------------------------------------------------------------------------------------"
            << endl;
        }
        FROSCH_TIMER_STOP(printStatisticsTime);

        return linearDependentVectors;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO,class NO>
    typename HarmonicCoarseOperator<SC,LO,GO,NO>::XMultiVectorPtr HarmonicCoarseOperator<SC,LO,GO,NO>::computeExtensions(ConstXMapPtr localMap,
#else
    template <class SC,class NO>
    typename HarmonicCoarseOperator<SC,NO>::XMultiVectorPtr HarmonicCoarseOperator<SC,NO>::computeExtensions(ConstXMapPtr localMap,
#endif
                                                                                                                         GOVecView indicesGammaDofsAll,
                                                                                                                         GOVecView indicesIDofsAll,
                                                                                                                         XMatrixPtr kII,
                                                                                                                         XMatrixPtr kIGamma)
    {
        FROSCH_TIMER_START_LEVELID(computeExtensionsTime,"HarmonicCoarseOperator::computeExtensions");
        //this->Phi_ = MatrixFactory<SC,LO,GO,NO>::Build(this->K_->getRangeMap(),AssembledInterfaceCoarseSpace_->getBasisMap(),AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements()); // Nonzeroes abhängig von dim/dofs!!!
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        XMultiVectorPtr mVPhi = MultiVectorFactory<SC,LO,GO,NO>::Build(localMap,AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
        XMultiVectorPtr mVtmp = MultiVectorFactory<SC,LO,GO,NO>::Build(kII->getRowMap(),AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
        XMultiVectorPtr mVPhiI = MultiVectorFactory<SC,LO,GO,NO>::Build(kII->getRowMap(),AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
#else
        XMultiVectorPtr mVPhi = MultiVectorFactory<SC,NO>::Build(localMap,AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
        XMultiVectorPtr mVtmp = MultiVectorFactory<SC,NO>::Build(kII->getRowMap(),AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
        XMultiVectorPtr mVPhiI = MultiVectorFactory<SC,NO>::Build(kII->getRowMap(),AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
#endif

        //Build mVPhiGamma
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        XMultiVectorPtr mVPhiGamma = MultiVectorFactory<SC,LO,GO,NO>::Build(kIGamma->getDomainMap(),AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
#else
        XMultiVectorPtr mVPhiGamma = MultiVectorFactory<SC,NO>::Build(kIGamma->getDomainMap(),AssembledInterfaceCoarseSpace_->getBasisMap()->getNodeNumElements());
#endif
        if (AssembledInterfaceCoarseSpace_->hasAssembledBasis()) {
            for (UN i=0; i<AssembledInterfaceCoarseSpace_->getAssembledBasis()->getNumVectors(); i++) {
                ConstSCVecPtr assembledInterfaceCoarseSpaceData = AssembledInterfaceCoarseSpace_->getAssembledBasis()->getData(i);
                for (UN j=0; j<AssembledInterfaceCoarseSpace_->getAssembledBasis()->getLocalLength(); j++) {
                    mVPhiGamma->replaceLocalValue(j,i,assembledInterfaceCoarseSpaceData[j]);
                    mVPhi->replaceLocalValue(indicesGammaDofsAll[j],i,assembledInterfaceCoarseSpaceData[j]);
                }
            }
        }
//        LO jj=0;
//        LO kk=0;
//        UNVec numLocalBlockRows(NumberOfBlocks_);
//        for (UN i=0; i<NumberOfBlocks_; i++) {
//            UN j = 0;
//            UN k = 0;
//            if (InterfaceCoarseSpaces_[i]->hasAssembledBasis()) {
//                numLocalBlockRows[i] = InterfaceCoarseSpaces_[i]->getAssembledBasis()->getNumVectors();
//                for (j=0; j<numLocalBlockRows[i]; j++) {
//                    for (k=0; k<InterfaceCoarseSpaces_[i]->getAssembledBasis()->getLocalLength(); k++) {
//                        mVPhiGamma->replaceLocalValue(k+kk,j+jj,InterfaceCoarseSpaces_[i]->getAssembledBasis()->getData(j)[k]);
//                        mVPhi->replaceLocalValue(indicesGammaDofsAll[k+kk],j+jj,InterfaceCoarseSpaces_[i]->getAssembledBasis()->getData(j)[k]);
//                    }
//                }
//            } else { // Das ist für den Fall, dass keine Basisfunktionen für einen Block gebaut werden sollen
//                k=GammaDofs_[i].size();
//            }
//            jj += j;
//            kk += k;
//        }
        // RCP<FancyOStream> fancy = fancyOStream(rcpFromRef(cout)); this->Phi_->describe(*fancy,VERB_EXTREME);
        // Hier Multiplikation kIGamma*PhiGamma
        kIGamma->apply(*mVPhiGamma,*mVtmp);

        mVtmp->scale(-ScalarTraits<SC>::one());

        // Jetzt der solver für kII
        if (indicesIDofsAll.size()>0) {
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
            ExtensionSolver_.reset(new SubdomainSolver<SC,LO,GO,NO>(kII,sublist(this->ParameterList_,"ExtensionSolver")));
#else
            ExtensionSolver_.reset(new SubdomainSolver<SC,NO>(kII,sublist(this->ParameterList_,"ExtensionSolver")));
#endif
            ExtensionSolver_->initialize();
            ExtensionSolver_->compute();
            ExtensionSolver_->apply(*mVtmp,*mVPhiI);
        }

        GOVec priorIndex(NumberOfBlocks_,0);
        GOVec postIndex(NumberOfBlocks_,0);

        GOVec2D excludeCols(NumberOfBlocks_);

        LOVec bound(NumberOfBlocks_+1,(LO)0);
        for (UN i=0; i<NumberOfBlocks_; i++) {
            bound[i+1] = bound[i] + this->IDofs_[i].size();
        }

        LO itmp = 0;
        ConstUNVecView numLocalBlockRows = AssembledInterfaceCoarseSpace_->getLocalSubspaceSizes();
        FROSCH_ASSERT(numLocalBlockRows.size()==NumberOfBlocks_,"FROSch::HarmonicCoarseOperator : ERROR: numLocalBlockRows.size()!=NumberOfBlocks_");
        for (UN i=0; i<NumberOfBlocks_; i++) {
            stringstream blockIdStringstream;
            blockIdStringstream << i+1;
            string blockIdString = blockIdStringstream.str();
            RCP<ParameterList> coarseSpaceList = sublist(sublist(this->ParameterList_,"Blocks"),blockIdString.c_str());
            string excludeBlocksString = coarseSpaceList->get("Exclude","0");
            UNVec excludeBlocks = FROSch::GetIndicesFromString(excludeBlocksString,(UN)0);
            sortunique(excludeBlocks);
            for (UN j=0; j<excludeBlocks.size(); j++) {
                excludeBlocks[j] -= 1;
            }
            UNVec extensionBlocks(0);
            for (UN j=0; j<NumberOfBlocks_; j++) {
                typename UNVec::iterator it = find(excludeBlocks.begin(),excludeBlocks.end(),j);
                if (it == excludeBlocks.end()) {
                    extensionBlocks.push_back(j);
                }
            }

            for (UN j=0; j<numLocalBlockRows[i]; j++) {
                ConstSCVecPtr mVPhiIData = mVPhiI->getData(itmp);
                for (UN ii=0; ii<extensionBlocks.size(); ii++) {
                    for (LO k=bound[extensionBlocks[ii]]; k<bound[extensionBlocks[ii]+1]; k++) {
                        mVPhi->replaceLocalValue(indicesIDofsAll[k],itmp,mVPhiIData[k]);
                    }
                }
                itmp++;
            }
        }
        return mVPhi;
    }
    // REPMAP-------------------------------------
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template<class SC,class LO, class GO, class NO>
    int HarmonicCoarseOperator<SC,LO,GO,NO>::buildGlobalGraph(Teuchos::RCP<DDInterface<SC,LO,GO,NO> > theDDInterface)
#else
    template<class SC, class NO>
    int HarmonicCoarseOperator<SC,NO>::buildGlobalGraph(Teuchos::RCP<DDInterface<SC,NO> > theDDInterface)
#endif
    {
      FROSCH_TIMER_START_LEVELID(buildGlobalGraphTime,"HarmonicCoarseOperator::buildGlobalGraph");
      std::map<GO,int> rep;
      Teuchos::Array<GO> entries;
      IntVec2D conn;
      InterfaceEntityPtrVec connVec;
      int connrank;
      //get connected subdomains
      theDDInterface->identifyConnectivityEntities();
      EntitySetConstPtr connect=  theDDInterface->getConnectivityEntities();
      connect->buildEntityMap(theDDInterface->getNodesMap());
      connVec = connect->getEntityVector();
      connrank = connect->getEntityMap()->getComm()->getRank();

       GO connVecSize = connVec.size();
       conn.resize(connVecSize);
       {
         if (connVecSize>0) {
           for(GO i = 0;i<connVecSize;i++) {
               conn[i] = connVec[i]->getSubdomainsVector();
               for (int j = 0; j<conn[i].size(); j++) rep.insert(std::pair<GO,int>(conn.at(i).at(j),connrank));
           }
           for (auto& x: rep) {
               entries.push_back(x.first);
           }
         }
       }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
       Teuchos::RCP<Xpetra::Map<LO,GO,NO> > graphMap = Xpetra::MapFactory<LO,GO,NO>::Build(this->K_->getMap()->lib(),-1,1,0,this->K_->getMap()->getComm());
#else
       Teuchos::RCP<Xpetra::Map<NO> > graphMap = Xpetra::MapFactory<NO>::Build(this->K_->getMap()->lib(),-1,1,0,this->K_->getMap()->getComm());
#endif

       //UN maxNumElements = -1;
       //get the maximum number of neighbors for a subdomain
       MaxNumNeigh_ = -1;
       UN numElementsLocal = entries.size();
       reduceAll(*this->MpiComm_,Teuchos::REDUCE_MAX,numElementsLocal,Teuchos::ptr(&MaxNumNeigh_));
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
       this->SubdomainConnectGraph_ = Xpetra::CrsGraphFactory<LO,GO,NO>::Build(graphMap,MaxNumNeigh_);
#else
       this->SubdomainConnectGraph_ = Xpetra::CrsGraphFactory<NO>::Build(graphMap,MaxNumNeigh_);
#endif
       this->SubdomainConnectGraph_->insertGlobalIndices(graphMap->getComm()->getRank(),entries());

       return 0;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO,class GO, class NO>
    int HarmonicCoarseOperator<SC,LO,GO,NO>::buildElementNodeList()
#else
    template <class SC, class NO>
    int HarmonicCoarseOperator<SC,NO>::buildElementNodeList()
#endif
    {
      //get elements belonging to one subdomain
      FROSCH_TIMER_START_LEVELID(buildElementNodeListTime,"CoarseOperator::buildElementNodeList");

      Teuchos::ArrayView<const GO> elements =  KRowMap_->getNodeElementList();
      UN maxNumElements = -1;
      UN numElementsLocal = elements.size();

      reduceAll(*this->MpiComm_,Teuchos::REDUCE_MAX,numElementsLocal,Teuchos::ptr(&maxNumElements));


#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      GraphPtr elemGraph = Xpetra::CrsGraphFactory<LO,GO,NO>::Build(this->MLGatheringMaps_[0],maxNumElements);
#else
      GraphPtr elemGraph = Xpetra::CrsGraphFactory<NO>::Build(this->MLGatheringMaps_[0],maxNumElements);
#endif
      Teuchos::ArrayView<const GO> myGlobals = this->SubdomainConnectGraph_->getRowMap()->getNodeElementList();

      for (size_t i = 0; i < this->SubdomainConnectGraph_->getRowMap()->getNodeNumElements(); i++) {
        elemGraph->insertGlobalIndices(myGlobals[i],elements);
      }
      elemGraph->fillComplete();

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      GraphPtr tmpElemGraph = Xpetra::CrsGraphFactory<LO,GO,NO>::Build(this->MLGatheringMaps_[1],maxNumElements);
#else
      GraphPtr tmpElemGraph = Xpetra::CrsGraphFactory<NO>::Build(this->MLGatheringMaps_[1],maxNumElements);
#endif
      GraphPtr elemSGraph;
      //communicate ElemGrapg to CoarseComm_
      tmpElemGraph->doExport(*elemGraph,*this->MLCoarseSolveExporters_[1],Xpetra::INSERT);
      UN gathered = 0;
      for(int i  = 2;i<this->MLGatheringMaps_.size();i++){
        gathered = 1;
        tmpElemGraph->fillComplete();
        elemSGraph = tmpElemGraph;
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        tmpElemGraph = Xpetra::CrsGraphFactory<LO,GO,NO>::Build(this->MLGatheringMaps_[i],maxNumElements);
#else
        tmpElemGraph = Xpetra::CrsGraphFactory<NO>::Build(this->MLGatheringMaps_[i],maxNumElements);
#endif
        tmpElemGraph->doExport(*elemSGraph,*this->MLCoarseSolveExporters_[i],Xpetra::INSERT);
      }
      //tmpElemGraph->fillComplete();
      elemSGraph = tmpElemGraph;

      if(gathered == 0){
        elemSGraph = tmpElemGraph;
      }
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      this->ElementNodeList_ =Xpetra::CrsGraphFactory<LO,GO,NO>::Build(this->MLCoarseMap_,maxNumElements);
#else
      this->ElementNodeList_ =Xpetra::CrsGraphFactory<NO>::Build(this->MLCoarseMap_,maxNumElements);
#endif

      if(this->OnCoarseSolveComm_){
        const size_t numMyElementS = this->MLCoarseMap_->getNodeNumElements();
        Teuchos::ArrayView<const GO> va;
        for (UN i = 0; i < numMyElementS; i++) {
          GO kg = this->MLGatheringMaps_[this->MLGatheringMaps_.size()-1]->getGlobalElement(i);
          tmpElemGraph->getGlobalRowView(kg,va);
          Teuchos::Array<GO> vva(va);
          this->ElementNodeList_->insertGlobalIndices(kg,vva());//mal va nehmen
        }
        this->ElementNodeList_->fillComplete();

      }
      return 0;
    }

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
    template <class SC,class LO, class GO,class NO>
    int HarmonicCoarseOperator<SC,LO,GO,NO>::buildCoarseGraph()
#else
    template <class SC,class NO>
    int HarmonicCoarseOperator<SC,NO>::buildCoarseGraph()
#endif
    {
      //bring graph to CoarseSolveComm_
      FROSCH_TIMER_START_LEVELID(buildCoarseGraphTime,"CoarseOperator::buildCoarseGraph");

      //create temporary graphs to perform communication (possibly with gathering steps)
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
      GraphPtr tmpGraph =  Xpetra::CrsGraphFactory<LO,GO,NO>::Build(this->MLGatheringMaps_[1],MaxNumNeigh_);;
#else
      GraphPtr tmpGraph =  Xpetra::CrsGraphFactory<NO>::Build(this->MLGatheringMaps_[1],MaxNumNeigh_);;
#endif
      GraphPtr tmpGraphGathering;

      tmpGraph->doExport(*this->SubdomainConnectGraph_,*this->MLCoarseSolveExporters_[1],Xpetra::INSERT);

      for(int i  = 2;i<this->MLGatheringMaps_.size();i++){
        tmpGraph->fillComplete();
        tmpGraphGathering = tmpGraph;
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
        tmpGraph = Xpetra::CrsGraphFactory<LO,GO,NO>::Build(this->MLGatheringMaps_[i],MaxNumNeigh_);
#else
        tmpGraph = Xpetra::CrsGraphFactory<NO>::Build(this->MLGatheringMaps_[i],MaxNumNeigh_);
#endif
        tmpGraph->doExport(*tmpGraphGathering,*this->MLCoarseSolveExporters_[i],Xpetra::INSERT);
     }
     const size_t numMyElementS = this->MLGatheringMaps_[this->MLGatheringMaps_.size()-1]->getNodeNumElements();
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
     this->SubdomainConnectGraph_= Xpetra::CrsGraphFactory<LO,GO,NO>::Build(this->MLCoarseMap_,MaxNumNeigh_);
#else
     this->SubdomainConnectGraph_= Xpetra::CrsGraphFactory<NO>::Build(this->MLCoarseMap_,MaxNumNeigh_);
#endif

     if (this->OnCoarseSolveComm_) {
       for (size_t k = 0; k<numMyElementS; k++) {
         Teuchos::ArrayView<const LO> in;
         Teuchos::ArrayView<const GO> vals_graph;
         GO kg = this->MLGatheringMaps_[this->MLGatheringMaps_.size()-1]->getGlobalElement(k);
         tmpGraph->getGlobalRowView(kg,vals_graph);
         Teuchos::Array<GO> vals(vals_graph);
         this->SubdomainConnectGraph_->insertGlobalIndices(kg,vals());
       }
       this->SubdomainConnectGraph_->fillComplete();
     }
     return 0;
    }
    //----------------------------------------------

}

#endif
