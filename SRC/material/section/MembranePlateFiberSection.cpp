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
                                                                        
// $Revision: 1.8 $
// $Date: 2007-05-03 23:03:01 $
// $Source: /usr/local/cvs/OpenSees/SRC/material/section/MembranePlateFiberSection.cpp,v $

// Ed "C++" Love
//
//  Generic Plate Section with membrane
//


#include <MembranePlateFiberSection.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>
#include <MaterialResponse.h>
#include <string.h>
#include <elementAPI.h>

void* OPS_MembranePlateFiberSection()
{
    int numdata = OPS_GetNumRemainingInputArgs();
    if (numdata < 3) {
	opserr << "WARNING insufficient arguments\n";
	opserr << "Want: section PlateFiber tag? matTag? h? <integrationType?>" << endln;
	return 0;
    }
    
    int idata[2];
    numdata = 2;
    if (OPS_GetIntInput(&numdata, idata) < 0) {
	opserr << "WARNING: invalid tags\n";
	return 0;
    }

    double h;
    numdata = 1;
    if (OPS_GetDoubleInput(&numdata, &h) < 0) {
	opserr << "WARNING: invalid h\n";
	return 0;
    }

    int integrationType = 0;
    if (OPS_GetNumRemainingInputArgs() > 0) {
      const char *type = OPS_GetString();
      if (strcmp(type,"Lobatto") == 0)
	integrationType = 0;
      if (strcmp(type,"Gauss") == 0 || strcmp(type,"Legendre") == 0)
	integrationType = 1;
    }
    
    NDMaterial *theMaterial = OPS_getNDMaterial(idata[1]);
    if (theMaterial == 0) {
	opserr << "WARNING nD material does not exist\n";
	opserr << "nD material: " << idata[1]; 
	opserr << "\nPlateFiber section: " << idata[0] << endln;
	return 0;
    }

    return new MembranePlateFiberSection(idata[0], h, *theMaterial, integrationType);
}

//parameters
const double MembranePlateFiberSection::root56 = sqrt(5.0/6.0) ; //shear correction

//static vector and matrices
Vector  MembranePlateFiberSection::stressResultant(8) ;
Matrix  MembranePlateFiberSection::tangent(8,8) ;
ID      MembranePlateFiberSection::array(8) ;


const double  MembranePlateFiberSection::sgLobatto[] = { -1, 
							 -0.65465367, 
							 0, 
							 0.65465367, 
							 1 } ;
 
const double  MembranePlateFiberSection::wgLobatto[] = { 0.1, 
							 0.5444444444, 
							 0.7111111111, 
							 0.5444444444, 
							 0.1  };

const double MembranePlateFiberSection::sgGauss[] = { -0.906179845938664,
						      -0.538469310105683,
						      0.0,
						      0.538469310105683,
						      0.906179845938664 };

const double MembranePlateFiberSection::wgGauss[] = { 0.236926885056189,
						      0.478628670499366,
						      0.568888888888889,
						      0.478628670499366,
						      0.236926885056189};


//null constructor
MembranePlateFiberSection::MembranePlateFiberSection( ) : 
SectionForceDeformation( 0, SEC_TAG_MembranePlateFiberSection ), 
h(0.), integrationType(0),
strainResultant(8)
{ 
  for ( int i = 0; i < numFibers; i++ )
      theFibers[i] = 0 ;
}



//full constructor
MembranePlateFiberSection::MembranePlateFiberSection(    
				   int tag, 
                                   double thickness, 
                                   NDMaterial &Afiber, int integrType ) :
SectionForceDeformation( tag, SEC_TAG_MembranePlateFiberSection ),
h(thickness), integrationType(integrType), strainResultant(8)
{
  for ( int i = 0; i < numFibers; i++ )
      theFibers[i] = Afiber.getCopy( "PlateFiber" ) ;

}



//destructor
MembranePlateFiberSection::~MembranePlateFiberSection( ) 
{ 
  for ( int i = 0; i < numFibers; i++ )
     delete theFibers[i] ; 
} 



//make a clone of this material
SectionForceDeformation  *MembranePlateFiberSection::getCopy( ) 
{
  MembranePlateFiberSection *clone ;   //new instance of this class

  clone = new MembranePlateFiberSection( this->getTag(), 
                                         this->h,
                                         *theFibers[0], integrationType ) ; //make the copy
  clone->strainResultant = strainResultant;
  
  return clone ;
}



