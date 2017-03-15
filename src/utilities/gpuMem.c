#if defined(HYPRE_USE_GPU) || defined(HYPRE_USE_MANAGED)
#include "gpuErrorCheck.h"
#include "hypre_nvtx.h"
#include <stdlib.h>
#include <stdint.h>
#include <cublas_v2.h>
#include <cusparse.h>
#include "_hypre_utilities.h"
#include "gpuMem.h"
int ggc(int id);
#define FULL_WARN
//cublasHandle_t getCublasHandle();
//cusparseHandle_t getCusparseHandle();
void hypreGPUInit(){
  char pciBusId[80];
  int myid;
  int nDevices;
  int device;
  gpuErrchk(cudaGetDeviceCount(&nDevices));
  printf("There are %d GPUs on this node \n",nDevices);
  if (nDevices>1) printf("WARNING:: Code running without mpibind or similar\n");
  hypre_MPI_Comm_rank(hypre_MPI_COMM_WORLD, &myid );

  
  device=myid%nDevices; // Warning will not work on multiple nodes without mpibind
  gpuErrchk(cudaSetDevice(device));
  cudaDeviceGetPCIBusId ( pciBusId, 80, device);
  printf("MPI_RANK = %d runningon PCIBUS id :: %s as device %d of %d\n",myid,pciBusId,device,nDevices);
  //mempushc(NULL,0,1);
  // Initialize the handles and streams
  cublasHandle_t handle_1 = getCublasHandle();
  cusparseHandle_t handle_2 = getCusparseHandle();
  cudaStream_t s=getstream(4);
  /* Check is the arch flags used for compiling the cuda kernels match the device */
  CudaCompileFlagCheck();
  
}
void MemAdviseReadOnly(const void* ptr, int device){
  if (ptr==NULL) return;
    size_t size=mempush(ptr,0,0);
    if (size==0) printf("WARNING:: Operations with 0 size vector \n");
    gpuErrchk(cudaMemAdvise(ptr,size,cudaMemAdviseSetReadMostly,device));
}
void MemAdviseUnSetReadOnly(const void* ptr, int device){
  if (ptr==NULL) return;
    size_t size=mempush(ptr,0,0);
    if (size==0) printf("WARNING:: Operations with 0 size vector \n");
    gpuErrchk(cudaMemAdvise(ptr,size,cudaMemAdviseUnsetReadMostly,device));
}


void MemAdviseSetPrefLocDevice(const void *ptr, int device){
  if (ptr==NULL) return;
  gpuErrchk(cudaMemAdvise(ptr,mempush(ptr,0,0),cudaMemAdviseSetPreferredLocation,device));
}
void MemAdviseSetPrefLocHost(const void *ptr){
  if (ptr==NULL) return;
  gpuErrchk(cudaMemAdvise(ptr,mempush(ptr,0,0),cudaMemAdviseSetPreferredLocation,cudaCpuDeviceId));
}


void MemPrefetch(const void *ptr,int device,cudaStream_t stream){
  if (ptr==NULL) return;
  size_t size;
  //size=mempush(ptr,0,0);
  size=memsize(ptr);
  PUSH_RANGE_DOMAIN("MemPreFetchForce",4,0);
  /* Do a prefetch every time until a possible UM bug is fixed */
  if (size>0){
    PrintPointerAttributes(ptr);
    printf("Prefetch size %zu for %p %zu\n",size,ptr,mempush(ptr,0,0));
    gpuErrchk(cudaMemPrefetchAsync(ptr,size,device,stream));
    gpuErrchk(cudaStreamSynchronize(stream));
  POP_RANGE_DOMAIN(0);
  return;
 } else {
  //printf("WARNING :: Prefetching not done due to nvalid size  = %zu\n",size);
  return;
  }
  /* End forced prefetch */
  PUSH_RANGE_PAYLOAD("MemPreFetch",4,size);
  /* if (memloc(ptr,device)){ */
  /*   size=mempush(ptr,0,0); */
  /*   if (size==0) printf("WARNING:: Operations with 0 size vector \n"); */
  /*   gpuErrchk(cudaMemPrefetchAsync(ptr,size,device,stream)); */

  /* }  */
  POP_RANGE;
  return;
}
void MemPrefetchForce(const void *ptr,int device,cudaStream_t stream){
  if (ptr==NULL) return;
  //size_t size=mempush(ptr,0,0);
  size_t size=memsize(ptr);
  PUSH_RANGE_PAYLOAD("MemPreFetchForce",4,size);
  gpuErrchk(cudaMemPrefetchAsync(ptr,size,device,stream));
  POP_RANGE;
  return;
}

