/*=========================================================================

  Library   : Image Registration Toolkit (IRTK)
  Module    : $Id: basename.cc 2 2008-12-23 12:40:14Z dr $
  Copyright : Imperial College, Department of Computing
              Visual Information Processing (VIP), 2008 onwards
  Date      : $Date: 2008-12-23 12:40:14 +0000 (Tue, 23 Dec 2008) $
  Version   : $Revision: 2 $
  Changes   : $Author: dr $

=========================================================================*/

#include <irtkCommon.h>

// Copied from glibc-2.1.1

char *basename2(const char *filename)
{
  char *p = strrchr (const_cast<char*>(filename), '/');
  return p ? p + 1 : (char *) filename;
}
