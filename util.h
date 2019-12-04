#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BIMODAL 0
#define GSHARE 1
#define BTBuffer 4
#define GHRegister 5
#define BCTable 6
#define BHTable 7
#define ASSOC 8
#define NOTBRANCH 0
#define BRANCH 1
#define NOT_TAKEN 0
#define TAKEN 1
#define VALID 1
#define INVALID 0
#define CORRECT 0
#define WRONG 1
#define exitError(fun) { perror(fun); exit(EXIT_FAILURE); }
#define pow_2(num) (1 << ((int)(num)))
#define STRONGLYGSHARE 3
#define WEAKLYGSHARE 2
#define WEAKLYBIMODAL 1
#define STRONGLYBIMODAL 0
#define STRONGLYTAKEN 3
#define WEAKLYTAKEN 2
#define WEAKLYNOTTAKEN 1
#define STRONGLYNOTTAKEN 0

typedef char cint8T;
typedef unsigned int uint32T;
typedef unsigned long long ulint64T;

typedef enum brachResult
{ 
	notBranch = NOTBRANCH,
	branch = BRANCH 
} brachResult;

typedef enum resultTaken
{ 
	not_taken = NOT_TAKEN,
	taken = TAKEN 
} resultTaken;

typedef enum Predictor
{
	bimodal = BIMODAL,
	gshare = GSHARE,
	
}Predictor;

typedef struct Result
{
	Predictor pPredictor;
	brachResult predictBranch;
	resultTaken predictTaken[4];
	brachResult actualBranch;
	resultTaken actTaken;
}Result;

typedef struct Stat
{
	ulint64T branchsNums;
	ulint64T predictionNums;
	ulint64T misPredictionNums[6];
	double rateMisPrediction;
}Stat;

extern Stat stat;
extern char *traceFiles;


uint32T indexObtain(uint32T addr, uint32T indexLen);


/*
 *	branch chooser table
 */



typedef enum selectorTwoBit
{
	strBimodal = STRONGLYBIMODAL,
	wBimodal = WEAKLYBIMODAL,
	wGshare = WEAKLYGSHARE,
	strGshare = STRONGLYGSHARE
} selectorTwoBit;

typedef struct attributesBranchSelectorTable
{
	uint32T numSelector;
	uint32T indexLen;
}attributesBranchSelectorTable;

typedef struct BCT
{
	selectorTwoBit* selector;
	attributesBranchSelectorTable attributes;
}BCT;

Predictor predictBranchSelectorTable(BCT* BRCTable, uint32T addr);



/*
 *	branch prediction table
 */

typedef enum tokenTwoBit
{
	stronglynottaken = STRONGLYNOTTAKEN,
	weaklynottaken = WEAKLYNOTTAKEN,
	weaklytaken = WEAKLYTAKEN,
	stronglytaken = STRONGLYTAKEN
} tokenTwoBit;

typedef struct attributesBranchPredictionTable
{
	ulint64T tokenNum;
	uint32T indexLen;
}attributesBranchPredictionTable;

typedef struct BPT
{
	tokenTwoBit* token;
	attributesBranchPredictionTable attributes;
}BPT;

void initialBranchPredictionTable(BPT* BranchPredictionTable, uint32T indexLen);

resultTaken predictBranchPredictionTable(BPT* BranchPredictionTable, ulint64T index);


void updateBranchPredictionTable(BPT* BranchPredictionTable, ulint64T index, Result result);



/*
 *	globle branch history register
 */

typedef struct attributesGlobleBranchHistoryReg
{
	uint32T hisWidth;
	uint32T hisOne;
}attributesGlobleBranchHistoryReg;

typedef struct GHR
{
	uint32T history;
	attributesGlobleBranchHistoryReg attributes;
}GHR;


void initGBrachHistoryReg(GHR *GlobalBranchHistoryRegister, uint32T hisWidth);


void GHRUpdate(GHR *GlobalBranchHistoryRegister, Result result);


void GHRprint(GHR *GlobalBranchHistoryRegister, FILE *fp);


/*
 *	Branch Predictor
 */

typedef BPT biModalBranchPredictor;

typedef struct gshareBranchPredictor
{
	GHR* globalHistoryRegister;
	BPT* branchPredictionTable;
}gshareBranchPredictor;


typedef struct BP
{
	Predictor classPredictor;
	void *predictor;
}BP;

extern BP* branchPredictor;


resultTaken PreBimodal(biModalBranchPredictor *predictor, uint32T addr);

resultTaken preGShare(gshareBranchPredictor *predictor, uint32T addr);

Result pPredictor(uint32T addr);


/*
 *	Branch Target Buffer
 */

typedef struct Block
{
	uint32T tag;
	cint8T bitValid;
}Block;

typedef struct Set
{
	Block *block;
	ulint64T *rank;
}Set;

typedef struct attributesBranchTargetBuffer
{
	uint32T assoc;
	uint32T numSet;
	uint32T widthTag;
	uint32T indexLen;
}attributesBranchTargetBuffer;

typedef struct BTB
{
	Set *set;
	attributesBranchTargetBuffer attributes;
}BTB;

extern BTB* branchTargetBuffer;

uint32T addressRebuild(uint32T tag, uint32T index);

uint32T searchBranchTargetBuffer(ulint64T tag, uint32T index);

uint32T topRank(uint32T index);

brachResult predictBranchTargetBuffer(uint32T addr);