//send back order of strainResultant in vector form
int MembranePlateFiberSection::getOrder( ) const
{
  return 8 ;
}


//send back order of strainResultant in vector form
const ID& MembranePlateFiberSection::getType( ) 
{
    static bool initialized = false;
    if (!initialized) {
        array(0) = SECTION_RESPONSE_FXX;
        array(1) = SECTION_RESPONSE_FYY;
        array(2) = SECTION_RESPONSE_FXY;
        array(3) = SECTION_RESPONSE_MXX;
        array(4) = SECTION_RESPONSE_MYY;
        array(5) = SECTION_RESPONSE_MXY;
        array(6) = SECTION_RESPONSE_VXZ;
        array(7) = SECTION_RESPONSE_VYZ;
        initialized = true;
    }
    return array;
}



//swap history variables
int MembranePlateFiberSection::commitState( ) 
{
  int success = 0 ;

  for ( int i = 0; i < numFibers; i++ )
    success += theFibers[i]->commitState( ) ;

  return success ;
}



//revert to last saved state
int MembranePlateFiberSection::revertToLastCommit( )
{
  int success = 0 ;

  for ( int i = 0; i < numFibers; i++ )
    success += theFibers[i]->revertToLastCommit( ) ;

  return success ;
}

//revert to start
int MembranePlateFiberSection::revertToStart( )
{
  int success = 0 ;

  for ( int i = 0; i < numFibers; i++ )
    success += theFibers[i]->revertToStart( ) ;

  return success ;
}


//mass per unit area
double
MembranePlateFiberSection::getRho( )
{

  double weight ;

  double rhoH = 0.0 ;

  const double *wg = (integrationType == 0) ? wgLobatto : wgGauss;        
  
  for ( int i = 0; i < numFibers; i++ ) {
    
    weight = ( 0.5*h ) * wg[i] ;

    rhoH += ( theFibers[i]->getRho() ) * weight ;

  }

  return rhoH ;

}


//receive the strainResultant 
int MembranePlateFiberSection ::
setTrialSectionDeformation( const Vector &strainResultant_from_element)
{
  this->strainResultant = strainResultant_from_element ;

  static Vector strain(numFibers) ;

  int success = 0 ;

  double z ;

  const double *sg = (integrationType == 0) ? sgLobatto : sgGauss;  
  
  for ( int i = 0; i < numFibers; i++ ) {

      z = ( 0.5*h ) * sg[i] ;
  
      strain(0) =  strainResultant(0)  - z*strainResultant(3) ;

      strain(1) =  strainResultant(1)  - z*strainResultant(4) ;

      strain(2) =  strainResultant(2)  - z*strainResultant(5) ;

      strain(4) =  root56*strainResultant(6) ;

      strain(3) =  root56*strainResultant(7) ;
  
      success += theFibers[i]->setTrialStrain( strain ) ;

  } //end for i

  return success ;
}


//send back the strainResultant
const Vector& MembranePlateFiberSection::getSectionDeformation( )
{
  return this->strainResultant ;
}


//send back the stressResultant 
const Vector&  MembranePlateFiberSection::getStressResultant( )
{

  static Vector stress(numFibers) ;

  double z, weight ;

  stressResultant.Zero( ) ;

  const double *sg = (integrationType == 0) ? sgLobatto : sgGauss;
  const double *wg = (integrationType == 0) ? wgLobatto : wgGauss;        
  
  for ( int i = 0; i < numFibers; i++ ) {

      z = ( 0.5*h ) * sg[i] ;

      weight = ( 0.5*h ) * wg[i] ;

      stress = theFibers[i]->getStress( ) ;
  
      //membrane
      stressResultant(0)  +=  stress(0)*weight ;

      stressResultant(1)  +=  stress(1)*weight ;

      stressResultant(2)  +=  stress(2)*weight ;

      //bending moments
      stressResultant(3)  +=  ( z*stress(0) ) * weight ;

      stressResultant(4)  +=  ( z*stress(1) ) * weight ;

      stressResultant(5)  +=  ( z*stress(2) ) * weight ;

      //shear
      stressResultant(6)  += stress(4)*weight ;

      stressResultant(7)  += stress(3)*weight ;
  
  } //end for i

   //modify shear 
   stressResultant(6) *= root56 ;  
   stressResultant(7) *= root56 ;

   return this->stressResultant ;
}


