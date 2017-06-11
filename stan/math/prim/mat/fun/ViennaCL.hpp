#ifndef STAN_MATH_PRIM_MAT_FUN_VIENNACL_HPP
  #define STAN_MATH_PRIM_MAT_FUN_VIENNACL_HPP

  #ifdef STAN_GPU
    #ifndef VIENNACL_WITH_OPENCL
      #define VIENNACL_WITH_OPENCL 1
    #endif
  #else
    #define STAN_GPU 0
  #endif

  #ifndef VIENNACL_WITH_EIGEN
    #define VIENNACL_WITH_EIGEN 1
  #endif

  #include <viennacl/vector.hpp>
  #include <viennacl/matrix.hpp>
  #include <viennacl/linalg/direct_solve.hpp>
  #include <viennacl/linalg/lu.hpp>
  #include <viennacl/linalg/ilu_operations.hpp>

  #if STAN_GPU
        static const char * custom_kernel_lower_upper =
        "__kernel void copy_lower_tri_upper_tri(\n"
        "          __global double * vec1,\n"
        "          unsigned int M,\n"
        "          unsigned int N, \n"
        "          unsigned int Npad) \n"
        "{ \n"
        "    int i = get_global_id(0); \n"
        "    int j = get_global_id(1); \n"
        "    if(i < M &&  j< N && i<j ){\n"
        "         vec1[i*Npad+j] = vec1[j*Npad+i];\n"
        "    }\n"
        "};\n";
        
        static const char * custom_kernel_diagonal =
        "__kernel void diagonal_mul(\n"
        "          __global double * vec1,\n"
        "          unsigned int M,\n"
        "          unsigned int N, \n"
        "          unsigned int Npad) \n"
        "{ \n"
        "    int i = get_global_id(0); \n"
        "    int j = get_global_id(1); \n"
        "    if(i ==j && i < M &&  j < N ){\n"
        "         vec1[i*Npad+j] *= 0.5;\n"
        "    }\n"
        "};\n";
        
        static const char * custom_kernel_inverse_step1 =
        "__kernel void inverse_step1(__global double* ap,__global double* vv, int remainder,int part_size_fixed, int M, int Mpad){ \n"
        "	 \n"
        "	int indeks=get_global_id(0); \n"
        "	int i=indeks*part_size_fixed; \n"
        "	int part_size; \n"
        "	double faktor; \n"
        "	if(indeks<remainder){ \n"
        "		i+=indeks; \n"
        "		part_size=part_size_fixed+1; \n"
        "	}else{ \n"
        "		i+=remainder; \n"
        "		part_size=part_size_fixed; \n"
        "	}	 \n"
        "	int offset=indeks*(part_size_fixed+1)*(part_size_fixed+1); \n"
        "	 \n"
        "    for(int p=0;p<part_size;p++){ \n"
        "      for(int r=0;r<part_size;r++){ \n"
        "        if(p==r) \n"
        "          vv[offset+p*(part_size_fixed+1)+r]=1; \n"
        "        else \n"
        "          vv[offset+p*(part_size_fixed+1)+r]=0; \n"
        "      } \n"
        "    } \n"
        "	 \n"
        "    for (unsigned int ii = 0; ii < part_size; ii++){ \n"
        "      if(ii>0){ \n"
        "        for (unsigned int j = ii; j < part_size; j++) { \n"
        "          faktor=ap[(i+ii-1)*Mpad+(j+i)]; \n"
        "          for (unsigned int k = 0; k < part_size; k++) { \n"
        "            vv[offset+j*(part_size_fixed+1)+k]=vv[offset+j*(part_size_fixed+1)+k]-faktor*vv[offset+(ii-1)*(part_size_fixed+1)+k]; \n"
        "          } \n"
        "        } \n"
        "	    } \n"
        "      faktor=ap[(ii+i)*Mpad+ii+i]; \n"
        "      for (unsigned int k = 0; k < part_size; k++) { \n"
        "        vv[offset+ii*(part_size_fixed+1)+k]=vv[offset+ii*(part_size_fixed+1)+k]/faktor;   \n"
        "      } \n"
        "    } \n"
        "	 \n"
        "    for(int p=0;p<part_size;p++){ \n"
        "      for(int r=0;r<part_size;r++){ \n"
        "        ap[(i+r)*Mpad+(p+i)]=vv[offset+p*(part_size_fixed+1)+r]; \n"
        "      } \n"
        "    } \n"
        "} \n";
        
        static const char * custom_kernel_inverse_step2_3 =
        "#define WPT 4  \n"
        "#define RTS	8 \n"
        "#define TS2 32 \n"
        "__kernel void inverse_step2(__global double* ap,__global int* sizes,__global double* MM, int repeat, int remainder,int part_size_fixed, int M, int Mpad){ \n"
        " \n"
        "	int n=get_global_id(2)*2; \n"
        "	double sum=0; \n"
        "	int part_size1=0,part_size2=0; \n"
        " int offset_i,offset_j; \n"
        "	 \n"
        "	for(int r=n*repeat;r<(n+1)*repeat;r++) \n"
        "		part_size1+= sizes[r]; \n"
        " \n"
        "	for(int r=(n+1)*repeat;r<(n+2)*repeat;r++) \n"
        "		part_size2+= sizes[r]; \n"
        "	int sizeM=repeat*(part_size_fixed+1); \n"
        "	offset_i=(n+1)*repeat*part_size_fixed;offset_j=n*repeat*part_size_fixed; \n"
        "	if(((n+1)*repeat)<=remainder) \n"
        " offset_i+=(n+1)*repeat; \n"
        "	else \n"
        "		offset_i+=remainder; \n"
        "		 \n"
        "	if((n*repeat)<=remainder) \n"
        "		offset_j+=n*repeat; \n"
        "	else \n"
        "		offset_j+=remainder; \n"
        "	 \n"
        "	const int row = get_local_id(0); \n"
        "  const int col = get_local_id(1); \n"
        "  const int i = TS2*get_group_id(0) + row; \n"
        "  const int j = TS2*get_group_id(1) + col; \n"
        " \n"
        "  __local double Asub[TS2][TS2]; \n"
        "  __local double Bsub[TS2][TS2]; \n"
        "     \n"
        "  double acc[WPT]; \n"
        "  for (int w=0; w<WPT; w++) { \n"
        "      acc[w] = 0.0f; \n"
        "  } \n"
        " \n"
        "  const int numTiles = (part_size2+TS2-1)/TS2; \n"
        "	 \n"
        "	sum=0; \n"
        "   \n"
        "	for (int t=0; t<numTiles; t++) { \n"
        "		for (int w=0; w<WPT; w++) { \n"
        "			const int tiledRow = TS2*t + row; \n"
        "			const int tiledCol = TS2*t + col; \n"
        "			 \n"
        "			if(i<part_size2 && (tiledCol+w*RTS)<part_size1){ \n"
        "				Asub[col+w*RTS][row] = ap[(tiledCol+offset_j+part_size1+w*RTS)*Mpad+i+offset_i]; \n"
        "			}else{ \n"
        "				Asub[col+w*RTS][row] = 0.0; \n"
        "			} \n"
        "				 \n"
        "			if((j+w*RTS)<part_size1 && tiledRow<part_size2){ \n"
        "				Bsub[col+w*RTS][row] = ap[(j+offset_j+w*RTS)*Mpad+tiledRow+offset_i]; \n"
        "			}else{		 \n"
        "				Bsub[col+w*RTS][row] = 0.0; \n"
        "			} \n"
        "		} \n"
        "			 \n"
        "		barrier(CLK_LOCAL_MEM_FENCE); \n"
        "	 \n"
        "		for(int k=0;k<TS2;k++){				 \n"
        "			for (int w=0; w<WPT; w++) { \n"
        "				acc[w]+=Asub[k][row]*Bsub[col+w*RTS][k]; \n"
        "			} \n"
        "		}  \n"
        "		barrier(CLK_LOCAL_MEM_FENCE); \n"
        "	} \n"
        " \n"
        "	for (int w=0; w<WPT; w++) { \n"
        "		if(i<part_size2&&(j+w*RTS)<part_size1){ \n"
        "			MM[(n/2)*(sizeM)*(sizeM)+i*part_size1+j+w*RTS]=acc[w]; \n"
        "		} \n"
        "	} \n"
        "} \n"
        " \n"
        "__kernel void inverse_step3(__global double* ap,__global int* sizes,__global double* MM, int repeat, int remainder,int part_size_fixed, int M, int Mpad){ \n"
        " \n"
        "	int n=get_global_id(2)*2; \n"
        "	double sum=0; \n"
        "	int part_size1=0,part_size2=0; \n"
        "	int offset_i,offset_j; \n"
        "	for(int r=n*repeat;r<(n+1)*repeat;r++) \n"
        "		part_size1+= sizes[r]; \n"
        " \n"
        "	for(int r=(n+1)*repeat;r<(n+2)*repeat;r++) \n"
        "		part_size2+= sizes[r]; \n"
        "	 \n"
        "	int sizeM=repeat*(part_size_fixed+1); \n"
        "	offset_i=(n+1)*repeat*part_size_fixed;offset_j=n*repeat*part_size_fixed; \n"
        "	if(((n+1)*repeat)<=remainder) \n"
        "		offset_i+=(n+1)*repeat; \n"
        "	else \n"
        "		offset_i+=remainder; \n"
        "		 \n"
        "	if((n*repeat)<=remainder) \n"
        "		offset_j+=n*repeat; \n"
        "	else \n"
        "		offset_j+=remainder; \n"
        " \n"
        "	 \n"
        "	const int row = get_local_id(0); \n"
        "  const int col = get_local_id(1); \n"
        "  const int i = TS2*get_group_id(0) + row; \n"
        "  const int j = TS2*get_group_id(1) + col; \n"
        " \n"
        "  __local double Asub[TS2][TS2]; \n"
        "  __local double Bsub[TS2][TS2]; \n"
        " \n"
        "  double acc[WPT]; \n"
        "  for (int w=0; w<WPT; w++) { \n"
        "      acc[w] = 0.0f; \n"
        "  } \n"
        " \n"
        "  const int numTiles = (part_size1+TS2-1)/TS2; \n"
        "	 \n"
        "	sum=0; \n"
        "	for (int t=0; t<numTiles; t++) { \n"
        "		for (int w=0; w<WPT; w++) { \n"
        "			const int tiledRow = TS2*t + row; \n"
        "			const int tiledCol = TS2*t + col; \n"
        "			if(i<part_size2 && (tiledCol+w*RTS)<part_size1 ){ \n"
        "				Asub[col+w*RTS][row] = MM[(n/2)*(sizeM)*(sizeM)+i*part_size1+tiledCol+w*RTS]; \n"
        "			}else{ \n"
        "				Asub[col+w*RTS][row] = 0.0; \n"
        "			} \n"
        "			if((j+w*RTS)<part_size1 && (j+offset_j+w*RTS)<M ){ \n"
        "				Bsub[col+w*RTS][row] = ap[(j+offset_j+w*RTS)*Mpad+(tiledRow+offset_i-part_size1)]; \n"
        "			}else{		 \n"
        "				Bsub[col+w*RTS][row] = 0.0; \n"
        "			} \n"
        "		} \n"
        "		barrier(CLK_LOCAL_MEM_FENCE); \n"
        "		 \n"
        "		for(int k=0;k<TS2;k++){ \n"
        "			for (int w=0; w<WPT; w++) { \n"
        "				acc[w]+=Asub[k][row]*Bsub[col+w*RTS][k]; \n"
        "			} \n"
        "		} 	 \n"
        "		barrier(CLK_LOCAL_MEM_FENCE);		 \n"
        "	} \n"
        "	for (int w=0; w<WPT; w++) { \n"
        "   if(i<part_size2&&(j+w*RTS)<part_size1){ \n"
        "			ap[(j+offset_j+w*RTS)*Mpad+i+offset_i]=-acc[w]; \n"
        "		} \n"
        " } \n"
        "	 \n"
        "}  \n";
      viennacl::ocl::program & my_prog = viennacl::ocl::current_context().add_program(
        custom_kernel_lower_upper, "custom_kernel_lower_upper");
      viennacl::ocl::kernel & my_kernel_mul = my_prog.get_kernel("copy_lower_tri_upper_tri");
      
      viennacl::ocl::program & my_prog_diag = viennacl::ocl::current_context().add_program(
        custom_kernel_diagonal, "custom_kernel_diagonal");
      viennacl::ocl::kernel & my_kernel_diag = my_prog_diag.get_kernel("diagonal_mul");

      viennacl::ocl::program & my_prog_inv1 = viennacl::ocl::current_context().add_program(
        custom_kernel_inverse_step1, "custom_kernel_inverse_step1");
      viennacl::ocl::kernel & my_kernel_inv1 = my_prog_inv1.get_kernel("inverse_step1");
      
      viennacl::ocl::program & my_prog_inv2_3 = viennacl::ocl::current_context().add_program(
        custom_kernel_inverse_step2_3, "custom_kernel_inverse_step2_3");
      viennacl::ocl::kernel & my_kernel_inv2 = my_prog_inv2_3.get_kernel("inverse_step2");
      viennacl::ocl::kernel & my_kernel_inv3 = my_prog_inv2_3.get_kernel("inverse_step3");
  #endif

#endif

