/*
 * MueLu_RepartitionInterface_decl.hpp
 *
 *  Created on: 5 Sep 2013
 *      Author: wiesner
 */

#ifndef MUELU_REPARTITIONINTERFACE_DECL_HPP_
#define MUELU_REPARTITIONINTERFACE_DECL_HPP_

#include <Xpetra_Map.hpp>
#include <Xpetra_Matrix.hpp>
#include <Xpetra_MapFactory_fwd.hpp>
#include <Xpetra_MultiVectorFactory.hpp>
#include <Xpetra_VectorFactory.hpp>

#include "MueLu_SingleLevelFactoryBase.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_Graph_fwd.hpp"
#include "MueLu_AmalgamationFactory_fwd.hpp"
#include "MueLu_AmalgamationInfo_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"


namespace MueLu {

  /*!
    @class RepartitionInterface
    @brief Helper class which transforms an "AmalgamatedPartition" array to an unamalgamated "Partition".
    @ingroup Rebalancing

    This is a general class that allows to translate node-based rebalancing information (given by "AmalgamatedPartition") to
    DOF-based rebalancing information (stored as output in the "Partition" variable).
    It is meant to be used together with the IsorropiaInterface class which provides the node-based rebalancing information
    in the "AmalgamatedPartition" variable. It uses the striding information of "A" to transform the amalgamated rebalaning info
    into DOF-based rebalancing information that can be processed by the RepartitionFactory class.

    @note: We assume a constant number of DOFs per node

    ## Input/output of RepartitionInterface ##

    ### User parameters of RepartitionInterface ###
    Parameter | type | default | master.xml | validated | requested | description
    ----------|------|---------|:----------:|:---------:|:---------:|------------
    | A                                      | Factory | null  |   | * | * | Generating factory of the matrix A used during the prolongator smoothing process |
    | AmalgamatedPartition | Factory | null |  | * | * | Factory generating the AmalgamatedPartition (e.g. an IsorropiaInterface)
    | number of partitions                   | GO      | - |  |  |  | Short-cut parameter set by RepartitionFactory. Avoid repartitioning algorithms if only one partition is necessary (see details below)

    The * in the @c master.xml column denotes that the parameter is defined in the @c master.xml file.<br>
    The * in the @c validated column means that the parameter is declared in the list of valid input parameters (see RepartitionInterface::GetValidParameters).<br>
    The * in the @c requested column states that the data is requested as input with all dependencies (see RepartitionInterface::DeclareInput).

    ### Variables provided by RepartitionInterface ###

    After RepartitionInterface::Build the following data is available (if requested)

    Parameter | generated by | description
    ----------|--------------|------------
    | Partition | RepartitionInterface   | GOVector based on the Row map of A (DOF-based) containing the process id the DOF should be living in after rebalancing/repartitioning

    The "Partition" vector is used as input for the RepartitionFactory class.
    If Re-partitioning/rebalancing is necessary it uses the "Partition" variable to create the corresponding Xpetra::Import object which then is used
    by the RebalanceFactory classes (e.g., RebalanceAcFactory, RebalanceTransferFactory,...) to rebalance the coarse level operators.

    The RepartitionHeuristicFactory calculates how many partitions are to be built when performing rebalancing.
    It stores the result in the "number of partitions" variable on the current level (type = GO).
    If it is "number of partitions=1" we skip the underlying Zoltan call and just create an dummy "Partition" vector containing zeros only.
    If no repartitioning is necessary (i.e., just keep the current partitioning) we return "Partition = Teuchos::null".
    If "number of partitions" > 1, the algorithm tries to find the requested number of partitions.
  */

  //FIXME: this class should not be templated
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
  template <class LocalOrdinal = DefaultLocalOrdinal,
            class GlobalOrdinal = DefaultGlobalOrdinal,
            class Node = DefaultNode>
#else
  template <class Node = DefaultNode>
#endif
  class RepartitionInterface : public SingleLevelFactoryBase {

#ifndef TPETRA_ENABLE_TEMPLATE_ORDINALS
    using LocalOrdinal = typename Tpetra::Map<>::local_ordinal_type;
    using GlobalOrdinal = typename Tpetra::Map<>::global_ordinal_type;
#endif
    typedef double Scalar; // FIXME
#undef MUELU_REPARTITIONINTERFACE_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors
    //@{

    //! Constructor
    RepartitionInterface() { }

    //! Destructor
    virtual ~RepartitionInterface() { }
    //@}

    RCP<const ParameterList> GetValidParameterList() const;

    //! @name Input
    //@{
    void DeclareInput(Level & level) const;
    //@}

    //! @name Build methods.
    //@{
    void Build(Level &level) const;

    //@}



  private:



  };  //class RepartitionInterface

} //namespace MueLu

#define MUELU_REPARTITIONINTERFACE_SHORT
#endif /* MUELU_REPARTITIONINTERFACE_DECL_HPP_ */
