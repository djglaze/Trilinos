/*
 * MueLu_TopRAPFactory_decl.hpp
 *
 *  Created on: Jan 25, 2016
 *      Author: tawiesn
 */

#ifndef PACKAGES_MUELU_SRC_MUECENTRAL_MUELU_TOPRAPFACTORY_DECL_HPP_
#define PACKAGES_MUELU_SRC_MUECENTRAL_MUELU_TOPRAPFACTORY_DECL_HPP_

#include "MueLu_ConfigDefs.hpp"

//#include "MueLu_FactoryManager_fwd.hpp"
#include "MueLu_FactoryManagerBase.hpp"
#include "MueLu_Level_fwd.hpp"
#include "MueLu_TwoLevelFactoryBase.hpp"
//#include "MueLu_Hierarchy_fwd.hpp"
//#include "MueLu_HierarchyManager_fwd.hpp"

namespace MueLu {

  template<class Scalar = DefaultScalar,
#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
           class LocalOrdinal = DefaultLocalOrdinal,
           class GlobalOrdinal = DefaultGlobalOrdinal,
#endif
           class Node = DefaultNode>
  class TopRAPFactory : public TwoLevelFactoryBase {
#undef MUELU_TOPRAPFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

#ifndef TPETRA_ENABLE_TEMPLATE_ORDINALS
    using LocalOrdinal = typename Tpetra::Map<>::local_ordinal_type;
    using GlobalOrdinal = typename Tpetra::Map<>::global_ordinal_type;
#endif
    TopRAPFactory(RCP<const FactoryManagerBase> parentFactoryManager);
    TopRAPFactory(RCP<const FactoryManagerBase> parentFactoryManagerFine, RCP<const FactoryManagerBase> parentFactoryManagerCoarse);

    virtual ~TopRAPFactory();

    void DeclareInput(Level & fineLevel, Level & coarseLevel) const;

    void Build(Level & fineLevel, Level & coarseLevel) const;

  private:
    RCP<const FactoryBase> PFact_;
    RCP<const FactoryBase> RFact_;
    RCP<const FactoryBase> AcFact_;
  };

}

#define MUELU_TOPRAPFACTORY_SHORT
#endif /* PACKAGES_MUELU_SRC_MUECENTRAL_MUELU_TOPRAPFACTORY_DECL_HPP_ */
