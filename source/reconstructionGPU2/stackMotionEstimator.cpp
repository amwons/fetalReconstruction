/*=========================================================================
Library   : Image Registration Toolkit (IRTK)
Module    : $Id: irtkReconstructionCuda.cc 1 2013-11-15 14:36:30 bkainz $
Copyright : Imperial College, Department of Computing
Visual Information Processing (VIP), 2011 onwards
Date      : $Date: 2013-11-15 14:36:30 +0100 (Fri, 15 Nov 2013) $
Version   : $Revision: 1 $
Changes   : $Author: bkainz $

Copyright (c) 2014, Bernhard Kainz, Markus Steinberger,
Maria Murgasova, Kevin Keraudren
All rights reserved.

If you use this work for research we would very much appreciate if you
could cite
TODO: update as soon as accepted
Bernhard Kainz, Markus Steinberger, Christina Malamateniou, Wolfgang Wein, Maria Murgasova,
Kevin Keraudren, Thomas Torsney-Weir, Mary Rutherford, Joseph  V. Hajnal, and Daniel Rueckert:
Generalized Fast Volume Reconstruction from Motion Corrupted 2D Slices.
IEEE Transactions on Medical Imaging, under review, 2014

IRTK IS PROVIDED UNDER THE TERMS OF THIS CREATIVE
COMMONS PUBLIC LICENSE ("CCPL" OR "LICENSE"). THE WORK IS PROTECTED BY
COPYRIGHT AND/OR OTHER APPLICABLE LAW. ANY USE OF THE WORK OTHER THAN
AS AUTHORIZED UNDER THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.

BY EXERCISING ANY RIGHTS TO THE WORK PROVIDED HERE, YOU ACCEPT AND AGREE
TO BE BOUND BY THE TERMS OF THIS LICENSE. TO THE EXTENT THIS LICENSE MAY BE
CONSIDERED TO BE A CONTRACT, THE LICENSOR GRANTS YOU THE RIGHTS CONTAINED
HERE IN CONSIDERATION OF YOUR ACCEPTANCE OF SUCH TERMS AND CONDITIONS.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#if HAVE_CULA

#include "stackMotionEstimator.h"
#include <thrust/functional.h>
#include <thrust/transform_reduce.h>
#include <thrust/extrema.h>
#include <thrust/count.h>

stackMotionEstimator::stackMotionEstimator()
{
  status = culaInitialize();
  checkStatus(status);
}


stackMotionEstimator::~stackMotionEstimator()
{
  culaShutdown();
}

struct is_larger_eps
{
  float alpha;
  is_larger_eps(float eps){alpha = eps;};
  __host__ __device__
    bool operator()(float &x)
  {
    return x > alpha;
  }
};

struct is_larger_zero
{
  __host__ __device__
    bool operator()(const int &x)
  {
    return (x > 0);
  }
};

float stackMotionEstimator::evaluateStackMotion(irtkRealImage* img)
{

  //TODO do this non static, move shtdown and init of CULA in cons/destructor

  double min_, max_;
  img->GetMinMax(&min_,&max_);
  printf("min: %f, max: %f \n", min_, max_);

  irtkGenericImage<float> imgN;
  imgN.Initialize(img->GetImageAttributes());
  for(int i = 0; i < imgN.GetNumberOfVoxels(); i++)
  {
    imgN.GetPointerToVoxels()[i] = (float)((img->GetPointerToVoxels()[i]-min_)/(max_-min_));
  }

  //we take only central slices into account
  unsigned int slicesElems = imgN.GetX()*imgN.GetY();
  unsigned int aThirdSlices = imgN.GetZ();//(imgN.GetZ()/3.0);

  //use rank minimization paradigm
  int M = slicesElems; //one image per row
  int N = aThirdSlices; // num rows

  int LDA = M;
  int LDU = M;
  int LDVT = N;

  culaDeviceFloat* A = NULL;
  culaDeviceFloat* S = NULL;
  culaDeviceFloat* U = NULL;
  culaDeviceFloat* VT = NULL;

  char jobu = 'N';
  char jobvt = 'N';

  printf("testing motion %d %d %d MB\n", M, N, LDU, LDU*M*sizeof(culaDeviceFloat)/1024/1024);

  unsigned int num_ev = imin(M,N);
  //cudaMalloc output
  checkCudaErrors(cudaMalloc((void**)&A, M*N*sizeof(culaDeviceFloat))); 
  checkCudaErrors(cudaMalloc((void**)&S, num_ev*sizeof(culaDeviceFloat))); 

  checkCudaErrors(cudaMemcpy(A, imgN.GetPointerToVoxels() /*+ slicesElems*aThirdSlices*/, M*N*sizeof(culaDeviceFloat), cudaMemcpyHostToDevice));


  //TODO #if USE_CPU
  status = culaDeviceSgesvd(jobu, jobvt, M, N, A, LDA, S, U, LDU, VT, LDVT);
  checkStatus(status);

  float* S_host = (float*)malloc(num_ev*sizeof(culaDeviceFloat));
  checkCudaErrors(cudaMemcpy(S_host, S, num_ev*sizeof(culaDeviceFloat), cudaMemcpyDeviceToHost));

  //why does thrust not work here? do we need to compile that with nvcc
  //int result = thrust::count_if(S, S+N, is_larger_zero());
  // thrust::transform(A, A+N, A, ptr_addon, plus<float>());

  int result = 0;
  float alpha = 10;
  float sum = 0;
  for(int i = 0; i < num_ev; i++)
  {
    printf("%f ", S_host[i]);
    if(S_host[i] > 10) 
    {
      result++;
      sum += abs(S_host[i]);
    }
  }
  printf("\n");

  printf("approx. Rank = %d (smaller is better)\n", result);

  //print S for debug

  //count only eigenvalues > eps = Rank

  cudaFree(A);
  cudaFree(S); 

  printf("done!\n");

  //result/alpha
  return float(result/alpha);//result*0.4 - 2.0; //pow(float(result/alpha),2.0);//exp(float(result/alpha));//pow(float(result/alpha),2.0);
}

void stackMotionEstimator::checkStatus(culaStatus status)
{
  char buf[256];

  if(!status)
    return;

  culaGetErrorInfoString(status, culaGetErrorInfo(), buf, sizeof(buf));
  printf("%s\n", buf);

  culaShutdown();
  exit(EXIT_FAILURE);
}

#endif