// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
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
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_SCALEDNULLSPACEFACTORY_DECL_HPP
#define MUELU_SCALEDNULLSPACEFACTORY_DECL_HPP

#include <Xpetra_Matrix_fwd.hpp>
#include <Xpetra_VectorFactory_fwd.hpp>
#include <Xpetra_MultiVectorFactory_fwd.hpp>

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_SingleLevelFactoryBase.hpp"
#include "MueLu_ScaledNullspaceFactory_fwd.hpp"

#include "MueLu_Level_fwd.hpp"

namespace MueLu {

  /*!
     @class ScaledNullspaceFactory class.
     @brief Factory for generating a very special nullspace

      The ScaledNullspaceFactory is meant to generate a default approximation for
      the fine level nullspace (Level 0 only). For all other levels it is used
      to generate a (block-diagonally) scaled version of the Nullspace designed for non-symmetric problems.
      only to act as "generating factory" for the "Nullspace", which is actually
      handled by the TentativePFactory.

      Constructor:
      \code{.cpp}
      ScaledNullspaceFactory(const std::string & nspName = "Nullspace")
      \endcode
      This constructor uses the variable with the variable nspName on the finest level
      as null space for the finest multigrid level.

      Currently, the only accepted names for the null space vectors are "Nullspace" (default) or,
      in case of multiphysics problems, "Nullspace1" and "Nullspace2"

     @ingroup MueLuTransferClasses

    ## Input/output of NullspaceFactory ##

    ### User parameters of NullspaceFactory ###
    Parameter | type | default | master.xml | validated | requested | description
    ----------|------|---------|:----------:|:---------:|:---------:|------------
     Fine level nullspace | string | "Nullspace" | | * | * | Name of the variable containing the MultiVector with the near null space vectors on the finest level (level 0) provided by the user.
     A                  | Factory | null |   | * | * | Generating factory of the matrix A. Note that "A" is only requested, if the user does not provide a fine level null space information and ScaledNullspaceFactory is meant to provide a default set of fine level null space vectors. Then, the information of "A" is used to create constant null space vectors compatible with the row map of A.
     Nullspace          | Factory | null |   | * | * | Generating factory of the fine nullspace vectors (of type "MultiVector"). In the default case the same instance of the TentativePFactory is also the generating factory for the null space vectors (on the next coarser levels). Therefore, it is recommended to declare the TentativePFactory to be the generating factory of the "Nullspace" variable globally using the FactoryManager object! For defining the near null space vectors on the finest level one should use the ScaledNullspaceFactory.

    The * in the @c master.xml column denotes that the parameter is defined in the @c master.xml file.<br>
    The * in the @c validated column means that the parameter is declared in the list of valid input parameters (see ScaledNullspaceFactory::GetValidParameters).<br>
    The * in the @c requested column states that the data is requested as input with all dependencies (see ScaledNullspaceFactory::DeclareInput).

    ### Variables provided by ScaledNullspaceFactory ###

    After ScaledNullspaceFactory::Build the following data is available (if requested)

    Parameter | generated by | description
    ----------|--------------|------------
    | Nullspace | ScaledNullspaceFactory | MultiVector containing the near null space vectors. On the finest level it can be a user-provided set of null space vectors. Otherwise a default set of constant near nullspace vectors is provided. On the coarser levels the null space vectors will be a scaled version of the nullspace generated by the TentativePFactory.
  */

template <class Scalar = DefaultScalar,
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
          class LocalOrdinal = DefaultLocalOrdinal,
          class GlobalOrdinal = DefaultGlobalOrdinal,
#endif
          class Node = DefaultNode>
  class ScaledNullspaceFactory : public SingleLevelFactoryBase {
#undef MUELU_SCALEDNULLSPACEFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

#ifndef TPETRA_ENABLE_TEMPLATE_ORDINALS
    using LocalOrdinal = typename Tpetra::Map<>::local_ordinal_type;
    using GlobalOrdinal = typename Tpetra::Map<>::global_ordinal_type;
#endif
    //! @name Constructors/Destructors.
    //@{

    //! Constructor
    ScaledNullspaceFactory(const std::string & nspName = "Nullspace") {
      SetParameter("Fine level nullspace", ParameterEntry(nspName));
    }

    //! Destructor
    virtual ~ScaledNullspaceFactory() { }

    //@}

    //! @name Input

    //@{

    /*! @brief Define valid parameters for internal factory parameters */
    RCP<const ParameterList> GetValidParameterList() const;

    /*! @brief Specifies the data that this class needs, and the factories that generate that data.

        If the Build method of this class requires some data, but the generating factory is not specified in DeclareInput, then this class
        will fall back to the settings in FactoryManager.
    */

    void DeclareInput(Level &currentLevel) const;

    //@}

    //! @name Build methods.
    //@{

    //! Build an object with this factory.
    void Build(Level &currentLevel) const;

    //@}

  private:

  }; //class NullspaceFactory

} //namespace MueLu

#define MUELU_SCALEDNULLSPACEFACTORY_SHORT
#endif // MUELU_SCALEDNULLSPACEFACTORY_DECL_HPP