void MemPrefetchSized(const void *ptr,size_t size,int device,cudaStream_t stream){
  if (ptr==NULL) return;
  PUSH_RANGE_DOMAIN("MemPreFetchSized",4,0);
  /* Do a prefetch every time until a possible UM bug is fixed */
  if (size>0){
    gpuErrchk(cudaMemPrefetchAsync(ptr,size,device,stream));
    //gpuErrchk(cudaStreamSynchronize(stream));
    POP_RANGE_DOMAIN(0);
    return;
  } else {
    //printf("WARNING :: Prefetching not done due to nvalid size  = %zu\n",size);
    return;
  }
  return;
}

void MemPrefetchReadOnly(const void *ptr,int device,cudaStream_t stream){
  if (ptr==NULL) return;
  /* if (memloc(ptr,device)){ */
  /*   size_t size=mempush(ptr,0,0); */
  /*   PUSH_RANGE_PAYLOAD("MemAdviseRO",3,size); */
  /*   gpuErrchk(cudaMemAdvise(ptr,size,cudaMemAdviseSetReadMostly,device)); */
  /*   POP_RANGE; */
  /*   PUSH_RANGE_PAYLOAD("MemPreFetchRO",4,size); */
  /*   gpuErrchk(cudaMemPrefetchAsync(ptr,size,device,stream)); */
  /*   POP_RANGE; */
  /* }  */
  return;
}

void PrintPointerAttributesNew(const void *ptr){
  struct cudaPointerAttributes ptr_att;
  if (cudaPointerGetAttributes(&ptr_att,ptr)!=cudaSuccess){
    printf("PrintPointerAttributes:: Raw pointer\n");
    return;
  }
  if (ptr_att.isManaged){
    printf("PrintPointerAttributes:: Managed pointer\n");
    printf("Host address = %p, Device Address = %p\n",ptr_att.hostPointer, ptr_att.devicePointer);
    if (ptr_att.memoryType==cudaMemoryTypeHost) printf("Memory is located on host\n");
    if (ptr_att.memoryType==cudaMemoryTypeDevice) printf("Memory is located on device\n");
    printf("Device associated with this pointer is %d\n",ptr_att.device);
  } else {
    printf("PrintPointerAttributes:: Non-Managed & non-raw pointer\n Probably a device pointer\n");
  }
  return;
}
int OnHost(void *ptr){
  return 1;
  struct cudaPointerAttributes ptr_att;
  if (cudaPointerGetAttributes(&ptr_att,ptr)==cudaSuccess){
    return (ptr_att.memoryType==cudaMemoryTypeHost);
  } else {
    printf("PrintPointerAttributes:: Raw pointer\n");
    return 0;
  } 
}

// DeviceShare code doesnt work and triggers errors that are caught by cudaPeekLastError
int DeviceShare(void *ptr,size_t size){
uintptr_t p = (uintptr_t)ptr;
uintptr_t start,end,lastpage;
int pagesize = 64*1024; // 64KB HARDWIRED !!
start=p-p%pagesize;
end=p+size;
lastpage=end-end%pagesize;
uintptr_t i;
int dc=0,hc=0;
printf("Device Share s=%p, e =%p, actual=%p, pages = %lu\n",(void*)start,(void*)lastpage,ptr,size/pagesize);
for(i=start;i<=lastpage;i+=pagesize){
  void *pptr=(void*)i;
  struct cudaPointerAttributes ptr_att;
  if (cudaPointerGetAttributes(&ptr_att,pptr)==cudaSuccess){
    if (ptr_att.memoryType==cudaMemoryTypeHost)hc++;
	else dc++;
  } else {
    //printf("PrintPointerAttributes:: Raw pointer %p of size %d\n",ptr,size);
    // if (cudaPointerGetAttributes(&ptr_att,ptr)!=cudaSuccess) printf("Original Pointer is kaput as well\n");
    return -1;
  } 
}
int retval=(float)dc/(dc+hc)*100.0;
return retval;
}

/* Returns the same cublas handle with every call */
cublasHandle_t getCublasHandle(){
  cublasStatus_t stat;
  static cublasHandle_t handle;
  static int firstcall=1;
  if (firstcall){
    firstcall=0;
    stat = cublasCreate(&handle);
    if (stat!=CUBLAS_STATUS_SUCCESS) {
      printf("ERROR:: CUBLAS Library initialization failed\n");
      handle=0;
      exit(2);
    }
    cublasErrchk(cublasSetStream(handle,getstream(4)));
  } else return handle;
  return handle;
}

/* Returns the same cusparse handle with every call */
cusparseHandle_t getCusparseHandle(){
  cusparseStatus_t status;
  static cusparseHandle_t handle;
  static int firstcall=1;
  if (firstcall){
    firstcall=0;
    status= cusparseCreate(&handle);
    if (status != CUSPARSE_STATUS_SUCCESS) {
      printf("ERROR:: CUSPARSE Library initialization failed\n");
      handle=0;
      exit(2);
    }
    cusparseErrchk(cusparseSetStream(handle,getstream(4)));
  } else return handle;
  return handle;
}
/* C version of mempush using linked lists */