//send back the tangent 
const Matrix&  MembranePlateFiberSection::getSectionTangent( )
{
  static Matrix dd(5,5) ;

  static Matrix Aeps(5,8) ;

  static Matrix Asig(8,5) ;

  double z, weight ;

  tangent.Zero( ) ;

  const double *sg = (integrationType == 0) ? sgLobatto : sgGauss;
  const double *wg = (integrationType == 0) ? wgLobatto : wgGauss;      
  
  for ( int i = 0; i < numFibers; i++ ) {

      z = ( 0.5*h ) * sg[i] ;

      weight = (0.5*h) * wg[i] ;

/*      //compute Aeps

      Aeps.Zero( ) ;

      Aeps(0,0) = 1.0 ;
      Aeps(0,3) = -z ;

      Aeps(1,1) = 1.0 ;
      Aeps(1,4) = -z ;

      Aeps(2,2) = 1.0 ;
      Aeps(2,5) = -z ;

      Aeps(3,6) = root56 ;
      Aeps(4,7) = root56 ;

      //compute Asig

      Asig.Zero( ) ;

      Asig(0,0) = 1.0 ;
      Asig(3,0) = z ;

      Asig(1,1) = 1.0 ;
      Asig(4,1) = z ;

      Asig(2,2) = 1.0 ;
      Asig(5,2) = z ;

      Asig(6,3) = root56 ;
      Asig(7,4) = root56 ;
*/

      //compute the tangent

      dd = theFibers[i]->getTangent( ) ;

      dd *= weight ;

      //tangent +=  ( Asig * dd * Aeps ) ;   

//from MATLAB : tangent = 
//[      d11,           d12,           d13,        -z*d11,        -z*d12,        -z*d13,    d14*root56,    d15*root56]
//[      d21,           d22,           d23,        -z*d21,        -z*d22,        -z*d23,    d24*root56,    d25*root56]
//[      d31,           d32,           d33,        -z*d31,        -z*d32,        -z*d33,    d34*root56,    d35*root56]
//[     z*d11,         z*d12,         z*d13,      -z^2*d11,      -z^2*d12,      -z^2*d13,  z*d14*root56,  z*d15*root56]
//[     z*d21,         z*d22,         z*d23,      -z^2*d21,      -z^2*d22,      -z^2*d23,  z*d24*root56,  z*d25*root56]
//[     z*d31,         z*d32,         z*d33,      -z^2*d31,      -z^2*d32,      -z^2*d33,  z*d34*root56,  z*d35*root56]
//[  root56*d41,    root56*d42,    root56*d43, -root56*d41*z, -root56*d42*z, -root56*d43*z,  root56^2*d44,  root56^2*d45]
//[  root56*d51,    root56*d52,    root56*d53, -root56*d51*z, -root56*d52*z, -root56*d53*z,  root56^2*d54,  root56^2*d55]
 
      //row 1
//[      d11,           d12,           d13,        -z*d11,        -z*d12,        -z*d13,    d14*root56,    d15*root56]
      tangent(0,0) +=  dd(0,0) ;
      tangent(0,1) +=  dd(0,1) ;
      tangent(0,2) +=  dd(0,2) ;      
      tangent(0,3) +=  -z*dd(0,0) ;      
      tangent(0,4) +=  -z*dd(0,1) ;
      tangent(0,5) +=  -z*dd(0,2) ;
      tangent(0,6) +=  root56*dd(0,4) ;
      tangent(0,7) +=  root56*dd(0,3) ;

      //row 2
//[      d21,           d22,           d23,        -z*d21,        -z*d22,        -z*d23,    d24*root56,    d25*root56]
      tangent(1,0) +=  dd(1,0) ;
      tangent(1,1) +=  dd(1,1) ;
      tangent(1,2) +=  dd(1,2) ;      
      tangent(1,3) +=  -z*dd(1,0) ;      
      tangent(1,4) +=  -z*dd(1,1) ;
      tangent(1,5) +=  -z*dd(1,2) ;
      tangent(1,6) +=  root56*dd(1,4) ;
      tangent(1,7) +=  root56*dd(1,3) ;

      //row 3
//[      d31,           d32,           d33,        -z*d31,        -z*d32,        -z*d33,    d34*root56,    d35*root56]
      tangent(2,0) +=  dd(2,0) ;
      tangent(2,1) +=  dd(2,1) ;
      tangent(2,2) +=  dd(2,2) ;      
      tangent(2,3) +=  -z*dd(2,0) ;      
      tangent(2,4) +=  -z*dd(2,1) ;
      tangent(2,5) +=  -z*dd(2,2) ;
      tangent(2,6) +=  root56*dd(2,4) ;
      tangent(2,7) +=  root56*dd(2,3) ;

      //row 4
//[     z*d11,         z*d12,         z*d13,      -z^2*d11,      -z^2*d12,      -z^2*d13,  z*d14*root56,  z*d15*root56]
      tangent(3,0) +=  z*dd(0,0) ;
      tangent(3,1) +=  z*dd(0,1) ;
      tangent(3,2) +=  z*dd(0,2) ;      
      tangent(3,3) +=  -z*z*dd(0,0) ;      
      tangent(3,4) +=  -z*z*dd(0,1) ;
      tangent(3,5) +=  -z*z*dd(0,2) ;
      tangent(3,6) +=  z*root56*dd(0,4) ;
      tangent(3,7) +=  z*root56*dd(0,3) ;

      //row 5
//[     z*d21,         z*d22,         z*d23,      -z^2*d21,      -z^2*d22,      -z^2*d23,  z*d24*root56,  z*d25*root56]
      tangent(4,0) +=  z*dd(1,0) ;
      tangent(4,1) +=  z*dd(1,1) ;
      tangent(4,2) +=  z*dd(1,2) ;      
      tangent(4,3) +=  -z*z*dd(1,0) ;      
      tangent(4,4) +=  -z*z*dd(1,1) ;
      tangent(4,5) +=  -z*z*dd(1,2) ;
      tangent(4,6) +=  z*root56*dd(1,4) ;
      tangent(4,7) +=  z*root56*dd(1,3) ;

      //row 6
//[     z*d31,         z*d32,         z*d33,      -z^2*d31,      -z^2*d32,      -z^2*d33,  z*d34*root56,  z*d35*root56]
      tangent(5,0) +=  z*dd(2,0) ;
      tangent(5,1) +=  z*dd(2,1) ;
      tangent(5,2) +=  z*dd(2,2) ;      
      tangent(5,3) +=  -z*z*dd(2,0) ;      
      tangent(5,4) +=  -z*z*dd(2,1) ;
      tangent(5,5) +=  -z*z*dd(2,2) ;
      tangent(5,6) +=  z*root56*dd(2,4) ;
      tangent(5,7) +=  z*root56*dd(2,3) ;

      //row 7
//[  root56*d41,    root56*d42,    root56*d43, -root56*d41*z, -root56*d42*z, -root56*d43*z,  root56^2*d44,  root56^2*d45]
      tangent(6,0) +=  root56*dd(4,0) ;
      tangent(6,1) +=  root56*dd(4,1) ;
      tangent(6,2) +=  root56*dd(4,2) ;      
      tangent(6,3) +=  -root56*z*dd(4,0) ;      
      tangent(6,4) +=  -root56*z*dd(4,1) ;
      tangent(6,5) +=  -root56*z*dd(4,2) ;
      tangent(6,6) +=  root56*root56*dd(4,4) ;
      tangent(6,7) +=  root56*root56*dd(4,3) ;

      //row 8 
//[  root56*d51,    root56*d52,    root56*d53, -root56*d51*z, -root56*d52*z, -root56*d53*z,  root56^2*d54,  root56^2*d55]
      tangent(7,0) +=  root56*dd(3,0) ;
      tangent(7,1) +=  root56*dd(3,1) ;
      tangent(7,2) +=  root56*dd(3,2) ;      
      tangent(7,3) +=  -root56*z*dd(3,0) ;      
      tangent(7,4) +=  -root56*z*dd(3,1) ;
      tangent(7,5) +=  -root56*z*dd(3,2) ;
      tangent(7,6) +=  root56*root56*dd(3,4) ;
      tangent(7,7) +=  root56*root56*dd(3,3) ;

  } //end for i

  return this->tangent ;
}


