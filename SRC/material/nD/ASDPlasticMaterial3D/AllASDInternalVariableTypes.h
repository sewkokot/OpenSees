/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
                                                                        
// Original implementation: José Abell (UANDES), Massimo Petracca (ASDEA)
//
// ASDPlasticMaterial3D
//
// Fully general templated material class for plasticity modeling

#include "ASDPlasticMaterial3DGlobals.h"
#include <string>
#include "InternalVariableType.h"

//Definitions of possible internal variables
struct BackStressName { static constexpr const char* name = "BackStress";};
template <class HardeningType>
using BackStress = InternalVariableType<VoigtVector, HardeningType, BackStressName>;

struct VonMisesRadiusName { static constexpr const char* name = "VonMisesRadius";};
template <class HardeningType>
using VonMisesRadius = InternalVariableType<VoigtScalar, HardeningType, VonMisesRadiusName>;

struct ScalarInternalVariableName { static constexpr const char* name = "ScalarInternalVariable";};
template <class HardeningType>
using ScalarInternalVariable = InternalVariableType<VoigtScalar, HardeningType, ScalarInternalVariableName>;
