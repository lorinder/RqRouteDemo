#ifndef RQ_MEMBLOCKS
#define RQ_MEMBLOCKS

#include <stddef.h>

#include <rq_api.h>

typedef struct {
	/* Limits on the parameters */
	int nMaxK;
	int nMaxExtraIn;
	int nMaxOut;
	size_t nMaxT;

	/* Memory pointers & sizes */

	/* Intermediate Block */
	size_t nInterWorkMemSize;
	RqInterWorkMem* pInterWorkMem;
	size_t nInterProgMemSize;
	RqInterProgram* pInterProgMem;
	size_t nInterSymMemSize;
	void* pInterSymMem;

	/* Output symbols */
	size_t nOutWorkMemSize;
	RqOutWorkMem* pOutWorkMem;
	size_t nOutProgMemSize;
	RqOutProgram* pOutProgMem;
} RqMemBlocks;

int RqMemBlocksInit(RqMemBlocks* pMemBlocks,
			int nMaxK,
			int nMaxExtraIn,
			int nMaxOut,
			size_t nMaxT);

int RqMemBlocksIncludeCase(RqMemBlocks* pMemBlocks,
			int nMaxK,
			int nMaxExtraIn,
			int nMaxOut,
			size_t nMaxT);

void RqMemBlocksFreeMem(RqMemBlocks* pMemBlocks);


#endif /* RQ_MEMBLOCKS */