//print out data
void  MembranePlateFiberSection::Print( OPS_Stream &s, int flag )
{
    if (flag == OPS_PRINT_PRINTMODEL_JSON) {
        s << "\t\t\t{";
        s << "\"name\": \"" << this->getTag() << "\", ";
        s << "\"type\": \"PlateFiber\", ";
        s << "\"thickness\": \"" << h << "\", ";
        s << "\"fibers\": [\n";
        for (int i = 0; i < numFibers; i++) {
            s << "\t\t\t\t{\"centroid\": " << (i+0.5) * h / numFibers << ", ";
            s << "\"material\": \"" << theFibers[i]->getTag() << "\"";
            if (i < numFibers - 1)
                s << "},\n";
            else
                s << "}\n";
        }
        s << "\t\t\t]}";
    }
    else {
        s << "MembranePlateFiberSection: \n ";
        s << "  Thickness h = " << h << endln;

        for (int i = 0; i < numFibers; i++) {
            theFibers[i]->Print(s, flag);
        }

        return;
    }
}

int 
MembranePlateFiberSection::sendSelf(int commitTag, Channel &theChannel) 
{
  int res = 0;

  // note: we don't check for dataTag == 0 for Element
  // objects as that is taken care of in a commit by the Domain
  // object - don't want to have to do the check if sending data
  int dataTag = this->getDbTag();
  
  static Vector vectData(2);
  vectData(0) = h;
  vectData(1) = integrationType;

  res += theChannel.sendVector(dataTag, commitTag, vectData);
  if (res < 0) {
      opserr << "WARNING MembranePlateFiberSection::sendSelf() - " << this->getTag() << " failed to send vectData\n";
      return res;
  }

  // Now quad sends the ids of its materials
  int matDbTag;
  
  static ID idData(2*numFibers+1);
  
  for (int i = 0; i < numFibers; i++) {
    idData(i) = theFibers[i]->getClassTag();
    matDbTag = theFibers[i]->getDbTag();
    // NOTE: we do have to ensure that the material has a database
    // tag if we are sending to a database channel.
    if (matDbTag == 0) {
      matDbTag = theChannel.getDbTag();
			if (matDbTag != 0)
			  theFibers[i]->setDbTag(matDbTag);
    }
    idData(i+numFibers) = matDbTag;
  }
  
  idData(2*numFibers) = this->getTag();

  res += theChannel.sendID(dataTag, commitTag, idData);
  if (res < 0) {
    opserr << "WARNING MembranePlateFiberSection::sendSelf() - " << this->getTag() << " failed to send ID\n";
			    
    return res;
  }

  // Finally, quad asks its material objects to send themselves
  for (int i = 0; i < numFibers; i++) {
    res += theFibers[i]->sendSelf(commitTag, theChannel);
    if (res < 0) {
      opserr << "WARNING MembranePlateFiberSection::sendSelf() - " << this->getTag() << " failed to send its Material\n";
      return res;
    }
  }

  return res;
}


