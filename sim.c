#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

/*
 *	branch selector table
 */

Predictor predictBranchSelectorTable(BCT* BRCTable, uint32T addr)
{
	uint32T index = indexObtain(addr, BRCTable->attributes.indexLen);
	switch (BRCTable->selector[index])
	{
	case strGshare:
	case wGshare:
		return gshare;
	default:
		return bimodal;
	}
}

void updateBranchCT(BCT* BRCTable, uint32T addr, Result result)
{
	if (result.actTaken == result.predictTaken[BIMODAL] && result.actTaken == result.predictTaken[GSHARE])
		return;
	if (result.actTaken != result.predictTaken[BIMODAL] && result.actTaken != result.predictTaken[GSHARE])
		return;
	uint32T index = indexObtain(addr, BRCTable->attributes.indexLen);
	if (result.actTaken == result.predictTaken[GSHARE])
	{
		switch (BRCTable->selector[index])
		{
		case strBimodal:
		case wBimodal:
		case wGshare:
			BRCTable->selector[index]++;
		default:
			return;
		}
	}
	else
	{
		switch (BRCTable->selector[index])
		{
		case strGshare:
		case wGshare:
		case wBimodal:
			BRCTable->selector[index]--;
		default:
			return;
		}
	}
}

/*
 *	branch selector table end
 */

/*
 *	Branch Predictor
 */

// initial global history register 
void initPredictor(Predictor type, uint32T* width)
{
	branchPredictor->classPredictor = type;
	branchPredictor->predictor = (gshareBranchPredictor*)malloc(sizeof(gshareBranchPredictor));
	if (branchPredictor->predictor == NULL)
		exitError("malloc")
		gshareBranchPredictor* predictor = branchPredictor->predictor;
	predictor->globalHistoryRegister = (GHR*)malloc(sizeof(GHR));
	if (predictor->globalHistoryRegister == NULL)
		exitError("malloc")
		initGBrachHistoryReg(predictor->globalHistoryRegister, width[GHRegister]);
	predictor->branchPredictionTable = (BPT*)malloc(sizeof(BPT));
	if (predictor->branchPredictionTable == NULL)
		exitError("malloc")
		initialBranchPredictionTable(predictor->branchPredictionTable, width[GSHARE]);
	return;
	
}

resultTaken PreBimodal(biModalBranchPredictor *predictor, uint32T addr)
{
	uint32T index = indexObtain(addr, ((BPT *)predictor)->attributes.indexLen);
	return predictBranchPredictionTable((BPT *)predictor, index);
}

resultTaken preGShare(gshareBranchPredictor *predictor, uint32T addr)
{
	uint32T h = predictor->globalHistoryRegister->attributes.hisWidth;
	uint32T i = predictor->branchPredictionTable->attributes.indexLen;
	uint32T index = indexObtain(addr, i);
	uint32T history = predictor->globalHistoryRegister->history;
	uint32T tailIndex = (index << (32 - (i - h))) >> (32 - (i - h));
	uint32T headIndex = (index >> (i - h)) ^ history;
	index = ((headIndex) << (i - h)) | tailIndex;
	return predictBranchPredictionTable(predictor->branchPredictionTable, index);
}

Result pPredictor(uint32T addr)
{
	Result result;
	result.pPredictor = branchPredictor->classPredictor;
	
		result.predictTaken[GSHARE] = preGShare((gshareBranchPredictor *)branchPredictor->predictor, addr);
		return result;
}

void updateBimodal(biModalBranchPredictor *predictor, uint32T addr, Result result)
{
	uint32T index = indexObtain(addr, ((BPT *)predictor)->attributes.indexLen);
	updateBranchPredictionTable(predictor, index, result);
}

