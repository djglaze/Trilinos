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

#include "Teko_TpetraInverseOpWrapper.hpp"

using namespace Teuchos;

namespace Teko {
namespace TpetraHelpers {

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void TpetraInverseOpWrapper::apply(const Tpetra::MultiVector<ST,LO,GO,NT>& X, Tpetra::MultiVector<ST,LO,GO,NT>& Y,Teuchos::ETransp mode,ST alpha, ST beta) const
#else
void TpetraInverseOpWrapper::apply(const Tpetra::MultiVector<ST,NT>& X, Tpetra::MultiVector<ST,NT>& Y,Teuchos::ETransp mode,ST alpha, ST beta) const
#endif
{ TpetraOperatorWrapper::apply(X,Y,mode,alpha,beta);}

#ifdef TPETRA_ENABLE_TEMPLATE_ORDINALS
void TpetraInverseOpWrapper::applyInverse(const Tpetra::MultiVector<ST,LO,GO,NT>& X, Tpetra::MultiVector<ST,LO,GO,NT>& Y,Teuchos::ETransp mode,ST alpha, ST beta) const
#else
void TpetraInverseOpWrapper::applyInverse(const Tpetra::MultiVector<ST,NT>& X, Tpetra::MultiVector<ST,NT>& Y,Teuchos::ETransp mode,ST alpha, ST beta) const
#endif
{ TpetraOperatorWrapper::apply(X,Y,mode,alpha,beta);}

} // end namespace Epetra
} // end namespace Teko