int 
MembranePlateFiberSection::recvSelf(int commitTag, Channel &theChannel, FEM_ObjectBroker &theBroker)
{
  int res = 0;
  
  int dataTag = this->getDbTag();

  static Vector vectData(2);
  res += theChannel.recvVector(dataTag, commitTag, vectData);

  if (res < 0) {
      opserr << "WARNING MembranePlateFiberSection::recvSelf() - " << this->getTag() << " failed to recv vectData\n";
      return res;
  }

  h = vectData(0);
  integrationType = (int)vectData(1);

  static ID idData(2*numFibers+1);
  // Quad now receives the tags of its four external nodes
  res += theChannel.recvID(dataTag, commitTag, idData);
  if (res < 0) {
    opserr << "WARNING MembranePlateFiberSection::recvSelf() - " << this->getTag() << " failed to receive ID\n";
    return res;
  }

  this->setTag(idData(2*numFibers));

  if (theFibers[0] == 0) {
    for (int i = 0; i < numFibers; i++) {
      int matClassTag = idData(i);
      int matDbTag = idData(i+numFibers);
      // Allocate new material with the sent class tag
      theFibers[i] = theBroker.getNewNDMaterial(matClassTag);
      if (theFibers[i] == 0) {
	opserr << "MembranePlateFiberSection::recvSelf() - " <<
	  "Broker could not create NDMaterial of class type " << matClassTag << endln;
	return -1;
      }
      // Now receive materials into the newly allocated space
      theFibers[i]->setDbTag(matDbTag);
      res += theFibers[i]->recvSelf(commitTag, theChannel, theBroker);
      if (res < 0) {
	opserr << "MembranePlateFiber::recvSelf() - material " << i << "failed to recv itself\n";
	  
	return res;
      }
    }
  }
  // Number of materials is the same, receive materials into current space
  else {
    for (int i = 0; i < numFibers; i++) {
      int matClassTag = idData(i);
      int matDbTag = idData(i+numFibers);
      // Check that material is of the right type; if not,
      // delete it and create a new one of the right type
      if (theFibers[i]->getClassTag() != matClassTag) {
	delete theFibers[i];
	theFibers[i] = theBroker.getNewNDMaterial(matClassTag);
	if (theFibers[i] == 0) {
	  opserr << "MembranePlateFiberSection::recvSelf() - " << 
	    "Broker could not create NDMaterial of class type" << matClassTag << endln;
	  exit(-1);
	}
      }
      // Receive the material
      theFibers[i]->setDbTag(matDbTag);
      res += theFibers[i]->recvSelf(commitTag, theChannel, theBroker);
      if (res < 0) {
	opserr << "MembranePlateFiberSection::recvSelf() - material " << 
	  i << ", failed to recv itself\n";
	return res;
      }
    }
  }

  return res;
}
 