size_t mempush(const void *ptr, size_t size, int action){
  static node* head=NULL;
  static int nc=0;
  node *found=NULL;
  if (!head){
    if ((size<=0)||(action==1)) {
      fprintf(stderr,"mempush can start only with an insertion or a size call \n");
      return 0;
    }
    //printf("GENERATING head node ..");
    head = (node*)malloc(sizeof(node));
    head->ptr=ptr;
    head->size=size;
    head->next=NULL;
    //printf("Done\n");
    //printlist(head);
    nc++;
    return size;
  } else {
    // Purge an address
    if (action==1){
      found=memfind(head,ptr);
      if (found){
	memdel(&head, found);
	//	printf("DELETING FROM LIST\n");
	//printlist(head,nc);
	nc--;
	return 0;
      } else {
#ifdef FULL_WARN
	fprintf(stderr,"ERROR :: Pointer for deletion not found in linked list %p\n",ptr);
	//printlist(head,nc);
#endif
	return 0;
      }
    } // End purge
    
    // Insertion
    if (size>0){
      found=memfind(head,ptr);
      if (found){
#ifdef FULL_WARN
	fprintf(stderr,"ERROR :: Pointer for insertion already in use in linked list %p\n",ptr);
	//printlist(head,nc);
#endif
	return 0;
      } else {
	nc++;
	meminsert(&head,ptr,size);
	//printf("INSERTING INTO LIST\n");
	//printlist(head,nc);
	return 0;
      }
    }

    // Getting allocation size
    found=memfind(head,ptr);
    if (found){
      return found->size;
    } else{
#ifdef FULL_WARN
      fprintf(stderr,"ERROR :: Pointer for size check NOT found in linked list\n");
#endif
      return 0;
    }
  }
}
node *memfind(node *head, const void *ptr){
  node *next;
  next=head;
  while(next!=NULL){
    if (next->ptr==ptr) return next;
    next=next->next;
  }
  return NULL;
}
void memdel(node **head, node *found){
  node *next;
  if (found==*head){
    next=(*head)->next;
    free(*head);
    *head=next;
    return;
  }
  next=*head;
  while(next->next!=found){
    next=next->next;
  }
  next->next=next->next->next;
  free(found);
  return;
}
void meminsert(node **head, const void  *ptr,size_t size){
  node *nhead;
  nhead = (node*)malloc(sizeof(node));
  nhead->ptr=ptr;
  nhead->size=size;
  nhead->next=*head;
  *head=nhead;
  return;
}
void printlist(node *head,int nc){
  node *next;
  next=head;
  printf("Node count %d \n",nc);
  while(next!=NULL){
    printf("Address %p of size %zu \n",next->ptr,next->size);
    next=next->next;
  }
}

cudaStream_t getstream(int i){
  static int firstcall=1;
  const int MAXSTREAMS=10;
  static cudaStream_t s[MAXSTREAMS];
  if (firstcall){
    for(int jj=0;jj<MAXSTREAMS;jj++)
      gpuErrchk(cudaStreamCreateWithFlags(&s[jj],cudaStreamNonBlocking));
    printf("Created streams ..\n");
    firstcall=0;
    //nvtxNameCudaStream(s[4], "HYPRE_COMPUTE_STREAM");
  }
  if (i<MAXSTREAMS) return s[i];
  fprintf(stderr,"ERROR in getstream in utilities/gpuMem.c %d is greater than MAXSTREAMS = %d\n Returning default stream",i,MAXSTREAMS);
  return 0;
}

nvtxDomainHandle_t getdomain(int i){
    static int firstcall=1;
    const int MAXDOMAINS=1;
    static nvtxDomainHandle_t h[MAXDOMAINS];
    if (firstcall){
      h[0]= nvtxDomainCreateA("HYPRE");
      firstcall=0;
    }
    if (i<MAXDOMAINS) return h[i];
    fprintf(stderr,"ERROR in getdomain in utilities/gpuMem.c %d  is greater than MAXDOMAINS = %d \n Returning default domain",i,MAXDOMAINS);
    return NULL;
  }

cudaEvent_t getevent(int i){
  static int firstcall=1;
  const int MAXEVENTS=10;
  static cudaEvent_t s[MAXEVENTS];
  if (firstcall){
    for(int jj=0;jj<MAXEVENTS;jj++)
      gpuErrchk(cudaEventCreateWithFlags(&s[i],cudaEventDisableTiming));
    printf("Created events ..\n");
    firstcall=0;
  }
  if (i<MAXEVENTS) return s[i];
  fprintf(stderr,"ERROR in getevent in utilities/gpuMem.c %d is greater than MAXEVENTS = %d\n Returning default stream",i,MAXEVENTS);
  return 0;
}
  
#endif