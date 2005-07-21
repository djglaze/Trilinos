//
// @HEADER
// ***********************************************************************
// 
//                           Capo Package
//                 Copyright (2005) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef Capo_TCUBED_INTERFACE_H
#define Capo_TCUBED_INTERFACE_H

/************************************************************ 
File:      Tcubed_interface.hpp
Purpose:   Create a class to wrap "Epetra-Integrators"
Date:      7-15-05
Author:    Joseph Simonis
**************************************************************/

/**** Include Files ****/
#include "Capo_Integrator.hpp"
#include "Thyra_VectorBase.hpp"
#include "Tcubed_FiniteElementProblem.H"


/**** ThyraIntegrator Class ****/
class ThyraIntegrator : public CAPO::Integrator
{
  typedef double Scalar;
public:

  //!Constructor:
  ThyraIntegrator(Teuchos::RefCountPtr<Tcubed_FiniteElementProblem>& problem,
		  Teuchos::RefCountPtr<const Epetra_Map>& emap);
  
  //! Destructor
  ~ThyraIntegrator() {};
  
  // User must compile in an ThyraIntegrator::Integrate function
  bool Integrate(const Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> >& y, 
		 const Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> >& x, 
		 const double T,
		 const double lambda);
private:
  Teuchos::RefCountPtr<Tcubed_FiniteElementProblem> AppIntegrator;
  Teuchos::RefCountPtr<const Epetra_Map> Emap;

};

#endif