Response*
MembranePlateFiberSection::setResponse(const char **argv, int argc,
				       OPS_Stream &output)
{
  Response *theResponse =0;

  if (argc > 2 && strcmp(argv[0],"fiber") == 0) {
    
    int passarg = 2;
    int key = atoi(argv[1]);    
    
    if (key > 0 && key <= numFibers) {
      output.tag("FiberOutput");
      output.attr("number", key);
      const double *sg = (integrationType == 0) ? sgLobatto : sgGauss;
      const double *wg = (integrationType == 0) ? wgLobatto : wgGauss;      
      output.attr("zLoc", 0.5 * h * sg[key - 1]);
      output.attr("thickness", 0.5 * h * wg[key - 1]);
      theResponse = theFibers[key-1]->setResponse(&argv[passarg], argc-passarg, output);
      output.endTag();
    }

  }
  else if ((strcmp(argv[0],"sectionFailed") == 0) || 
	   (strcmp(argv[0],"hasSectionFailed") == 0) ||
	   (strcmp(argv[0],"hasFailed") == 0)) {
    theResponse = new MaterialResponse(this, 777, 0);
  }  

  if (theResponse == 0)
    return SectionForceDeformation::setResponse(argv, argc, output);

  return theResponse;
}


int 
MembranePlateFiberSection::getResponse(int responseID, Information &sectInfo)
{
  if (responseID == 777) {
    int count = 0;
    for (int j = 0; j < numFibers; j++) {    
      if (theFibers[j]->hasFailed() == true) {
	count += 1;
      }
    }
    if (count == numFibers)
      count = 1;
    else
      count = 0;
    
    return sectInfo.setInt(count);
  }
  
  // Just call the base class method ... don't need to define
  // this function, but keeping it here just for clarity
  return SectionForceDeformation::getResponse(responseID, sectInfo);
}

int MembranePlateFiberSection::setParameter(const char** argv, int argc, Parameter& param)
{
    // if the user explicitly wants to update a material in this section...
    if (argc > 1) {
        // case 1: fiber value (all fibers)
        // case 2: fiber id value (one specific fiber)
        if (strcmp(argv[0], "fiber") == 0 || strcmp(argv[0], "Fiber") == 0) {
            // test case 2 (one specific fiber) ...
            if (argc > 2) {
                int pointNum = atoi(argv[1]);
                if (pointNum > 0 && pointNum <= numFibers) {
                    return theFibers[pointNum - 1]->setParameter(&argv[2], argc - 2, param);
                }
            }
            // ... otherwise case 1 (all fibers), if the argv[1] is not a valid id
            int mixed_result = -1;
            for (int i = 0; i < numFibers; ++i) {
                if (theFibers[i]->setParameter(&argv[1], argc - 1, param) == 0)
                    mixed_result = 0; // if at least one fiber handles the param, make it successful
            }
            return mixed_result;
        }
    }
    // if we are here, the first keyword is not "fiber", so we can check for parameters
    // specific to this section (if any) or forward the request to all fibers.
    if (argc > 0) {
        // we don't have parameters for this section, so we directly forward it to all fibers.
        // placeholder for future implementations: if we will have parameters for this class, check them here
        // before forwarding to all fibers
        int mixed_result = -1;
        for (int i = 0; i < numFibers; ++i) {
            if (theFibers[i]->setParameter(argv, argc, param) == 0)
                mixed_result = 0; // if at least one fiber handles the param, make it successful
        }
        return mixed_result;
    }
    // fallback to base class implementation
    return SectionForceDeformation::setParameter(argv, argc, param);
}

int MembranePlateFiberSection::updateParameter(int parameterID, Information& info)
{
    // placeholder for future implementations: if we will have parameters for this class, update them here
    return SectionForceDeformation::updateParameter(parameterID, info);
}
