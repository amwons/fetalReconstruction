/*=========================================================================

  Library   : Image Registration Toolkit (IRTK)
  Module    : $Id: irtkModelAffineRegistration.h 665 2012-08-29 21:45:17Z ws207 $
  Copyright : Imperial College, Department of Computing
              Visual Information Processing (VIP), 2009 onwards
  Date      : $Date: 2012-08-29 22:45:17 +0100 (Wed, 29 Aug 2012) $
  Version   : $Revision: 665 $
  Changes   : $Author: ws207 $

=========================================================================*/

#ifndef _IRTKMODELAFFINEREGISTRATION_H

#define _IRTKMODELAFFINEREGISTRATION_H

#ifdef HAS_VTK

/**
 * Filter for affine registration based on voxel similarity measures.
 *
 */

class irtkModelAffineRegistration : public irtkModelRigidRegistration
{

public:

  /** Sets the output for the registration filter. The output must be a affine
   *  transformation. The current parameters of the affine transformation are
   *  used as initial guess for the affine registration. After execution of the
   *  filter the parameters of the affine transformation are updated with the
   *  optimal transformation parameters.
   */
  virtual void SetOutput(irtkTransformation *);

  /// Returns the name of the class
  virtual const char *NameOfClass();

  /// Print information about the progress of the registration
  virtual void Print();
};

inline void irtkModelAffineRegistration::SetOutput(irtkTransformation *transformation)
{
  if (strcmp(transformation->NameOfClass(), "irtkAffineTransformation") != 0) {
    cerr << "irtkImageAffineRegistration::SetOutput: Transformation must be affine"
         << endl;
    exit(0);
  }
  _transformation = transformation;
}

inline const char *irtkModelAffineRegistration::NameOfClass()
{
  return "irtkImageAffineRegistration";
}

inline void irtkModelAffineRegistration::Print()
{
  _transformation->Print();
}

#endif

#endif