void updateGShare(gshareBranchPredictor *predictor, uint32T addr, Result result)
{
	uint32T h = predictor->globalHistoryRegister->attributes.hisWidth;
	uint32T i = predictor->branchPredictionTable->attributes.indexLen;
	uint32T index = indexObtain(addr, i);
	uint32T history = predictor->globalHistoryRegister->history;
	uint32T tailIndex = (index << (32 - (i - h))) >> (32 - (i - h));
	uint32T headIndex = (index >> (i - h)) ^ history;
	index = ((headIndex) << (i - h)) | tailIndex;
	updateBranchPredictionTable(predictor->branchPredictionTable, index, result);
}

void predictorUpdate(uint32T addr, Result result)
{
	gshareBranchPredictor* predictor = branchPredictor->predictor;
	updateGShare(predictor, addr, result);
	GHRUpdate(predictor->globalHistoryRegister, result);
	return;
	
	
}

/*
 *	Branch Predictor end
 */

/*
 *	branch prediction table
 */
void initialBranchPredictionTable(BPT* BranchPredictionTable, uint32T indexLen)
{
	BranchPredictionTable->attributes.indexLen = indexLen;
	BranchPredictionTable->attributes.tokenNum = (ulint64T)pow_2(indexLen);

	BranchPredictionTable->token = (tokenTwoBit *)malloc(sizeof(tokenTwoBit) * BranchPredictionTable->attributes.tokenNum);
	if (BranchPredictionTable->token == NULL)
		exitError("malloc")
	ulint64T i;
	for (i = 0; i < BranchPredictionTable->attributes.tokenNum; i++)
		BranchPredictionTable->token[i] = weaklytaken;
}

resultTaken predictBranchPredictionTable(BPT* BranchPredictionTable, ulint64T index)
{
	switch (BranchPredictionTable->token[index])
	{
	case stronglytaken:
	case weaklytaken:
		return taken;
	default:
		return not_taken;
	}
}

void updateBranchPredictionTable(BPT* BranchPredictionTable, ulint64T index, Result result)
{
	if (result.actTaken == taken)
	{
		switch (BranchPredictionTable->token[index])
		{
		case stronglynottaken:
		case weaklynottaken:
		case weaklytaken:
			BranchPredictionTable->token[index]++;
		default:
			return;
		}
	}
	else
	{
		switch (BranchPredictionTable->token[index])
		{
		case stronglytaken:
		case weaklytaken:
		case weaklynottaken:
			BranchPredictionTable->token[index]--;
		default:
			return;
		}
	}
}


/*
 *	branch prediction table end
 */

/*
* Branch target buffer
*/
void initBranchTargetBuffer(uint32T assoc, uint32T indexLen)
{
	branchTargetBuffer->attributes.assoc = assoc;
	branchTargetBuffer->attributes.numSet = (uint32T)pow_2(indexLen);
	
	branchTargetBuffer->attributes.indexLen = indexLen;
	branchTargetBuffer->attributes.widthTag = 30 - indexLen;

	branchTargetBuffer->set = (Set *)malloc(sizeof(Set) * branchTargetBuffer->attributes.numSet);
	if (branchTargetBuffer->set == NULL)
		exitError("malloc")
	uint32T i;
	for (i = 0; i < branchTargetBuffer->attributes.numSet; i++)
	{
		branchTargetBuffer->set[i].block = (Block *)malloc(sizeof(Block) * branchTargetBuffer->attributes.assoc);
		if (branchTargetBuffer->set[i].block == NULL)
			exitError("malloc")
		memset(branchTargetBuffer->set[i].block, 0, sizeof(Block) * branchTargetBuffer->attributes.assoc);
		branchTargetBuffer->set[i].rank = (ulint64T *)malloc(sizeof(ulint64T) * branchTargetBuffer->attributes.assoc);
		if (branchTargetBuffer->set[i].rank == NULL)
			exitError("malloc")
		memset(branchTargetBuffer->set[i].rank, 0, sizeof(ulint64T) * branchTargetBuffer->attributes.assoc);
	}
}

void interpretAddress(uint32T addr, uint32T *tag, uint32T *index)
{
	uint32T widthTag = branchTargetBuffer->attributes.widthTag;
	*tag = addr >> (32 - widthTag);
	*index = (addr << widthTag) >> (widthTag + 2);
}

