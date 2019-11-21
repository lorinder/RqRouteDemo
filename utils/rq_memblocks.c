#include <stdlib.h>

#include "rq_memblocks.h"

#define ALIGN 4096

int RqMemBlocksInit(RqMemBlocks* pMemBlocks,
			int nMaxK,
			int nMaxExtraIn,
			int nMaxOut,
			size_t nMaxT)
{
	RqMemBlocks zero = { 0 };
	*pMemBlocks = zero;
	return RqMemBlocksIncludeCase(pMemBlocks,
			nMaxK,
			nMaxExtraIn,
			nMaxOut,
			nMaxT);
}

int RqMemBlocksIncludeCase(RqMemBlocks* pMemBlocks,
			int nMaxK,
			int nMaxExtraIn,
			int nMaxOut,
			size_t nMaxT)
{

	/* Compute new memory block sizes */
	size_t nInterWorkMemSize, nInterProgMemSize;
	size_t nInterSymNum, nInterSymMemSize;
	size_t nOutWorkMemSize, nOutProgMemSize;

	int rc;
	rc = RqInterGetMemSizes(nMaxK,
			nMaxExtraIn,
			&nInterWorkMemSize,
			&nInterProgMemSize,
			&nInterSymNum);
	if (rc < 0)
		return rc;
	nInterSymMemSize = nMaxT * nInterSymNum;
	rc = RqOutGetMemSizes(nMaxOut,
			&nOutWorkMemSize,
			&nOutProgMemSize);
	if (rc < 0)
		return rc;

	/* Allocate memories */
#define alloc(x, alloc_fn) \
	do { \
		if (n ## x ## Size > pMemBlocks->n ## x ## Size) { \
			if (pMemBlocks->p ## x) \
				free(pMemBlocks->p ## x); \
			pMemBlocks->p ## x = alloc_fn(n ## x ## Size); \
			pMemBlocks->n ## x ## Size = n ## x ## Size; \
		} \
	} while (0)
#define alignoc(v)	aligned_alloc(ALIGN, v)
	alloc(InterWorkMem, malloc);
	alloc(InterProgMem, malloc);
	alloc(InterSymMem, alignoc);
	alloc(OutWorkMem, malloc);
	alloc(OutProgMem, malloc);
#undef alignoc
#undef alloc

	/* Record updated parameters */
#define record(x)	(pMemBlocks->x = x)
	record(nMaxK);
	record(nMaxExtraIn);
	record(nMaxOut);
	record(nMaxT);
#undef record

	return 0;
}

void RqMemBlocksFreeMem(RqMemBlocks* pMemBlocks)
{
#define xfree(x) \
	do { \
		if (pMemBlocks->p ## x) \
			free(pMemBlocks->p ## x); \
	} while (0);
	xfree(InterWorkMem);
	xfree(InterProgMem);
	xfree(InterSymMem);
	xfree(OutWorkMem);
	xfree(OutProgMem);
#undef xfree
}
