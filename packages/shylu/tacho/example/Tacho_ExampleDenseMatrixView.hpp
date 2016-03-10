#ifndef __TACHO_EXAMPLE_DENSE_MATRIX_VIEW_HPP__
#define __TACHO_EXAMPLE_DENSE_MATRIX_VIEW_HPP__

#include <Kokkos_Core.hpp>
#include <impl/Kokkos_Timer.hpp>

#include "ShyLUTacho_config.h"

#include "Tacho_Util.hpp"
#include "Tacho_DenseMatrixBase.hpp"
#include "Tacho_DenseMatrixView.hpp"
#include "Tacho_DenseMatrixTools.hpp"

namespace Tacho {

  template<typename ValueType,
           typename OrdinalType,
           typename SizeType,
           typename DeviceSpaceType>
  KOKKOS_INLINE_FUNCTION
  int exampleDenseMatrixView(const OrdinalType mmin,
                             const OrdinalType mmax,
                             const OrdinalType minc,
                             const bool verbose) {
    typedef typename
      Kokkos::Impl::is_space<DeviceSpaceType>::host_mirror_space::execution_space HostSpaceType ;
    
    typedef DenseMatrixBase<ValueType,OrdinalType,SizeType,HostSpaceType>   DenseMatrixBaseHostType;
    typedef DenseMatrixBase<ValueType,OrdinalType,SizeType,DeviceSpaceType> DenseMatrixBaseDeviceType;

    typedef DenseMatrixView<DenseMatrixBaseDeviceType> DenseMatrixViewDeviceType;

    int r_val = 0;

    Kokkos::Impl::Timer timer;

    std::cout << "DenseMatrixBase:: test matrices "
              <<":: mmin = " << mmin << " , mmax = " << mmax << " , minc = " << minc << std::endl;
    
    for (auto m=mmin;m<=mmax;m+=minc) {
      // random test matrix on host 
      DenseMatrixBaseHostType TT("TT", m, m);
      for (ordinal_type j=0;j<TT.NumCols();++j) {
        for (ordinal_type i=0;i<TT.NumRows();++i)
          TT.Value(i,j) = 2.0*((value_type)std::rand()/(RAND_MAX)) - 1.0;
        TT.Value(j,j) = std::fabs(TT.Value(j,j));
      }
      if (verbose)
        std::cout << TT << std::endl;
      
      DenseMatrixBaseDeviceType AA("AA"); 
      AA.createConfTo(TT);
      
      timer.reset();
      AA.mirror(TT);
      double t_mirror = timer.seconds();

      DenseMatrixBaseDeviceType BB("BB");
      BB.createConfTo(AA);

      DenseMatrixViewDeviceType A(AA), B(BB);     

      timer.reset();
      DenseMatrixTools::copy(B, A);
      double t_copy = timer.seconds();

      // check
      DenseMatrixBaseHostType RR("RR");
      RR.createConfTo(BB);
      RR.mirror(BB);
      if (verbose)
        std::cout << RR << std::endl;

      double err = 0.0;
      for (ordinal_type j=0;j<TT.NumCols();++j) 
        for (ordinal_type i=0;i<TT.NumRows();++i)
          err += std::fabs(TT.Value(i,j) - RR.Value(i,j));
      
      {
        const auto prec = std::cout.precision();
        std::cout.precision(4);
        
        std::cout << std::scientific
                  << "DenseMatrixBase:: dimension = " << m << " x " << m << ", "
                  << "Mirroring to device  = " << t_mirror << " [sec], " 
                  << "Elementwise copy on device = " << t_copy << " [sec], " 
                  << "Error = " << err 
                  << std::endl;
        
        std::cout.unsetf(std::ios::scientific);
        std::cout.precision(prec);
      }
    }

    return r_val;
  }
}

#endif