uint32T addressRebuild(uint32T tag, uint32T index)
{
	uint32T addr = 0;
	addr |= (tag << (branchTargetBuffer->attributes.indexLen + 2));
	addr |= (index << 2);
	return addr;
}

uint32T searchBranchTargetBuffer(ulint64T tag, uint32T index)
{
	uint32T i, k = branchTargetBuffer->attributes.assoc;
	for (i = 0; i < branchTargetBuffer->attributes.assoc; i++)
		if (branchTargetBuffer->set[index].block[i].bitValid == VALID && branchTargetBuffer->set[index].block[i].tag == tag)
		{
			k = i;
			break;
		}
	return k;
}

void maintainRank(uint32T index, uint32T wayNum, ulint64T valueRank)
{
	branchTargetBuffer->set[index].rank[wayNum] = valueRank;
}

uint32T topRank(uint32T index)
{
	uint32T i, assoc = branchTargetBuffer->attributes.assoc;
	for (i = 0; i < assoc; i++)
		if (branchTargetBuffer->set[index].block[i].bitValid == INVALID)
			return i;
	ulint64T *rank = branchTargetBuffer->set[index].rank;
	uint32T k = 0;
	for (i = 0; i < assoc; i++)
		if (rank[i] < rank[k])
			k = i;
	return k;
}

void replacementBranchTB(uint32T index, uint32T wayNum, uint32T tag)
{
	branchTargetBuffer->set[index].block[wayNum].bitValid = VALID;
	branchTargetBuffer->set[index].block[wayNum].tag = tag;
}

brachResult predictBranchTargetBuffer(uint32T addr)
{
	uint32T tag, index;
	interpretAddress(addr, &tag, &index);
	uint32T wayNum = searchBranchTargetBuffer(tag, index);
	if (wayNum == branchTargetBuffer->attributes.assoc)
		return notBranch;
	return branch;
}
// update according to prediction
void BTBUpdate(uint32T addr, Result result, ulint64T valueRank)
{
	uint32T tag, index, wayNum;
	if (result.actualBranch == result.predictBranch)
	{
		if (result.actualBranch == notBranch)
			return;
		interpretAddress(addr, &tag, &index); // update the LRU bit
		wayNum = searchBranchTargetBuffer(tag, index);
	}
	else
	{
		interpretAddress(addr, &tag, &index);
		wayNum = topRank(index);
		replacementBranchTB(index, wayNum, tag);
	}
	maintainRank(index, wayNum, valueRank);
}

/*
* Branch target buffer end
*/

/*
* System out
*/
void argParse(int argc, char * argv[], Predictor *type, uint32T* width)
{

    
	if (argc != 5)
		printf("\nPlease input ./<fileOutputName> gshare <GPB><RB><traceFile>");
		*type = gshare;
		if (argc != 5)                                              
			printf("\nWrong Input");
		else{
			printf("\nConfiguration: M = %s N = %s",argv[2],argv[3]);
		}

		width[GSHARE] = atoi(argv[2]);
		width[GHRegister] = atoi(argv[3]);
		width[BTBuffer] = 0;
		width[ASSOC] = 0;
		traceFiles = argv[4];               


}

void initalStats()
{
	stat.branchsNums = 0;
	stat.predictionNums = 0;
	stat.rateMisPrediction = 0;
	memset(stat.misPredictionNums, 0, sizeof(ulint64T) * 6);
}

uint32T indexObtain(uint32T addr, uint32T indexLen)
{
	return (addr << (30 - indexLen)) >> (32 - indexLen);
}

void updateStats(Result result)
{
	if (result.actualBranch == BRANCH)
		stat.branchsNums++;
	if (result.predictBranch == notBranch && result.actualBranch == branch && result.actTaken == taken)
		stat.misPredictionNums[BTBuffer]++;
	if (result.predictBranch == notBranch)
	{
		stat.misPredictionNums[5] = stat.misPredictionNums[branchPredictor->classPredictor] + stat.misPredictionNums[BTBuffer];
		stat.rateMisPrediction = (double)stat.misPredictionNums[5] / (double)stat.predictionNums * 100.0;
		return;
	}
	stat.predictionNums++;
	if (result.actTaken != result.predictTaken[branchPredictor->classPredictor])
	{
		stat.misPredictionNums[branchPredictor->classPredictor]++;
		
	}
	stat.misPredictionNums[5] = stat.misPredictionNums[branchPredictor->classPredictor] + stat.misPredictionNums[BTBuffer];
	stat.rateMisPrediction = (double)stat.misPredictionNums[5] / (double)stat.branchsNums * 100.0;
}

void dataOutput(FILE *fp, int argc, char* argv[])
{
	fprintf(fp, "\n");
	fprintf(fp, "Misprediction Ratio: %4.2f percent\n",stat.rateMisPrediction);
}
/*
* System Out End
*/

/*
* Globle branch history register
*/
void initGBrachHistoryReg(GHR *GlobalBranchHistoryRegister, uint32T hisWidth)
{
	GlobalBranchHistoryRegister->attributes.hisWidth = hisWidth;
	if (hisWidth == 0)
		GlobalBranchHistoryRegister->attributes.hisOne = 0;
	else
		GlobalBranchHistoryRegister->attributes.hisOne = (uint32T)pow_2(hisWidth - 1);
	GlobalBranchHistoryRegister->history = 0;
}

void GHRUpdate(GHR *GlobalBranchHistoryRegister, Result result)
{
	uint32T historyDirty = GlobalBranchHistoryRegister->history;
	historyDirty = historyDirty >> 1;
	if (result.actTaken == TAKEN)
		historyDirty = historyDirty | GlobalBranchHistoryRegister->attributes.hisOne;
	GlobalBranchHistoryRegister->history = historyDirty;
}

void GHRprint(GHR *GlobalBranchHistoryRegister, FILE *fp)
{
	fprintf(fp, "0x\t\t%x\n", GlobalBranchHistoryRegister->history);
}
/*
* Globle branch history register end
*/
BTB *branchTargetBuffer;
BP *branchPredictor;
Stat stat;
char *traceFiles;
/*-------------------------- Run Data --------------------------------*/

int main(int argc, char *argv[])
{
	Predictor type;
	uint32T width[9];
	argParse(argc, argv, &type, width);
	branchTargetBuffer = NULL;
	branchPredictor = NULL;
	if (width[BTBuffer] != 0 && width[ASSOC] != 0)
	{
		branchTargetBuffer = (BTB *)malloc(sizeof(BTB));
		if (branchTargetBuffer == NULL)
			exitError("malloc")
		initBranchTargetBuffer(width[ASSOC], width[BTBuffer]);
	}

	branchPredictor = (BP *)malloc(sizeof(BP));
	if (branchPredictor == NULL)
		exitError("malloc")

	initPredictor(type, width);
	initalStats();

	FILE *traceFilesFP = fopen(traceFiles, "r");
	if (traceFilesFP == NULL)
		exitError("fopen")
		
	ulint64T counterTrace = 0;
	/* Parse through data and make predictions */
	while (1)
	{
		cint8T takeorNot, line;
		uint32T addr;
		int rr = fscanf(traceFilesFP, "%x %c%c", &addr, &takeorNot, &line);
		counterTrace++;
		if (rr == EOF)
			break;

		Result result = pPredictor(addr);
		if (branchTargetBuffer == NULL)
			result.predictBranch = branch;
		else
			result.predictBranch = predictBranchTargetBuffer(addr);

		result.actualBranch = branch;
		if (takeorNot == 't')
			result.actTaken = taken;
		else
			result.actTaken = not_taken;
		updateStats(result);
		if (result.predictBranch == branch)
			predictorUpdate(addr, result);
		if (branchTargetBuffer != NULL)
			BTBUpdate(addr, result, counterTrace);

	}
	FILE *fp = stdout;
	dataOutput(fp, argc, argv);
}