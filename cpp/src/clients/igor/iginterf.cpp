
#ifndef __IGINTERF_H
#include "iginterf.h"
#endif

#include <sstream>
//using namespace std;

//*************************************************************************

char srTIgorInterf::gProgressIndicatorWinName[256];
long srTIgorInterf::gProgressIndicatorScale = 1000000;
long srTIgorInterf::gRadIndices[MAX_DIMENSIONS+1];

//*************************************************************************

int srTIgorInterf::GetStringFromTextWave1D(waveHndl wavH, int IndNo, string& sValue)
{
	long RadIndices[MAX_DIMENSIONS];
	RadIndices[0] = IndNo;
	Handle textH = NewHandle(0L);
	int result;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	sValue.append(*textH);
	DisposeHandle(textH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetDoubleFromTextWave1D(waveHndl wavH, int IndNo, double& Value)
{
	long RadIndices[MAX_DIMENSIONS];
	RadIndices[0] = IndNo;
	Handle textH = NewHandle(0L);
	int result;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	Value = atof(*textH);
	if((Value == 0.) && (**textH != '0') && (**textH != ' ')) 
	{
		DisposeHandle(textH);
		return ERROR_IN_READING_NUMBER_FROM_TEXT_WAVE;
	}
	DisposeHandle(textH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::UpdateTextWave1D(waveHndl wavH, vector<string>* pStringVect)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result;
	//long numDimensions;
	int numDimensions; //OC080613 (move to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	if(numDimensions > 1) return TEXT_WAVE_1D_REQUIRED;

	long RadIndices[MAX_DIMENSIONS+1];
	int AmOfNewRecords = (int)pStringVect->size();
	int AmOfOldRecords = dimensionSizes[0];

	if(AmOfNewRecords > AmOfOldRecords) 
	{
		if(result = ChangeWave(wavH, AmOfNewRecords, TEXT_WAVE_TYPE)) return result;
	}

	int AmOfRecords = (AmOfOldRecords > AmOfNewRecords)? AmOfOldRecords : AmOfNewRecords;
	char EmptyStr[] = " ";
	for(int k=0; k<AmOfRecords; k++)
	{
		const char *pStr;
		if(k < AmOfNewRecords) pStr = ((*pStringVect)[k]).c_str();
		else pStr = EmptyStr;

		int AmOfBytes = (int)strlen(pStr);
		Handle textH = NewHandle(AmOfBytes);
		strncpy(*textH, pStr, AmOfBytes);
		*RadIndices = k;
		if(result = MDSetTextWavePointValue(wavH, RadIndices, textH)) return result;
		DisposeHandle(textH);
	}
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::UpdateTextWaveMD(waveHndl wavH, char** arStrFlat, int* arNp, int numDim)
{
	for(int j=0; j<=MAX_DIMENSIONS; j++) gRadIndices[j] = 0;
	return UpdateTextWaveMD_NestedFor(wavH, arStrFlat, arNp, numDim);
}

//*************************************************************************

int srTIgorInterf::UpdateTextWaveMD_NestedFor(waveHndl wavH, char** arStrFlat, int* arNp, int numDim)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	if(numDim > MAX_DIMENSIONS) numDim = MAX_DIMENSIONS;

	int result;
	//long numDimensions;
	int numDimensions; //OC080613 (move to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	//if(numDimensions < numDim) return TOO_SHORT_WAVE; //to use another message

	if((numDimensions < numDim) || (dimensionSizes[numDim - 1] < arNp[numDim - 1])) 
	{
		for(int i=0; i<MAX_DIMENSIONS; i++) 
		{
			dimensionSizes[i] = (i < numDim)? arNp[i] : 0;
		}
		if(result = MDChangeWave(wavH, -1, dimensionSizes)) return result;
		numDimensions = numDim;
	}
	//for(int j=(numDim - 1); j<numDimensions; j++) gRadIndices[j] = 0;
	//for(int j=numDim; j<=MAX_DIMENSIONS; j++) gRadIndices[j] = 0;

	long per = 1;
	for(int k=0; k<(numDim - 1); k++) per *= arNp[k];

	char **t_arStrFlat = arStrFlat;
	for(int i=0; i<arNp[numDim - 1]; i++)
	{
		gRadIndices[numDim - 1] = i;

		if(numDim == 1) 
		{
			int AmOfBytes = 0;
			char *t_arStrFlat_i = arStrFlat[i];
			
			if(t_arStrFlat_i != 0)
			{
				AmOfBytes = (int)strlen(t_arStrFlat_i);
				Handle textH = NewHandle(AmOfBytes);
				strncpy(*textH, t_arStrFlat_i, AmOfBytes);
				//*gRadIndices = i;
				if(result = MDSetTextWavePointValue(wavH, gRadIndices, textH)) return result;
				DisposeHandle(textH);
			}
		}
		else
		{//call itself
			UpdateTextWaveMD_NestedFor(wavH, t_arStrFlat, arNp, numDim - 1);
			t_arStrFlat += per;
		}
	}
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

void srTIgorInterf::Message(const char* Text)
{
	if(Text == 0) return;
	ostringstream DecoratedStream;
	//DecoratedStream << "\r";
	DecoratedStream << Text << "\r" << ends;
	basic_string<char> BufDecoratedWarnString = DecoratedStream.str();
	const char* DecoratedWarnString = BufDecoratedWarnString.c_str();

	XOPNotice(DecoratedWarnString);
}

//*************************************************************************

void srTIgorInterf::WarningMessage(const char* WarnText)
{
	if(WarnText == 0) return;
	ostringstream DecoratedStream;
	DecoratedStream << "\rW A R N I N G\r";
	DecoratedStream << "- " << WarnText << "\r";
	DecoratedStream << "\r" << ends;
	basic_string<char> BufDecoratedWarnString = DecoratedStream.str();
	const char* DecoratedWarnString = BufDecoratedWarnString.c_str();

	XOPNotice(DecoratedWarnString);
}

//*************************************************************************

void srTIgorInterf::ErrorMessage(const char* WarnText)
{
	if(WarnText == 0) return;
	ostringstream DecoratedStream;
	DecoratedStream << "\rE R R O R\r";
	DecoratedStream << "- " << WarnText << "\r";
	DecoratedStream << "\r" << ends;
	basic_string<char> BufDecoratedWarnString = DecoratedStream.str();
	const char* DecoratedWarnString = BufDecoratedWarnString.c_str();

	XOPNotice(DecoratedWarnString);
}

//*************************************************************************

int srTIgorInterf::ShowCompProgress(double CurNormVal)
{
	int result = 0;
	if((CurNormVal < 0.) || (CurNormVal > 1.)) return 0;
	if(CurNormVal == 0.) return InitCompProgressIndicator(); //start indicating
	else if(CurNormVal < 1.) return UpdateCompProgressIndicator(CurNormVal); //continue indicating
	else 
	{
		UpdateCompProgressIndicator(1.);
		DestroyCompProgressIndicator(); //stop indicating
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::InitCompProgressIndicator()
{
	int result = 0;
	if(result = SetIgorIntVar("IgorTotAmOfPo", gProgressIndicatorScale, 0)) return result;
	if(result = XOPCommand("NewPanel/W=(3.75,43.25,163,85)/K=2 as \" \"")) return result;
	if(result = XOPCommand("String IgorIndicatorPanelWinName = WinName(0,64)")) return result;
	if(result = FetchStrVar("IgorIndicatorPanelWinName", gProgressIndicatorWinName)) return result;
	if(result = XOPCommand("SetDrawEnv fname= \"Helvetica\",fsize= 12; DrawText 15,18,\"Computation Progress\"")) return result;
	if(result = XOPCommand("ValDisplay srIgorCompProgressBar,pos={15,23},size={130,10},font=\"Helvetica\",limits={0,IgorTotAmOfPo,0},barmisc={30,0},value= #\"0\"")) return result;
	return result;
}

//*************************************************************************

void srTIgorInterf::DestroyCompProgressIndicator()
{
	char TotOutStr[300];
	strcpy(TotOutStr, "DoWindow/K ");
	strcat(TotOutStr, gProgressIndicatorWinName);
	XOPCommand(TotOutStr);
}

//*************************************************************************

int srTIgorInterf::UpdateCompProgressIndicator(double CurNormVal)
{
	int result = 0;
	char TotOutStr[200];
	char PtNoStr[12];
	long CurPoint = (long)(CurNormVal*((double)gProgressIndicatorScale));
	sprintf(PtNoStr, "%d\n", CurPoint);
	strcpy(TotOutStr, "ValDisplay srIgorCompProgressBar value= #\"");
	strcat(TotOutStr, PtNoStr);
	strcat(TotOutStr, "\"");
	strcat(TotOutStr, ", win=");
	strcat(TotOutStr, gProgressIndicatorWinName);
	if(result = XOPCommand(TotOutStr)) return result;
	return result;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleFromNumWave1D(waveHndl wavH, long MaxNp, double*& pData, long& Np)
{//if(pData == 0) this function allocates pData
 //else assumes already allocated pData
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (move to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	Np = 0;
	if((MaxNp > 0) && (MaxNp < numRows)) Np = MaxNp;
	else Np = numRows;
	if(pData == 0) pData = new double[Np];
    double *tData = pData;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	
	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = *(fp++);
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = *(dp++);
	}
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleFromCmplxWave1D(waveHndl wavH, long MaxNp, double*& pData, long& Np)
{//if(pData == 0) this function allocates pData
 //else assumes already allocated pData

	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != (NT_FP64 | NT_CMPLX)) && (waveType != (NT_FP32 | NT_CMPLX))) return NT_CMPLX_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result = 0;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	Np = 0;
	if((MaxNp > 0) && (MaxNp < numRows)) Np = MaxNp;
	else Np = numRows;
	if(pData == 0) pData = new double[Np << 1];
    double *tData = pData;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	if(waveType == (NT_FP32 | NT_CMPLX))
	{
        float* fp = (float*)dataStartPtr;
		for(long i=0; i<Np; i++) { *(tData++) = *(fp++); *(tData++) = *(fp++);}
	}
	else if(waveType == (NT_FP64 | NT_CMPLX))
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
		for(long i=0; i<Np; i++) { *(tData++) = *(dp++); *(tData++) = *(dp++);}
	}

	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetSizeNumWave1D(waveHndl wavH, long& size)
{
	if(wavH == NIL) return NOWAV;
	//int waveType = WaveType(wavH);
	//if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	size = dimensionSizes[0];
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetDimSizesWaveMD(waveHndl wavH, long* arDimSizes, long& numDims)
{//assumes arDims[4] allocated outside
	if(wavH == NIL) return NOWAV;
	if(arDimSizes == 0) return 0;

	//long numDimensions;
	//long dimensionSizes[MAX_DIMENSIONS+1];
	int result;
	int locNumDims; //OC080613 (moving to XOPSupport 6)
	//if(result = MDGetWaveDimensions(wavH, &numDims, arDimSizes)) return result;
	if(result = MDGetWaveDimensions(wavH, &locNumDims, arDimSizes)) return result; //OC080613 (moving to XOPSupport 6)
	numDims = locNumDims; //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrIntFromNumWave1D(waveHndl wavH, long MaxNp, int*& pData, long& Np)
{//if(pData == 0) this function allocates pData
 //else assumes already allocated pData
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	if((MaxNp > 0) && (MaxNp < numRows)) Np = MaxNp;
	else Np = numRows;
	if(pData == 0) pData = new int[Np];
    int *tData = pData;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	
	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = (int)(*(fp++));
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = (int)(*(dp++));
	}
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::Get4ElemArrDoubleFromNumWave1D(waveHndl wavH, double* pData)
{
	int res=0;
	long Np=0;
	if(res = GetArrDoubleFromNumWave1D(wavH, 4, pData, Np)) return res;
	if((pData == 0) || (Np != 4)) return THREE_ELEM_NUM_WAVE_REQUIRED;
	return 0;
}

//*************************************************************************

int srTIgorInterf::Get3ElemArrDoubleFromNumWave1D(waveHndl wavH, double* pData)
{
	int res=0;
	long Np=0;
	if(res = GetArrDoubleFromNumWave1D(wavH, 3, pData, Np)) return res;
	if((pData == 0) || (Np != 3)) return THREE_ELEM_NUM_WAVE_REQUIRED;
	return 0;
}

//*************************************************************************

int srTIgorInterf::Get2ElemArrDoubleFromNumWave1D(waveHndl wavH, double* pData)
{
	int res=0;
	long Np=0;
	if(res = GetArrDoubleFromNumWave1D(wavH, 2, pData, Np)) return res;
	if((pData == 0) || (Np != 2)) return THREE_ELEM_NUM_WAVE_REQUIRED;
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetDataPtrFromWaveDouble(waveHndl wavH, double*& pData, int& hState)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	hState = 0; //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

    pData = (DOUBLE*)dataStartPtr;
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetDataPtrFromWaveDoubleOrFloat1D(waveHndl wavH, double*& pdData, float*& pfData, long& LenData, int& hState)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//hState = MoveLockHandle(wavH);
	hState = 0; //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	LenData = numRows;
	if(waveType == NT_FP32)
	{
		pdData = 0;
		pfData = (float*)dataStartPtr;
	}
	else if(waveType == NT_FP64)
	{
		pdData = (double*)dataStartPtr;
		pfData = 0;
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetDataPtrFromWaveDoubleOrFloat(waveHndl wavH, double*& pdData, float*& pfData, int& hState)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	//long dimensionSizes[MAX_DIMENSIONS+1]; //OC080613 (moving to XOPSupport 6)
	int result;

	//if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	//long numRows = dimensionSizes[0];
	//if(numRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//hState = MoveLockHandle(wavH);
	hState = 0; //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	//LenData = numRows;
	if(waveType == NT_FP32)
	{
		pdData = 0;
		pfData = (float*)dataStartPtr;
	}
	else if(waveType == NT_FP64)
	{
		pdData = (double*)dataStartPtr;
		pfData = 0;
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::ReleaseWave(waveHndl wavH, int hState)
{
	//if(wavH == NIL) return NOWAV;
	if(wavH == NIL) return 0;
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::ExtractCStrFromHandle(Handle hStr, int MaxLenStr, char* StrID)
{
	if((hStr == 0) || (StrID == 0)) return 0;

    int LenStr = GetHandleSize(hStr);
    if(LenStr > MaxLenStr) LenStr = MaxLenStr;
    strncpy(StrID, *hStr, LenStr);
    StrID[LenStr] = '\0';
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleFromNumWave2D(waveHndl wavH, int& NumRows, int& NumCols, double*& pData)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	//if(numDimensions != 2) return NEEDS_2D_WAVE;

	long nRows = dimensionSizes[0];
	//if(nRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	long nCols = dimensionSizes[1];
	//if(nCols <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	if(nCols <= 0) nCols = 1;

	if(NumRows > 0)
	{
        if(nRows < NumRows) return TOO_SHORT_WAVE;
	}
	else NumRows = nRows;

	if(NumCols > 0)
	{
        if(nCols < NumCols) return TOO_SHORT_WAVE;
	}
	else NumCols = nCols;

	long Np = NumRows*NumCols;
	if(Np <= 0) return 0; //allow empty waves
	
	if(pData == 0) pData = new double[Np];
    double *tData = pData;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = (double)(*(fp++));
		//long OffsetStart = 0;
		//for(int i=0; i<NumRows; i++)
		//{
		//	float* fpLoc = fp + OffsetStart;
		//	for(int j=0; j<NumCols; j++) { *(tData++) = (double)(*fpLoc); fpLoc += NumRows;}
		//	OffsetStart++;
		//}
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = (double)(*(dp++));
		//long OffsetStart = 0;
		//for(int i=0; i<NumRows; i++)
		//{
		//	DOUBLE* fpLoc = fp + OffsetStart;
		//	for(int j=0; j<NumCols; j++) { *(tData++) = (double)(*fpLoc); fpLoc += NumRows;}
		//	OffsetStart++;
		//}
	}

	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleFromNumWave3D(waveHndl wavH, int& NumRows, int& NumCols, int& NumChanks, double*& pData)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	//if(numDimensions != 3) return NEEDS_3D_WAVE;

	long nRows = dimensionSizes[0];
	//if(nRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	long nCols = dimensionSizes[1];
	//if(nCols <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	if(nCols <= 0) nCols = 1;
	long nChanks = dimensionSizes[2];
	if(nChanks <= 0) nChanks = 1;

	if(NumRows > 0)
	{
        if(nRows < NumRows) return TOO_SHORT_WAVE;
	}
	else NumRows = nRows;

	if(NumCols > 0)
	{
        if(nCols < NumCols) return TOO_SHORT_WAVE;
	}
	else NumCols = nCols;

	if(NumChanks > 0)
	{
        if(nChanks < NumChanks) return TOO_SHORT_WAVE;
	}
	else NumChanks = nChanks;

	long Np = NumRows*NumCols*NumChanks;
	if(Np <= 0) return 0; //allow empty waves
	
	if(pData == 0) pData = new double[Np];
    double *tData = pData;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = (double)(*(fp++));
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
		for(long i=0; i<Np; i++) *(tData++) = (double)(*(dp++));
	}

	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleScaledFromNumWave2D(waveHndl wavH, int& NumRows, int& NumCols, double*& pData, double* ArgStart, double* ArgStep)
{
	int res = GetArrDoubleFromNumWave2D(wavH, NumRows, NumCols, pData);
	if(res != 0) return res;

	DOUBLE LocAuxStep, LocAuxStart;
	if(res = MDGetWaveScaling(wavH, 0, &LocAuxStep, &LocAuxStart)) return res;
	ArgStep[0] = LocAuxStep; ArgStart[0] = LocAuxStart;

	if(res = MDGetWaveScaling(wavH, 1, &LocAuxStep, &LocAuxStart)) return res;
	ArgStep[1] = LocAuxStep; ArgStart[1] = LocAuxStart;
	return res;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleScaledFromNumWave3D(waveHndl wavH, int& NumRows, int& NumCols, int& NumChanks, double*& pData, double* ArgStart, double* ArgStep)
{
	int res = GetArrDoubleFromNumWave3D(wavH, NumRows, NumCols, NumChanks, pData);
	if(res != 0) return res;

	DOUBLE LocAuxStep, LocAuxStart;
	if(res = MDGetWaveScaling(wavH, 0, &LocAuxStep, &LocAuxStart)) return res;
	ArgStep[0] = LocAuxStep; ArgStart[0] = LocAuxStart;

	if(res = MDGetWaveScaling(wavH, 1, &LocAuxStep, &LocAuxStart)) return res;
	ArgStep[1] = LocAuxStep; ArgStart[1] = LocAuxStart;

	if(res = MDGetWaveScaling(wavH, 2, &LocAuxStep, &LocAuxStart)) return res;
	ArgStep[2] = LocAuxStep; ArgStart[2] = LocAuxStart;
	return res;
}

//*************************************************************************

int srTIgorInterf::GetTotNumPointsInWave4D(waveHndl wavH, long& totNumPt)
{//allocates if pData == 0
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	int res=0;

	if(res = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return res;
	if(numDimensions > 4) numDimensions = 4;
	
	int arNumPt[4];
	for(int j=0; j<4; j++) arNumPt[j] = 1;

	totNumPt = 1;
	for(int k=0; k<numDimensions; k++) 
	{
		arNumPt[k] = dimensionSizes[k];
		totNumPt *= arNumPt[k];
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrFloatScaledFromNumWave4D(waveHndl wavH, float*& pData, int* arNumPt, double* arArgStart, double* arArgStep, char* arStrUnits[], bool* pDataIsNotZero)
{//allocates if pData == 0
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	int res=0;

	if(res = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return res;
	if(numDimensions > 4) numDimensions = 4;
	
	for(int j=0; j<4; j++) arNumPt[j] = 1;

	if(arStrUnits != 0)
	{
		for(int jj=0; jj<5; jj++) *(arStrUnits[jj]) = '\0';
	}
	
	long totNumPt = 1;
	for(int k=0; k<numDimensions; k++) 
	{
		arNumPt[k] = dimensionSizes[k];
		totNumPt *= arNumPt[k];
		
		DOUBLE LocAuxStep, LocAuxStart;
		if(res = MDGetWaveScaling(wavH, k, &LocAuxStep, &LocAuxStart)) return res;
		arArgStart[k] = LocAuxStart; arArgStep[k] = LocAuxStep;
		
		if(arStrUnits != 0)
		{
			if(res = MDGetWaveUnits(wavH, k, arStrUnits[k])) return res;
		}
	}
	if(arStrUnits != 0) //OC22012010
	{
		if(res = MDGetWaveUnits(wavH, -1, arStrUnits[4])) return res; //Data units; attention allocation: numDimensions + 1
	}
	
	if(pData == 0) pData = new float[totNumPt];
    float *tData = pData;

	long dataOffset;
	if(res = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return res;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	bool dataIsNotZero = false;
	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
		for(long i=0; i<totNumPt; i++) 
		{
			if((*fp) != ((float)0.)) dataIsNotZero = true;
			*(tData++) = *(fp++);
		}
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
		for(long i=0; i<totNumPt; i++) 
		{
			if((*dp) != ((DOUBLE)0.)) dataIsNotZero = true;
			*(tData++) = (float)(*(dp++));
		}
	}
	
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleFromNumWaveN3(waveHndl wavH, double*& pData, int& Np)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;

	if(numDimensions == 0) return NEEDS_1D_OR_2D_WAVE;
	
	long numRows = dimensionSizes[0];
	if(numRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
    if(numRows < 3) return THREE_ELEM_NUM_WAVE_REQUIRED;
	
	if(numDimensions == 1)
	{
		pData = new double[3];
		Np = 1;
		return Get3ElemArrDoubleFromNumWave1D(wavH, pData);
	}

	long numCols = dimensionSizes[1];
	if(numCols <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	if(numDimensions >= 2)
	{
		int MaxRows = 3, MaxCols = -1;
		if(result = GetArrDoubleFromNumWave2D(wavH, MaxRows, MaxCols, pData)) return result;
		Np = MaxCols;
		return result;
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleScaledAndNpFromNumWave1D(waveHndl wavH, long MaxNp, double*& pData, long& Np, double& ArgStart, double& ArgStep)
{
	if(wavH == NIL) return NOWAV;

	int res = 0;
	if(res = GetArrDoubleFromNumWave1D(wavH, MaxNp, pData, Np)) return res;

	DOUBLE AuxStep, AuxStart;
	if(res = MDGetWaveScaling(wavH, 0, &AuxStep, &AuxStart)) return res;
	ArgStart = (double)AuxStart;
	ArgStep = (double)AuxStep;
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrDoubleAndNpFromNumWave1D(waveHndl wavH, int DeclarNp, int NumCoordPerPoint, double*& pData, int& Np)
{
	if(wavH == NIL) return NOWAV;

	int res = 0;
    long AllocNumCoord=0;
    if(res = GetArrDoubleFromNumWave1D(wavH, -1, pData, AllocNumCoord)) return res;
    int AllocNp = (int)(((double)AllocNumCoord)/((double)NumCoordPerPoint) + 0.00001);
	Np = (AllocNp < DeclarNp)? AllocNp : DeclarNp;
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetWaveData(waveHndl wavH, TDataWaveMD* pWaveData, int& hState)
{
	if(pWaveData == 0) return 0;
	if(wavH == NIL) return NOWAV;

	int waveType = WaveType(wavH);
	if(waveType == NT_FP32)
	{
		*(pWaveData->DataType) = 'f';
		*(pWaveData->DataType + 1) = '\0';
	}
	else if(waveType == (NT_FP32 | NT_CMPLX))
	{
		*(pWaveData->DataType) = 'c';
		*(pWaveData->DataType + 1) = 'f';
	}
	else if(waveType == NT_FP64)
	{
		*(pWaveData->DataType) = 'd';
		*(pWaveData->DataType + 1) = '\0';
	}
	else if(waveType == (NT_FP64 | NT_CMPLX))
	{
		*(pWaveData->DataType) = 'c';
		*(pWaveData->DataType + 1) = 'd';
	}
	else return NUMERIC_WAVE_REQUIRED;

	int result;
	//if(result = MDGetWaveDimensions(wavH, &(pWaveData->AmOfDims), pWaveData->DimSizes)) return result;
	int locAmOfDims; //OC080613 (moving to XOPSupport 6)
	if(result = MDGetWaveDimensions(wavH, &locAmOfDims, pWaveData->DimSizes)) return result;
	pWaveData->AmOfDims = locAmOfDims; //OC080613 (moving to XOPSupport 6)

	DOUBLE Step, Start;
	char Units[MAX_UNIT_CHARS + 1];

	for(long iDim=0; iDim<pWaveData->AmOfDims; iDim++)
	{
		if(result = MDGetWaveScaling(wavH, iDim, &Step, &Start)) return result;
		pWaveData->DimSteps[iDim] = Step; 
		pWaveData->DimStartValues[iDim] = Start; 

		if(result = MDGetWaveUnits(wavH, iDim, Units)) return result;
		strcpy(pWaveData->DimUnits[iDim], Units);
	}

	if(result = MDGetWaveUnits(wavH, -1, Units)) return result;
	strcpy(pWaveData->DataUnits, Units);

	WaveName(wavH, pWaveData->DataName);

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;

	//hState = MoveLockHandle(wavH);
	hState = 0; //OC080613 (moving to XOPSupport 6)
	pWaveData->pData = (char*)(*wavH) + dataOffset;

	return 0;
}

//*************************************************************************

int srTIgorInterf::UpdateScalingAndFinishWorkingWithWave(TDataWaveMD* pWaveData, waveHndl wavH, int hState)
{
	int result;
	if(pWaveData == 0) return 0;
	if(pWaveData->pData == 0) return 0;

	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)

	if(pWaveData->AmOfDims > -1)
	{
		DOUBLE xStart = pWaveData->DimStartValues[0];
		DOUBLE xStep = pWaveData->DimSteps[0];
		if((xStart < 1.E+23) && (xStep < 1.E+23))
			if(result = MDSetWaveScaling(wavH, ROWS, &xStep, &xStart)) return result;
		if(*(pWaveData->DimUnits[0]) != '\0')
			if(result = MDSetWaveUnits(wavH, ROWS, pWaveData->DimUnits[0])) return result;

		if(pWaveData->AmOfDims > 1)
		{
			DOUBLE yStart = pWaveData->DimStartValues[1];
			DOUBLE yStep = pWaveData->DimSteps[1];
			if((yStart < 1.E+23) && (yStep < 1.E+23))
				if(result = MDSetWaveScaling(wavH, COLUMNS, &yStep, &yStart)) return result;
			if(*(pWaveData->DimUnits[1]) != '\0')
				if(result = MDSetWaveUnits(wavH, COLUMNS, pWaveData->DimUnits[1])) return result;
		}
		if(pWaveData->AmOfDims > 2)
		{
			DOUBLE zStart = pWaveData->DimStartValues[2];
			DOUBLE zStep = pWaveData->DimSteps[2];
			if((zStart < 1.E+23) && (zStep < 1.E+23))
				if(result = MDSetWaveScaling(wavH, LAYERS, &zStep, &zStart)) return result;
			if(*(pWaveData->DimUnits[2]) != '\0')
				if(result = MDSetWaveUnits(wavH, LAYERS, pWaveData->DimUnits[2])) return result;
		}
		if(pWaveData->AmOfDims > 3)
		{
			DOUBLE tStart = pWaveData->DimStartValues[3];
			DOUBLE tStep = pWaveData->DimSteps[3];
			if((tStart < 1.E+23) && (tStep < 1.E+23))
				if(result = MDSetWaveScaling(wavH, CHUNKS, &tStep, &tStart)) return result;
			if(*(pWaveData->DimUnits[3]) != '\0')
				if(result = MDSetWaveUnits(wavH, CHUNKS, pWaveData->DimUnits[3])) return result;
		}
	}

	if(*(pWaveData->DataUnits) != '\0') 
		if(result = MDSetWaveUnits(wavH, -1, pWaveData->DataUnits)) return result;

	WaveHandleModified(wavH);
	//pWaveData->pData = 0;
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetNumWavePointValue1D(waveHndl wavH, int ind, DOUBLE val)
{
    long Indices[MAX_DIMENSIONS]; Indices[0] = ind;
	DOUBLE Value[] = {val, 0};
    return MDSetNumericWavePointValue(wavH, Indices, Value);
}

//*************************************************************************

int srTIgorInterf::ReDimNumWave1D(waveHndl wavH, int NumRows)
{
	if(wavH == NIL) return NOWAV;
	if(NumRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	int res = 0;
	long dimensionSizes[MAX_DIMENSIONS + 1];
	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)

	if(res = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return res;

	//if(numDimensions == 0) return NEEDS_2D_WAVE;
	long nRows = dimensionSizes[0];
	//long nCols = dimensionSizes[1];

	if(NumRows == nRows) return 0;

	for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	dimensionSizes[0] = NumRows;
	//dimensionSizes[1] = NumCols;

	if(res = MDChangeWave(wavH, -1, dimensionSizes)) return res;
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::ReDimWave3D(waveHndl wavH, int NumRows, int NumCols, int NumChunks)
{
	if(wavH == NIL) return NOWAV;

	int res = 0;
	long dimensionSizes[MAX_DIMENSIONS + 1];
	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	
	if(res = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return res;

	//if(numDimensions == 0) return NEEDS_3D_WAVE;
	long nRows = dimensionSizes[0];
	long nCols = dimensionSizes[1];
	long nChunks = dimensionSizes[2];

	if((NumRows == nRows) && (NumCols == nCols) && (NumChunks == nChunks)) return 0;
	for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	dimensionSizes[0] = NumRows;
	dimensionSizes[1] = NumCols;
	dimensionSizes[2] = NumChunks;
	if(res = MDChangeWave(wavH, -1, dimensionSizes)) return res;

	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::ReDimNumWave2D(waveHndl wavH, int NumRows, int NumCols)
{
	if(wavH == NIL) return NOWAV;
	//if(NumRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	int res = 0;
	long dimensionSizes[MAX_DIMENSIONS + 1];
	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)

	if(res = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return res;

	if(numDimensions == 0) return NEEDS_2D_WAVE;
	long nRows = dimensionSizes[0];
	long nCols = dimensionSizes[1];

	if((NumRows == nRows) && (NumCols == nCols)) return 0;

	for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	dimensionSizes[0] = NumRows;
	dimensionSizes[1] = NumCols;

	if(res = MDChangeWave(wavH, -1, dimensionSizes)) return res;
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::ReDimNumWave(waveHndl wavH, int* dimSizes, int numDims, int newType)
{//newType can be NT_FP32, NT_FP64, ... (Igor constants)
 //newType = -1 means no change in data type
	if(wavH == NIL) return NOWAV;
	if(dimSizes == 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	if(numDims <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	int res = 0;
	long dimensionSizes[MAX_DIMENSIONS + 1];
	//long numDimensions; //OC080613 (moving to XOPSupport 6)

	for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	for(int j=0; j<numDims; j++) dimensionSizes[j] = dimSizes[j];

	//char AuxRadWaveName[MAX_OBJ_NAME+1];
	//WaveName(wavH, AuxRadWaveName);
	//if(res = KillWave(wavH)) return res;
	//if(res = MDMakeWave(&wavH, AuxRadWaveName, NIL, dimensionSizes, newType, 1)) return res;

    if(res = MDChangeWave(wavH, newType, dimensionSizes)) return res;

	WaveHandleModified(wavH);
	return 0;
}

int srTIgorInterf::ReDimNumWave(waveHndl wavH, long* dimSizes, long numDims, int newType)
{//newType can be NT_FP32, NT_FP64, ... (Igor constants)
	if(wavH == NIL) return NOWAV;
	if(dimSizes == 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	if(numDims <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	int res = 0;
	long dimensionSizes[MAX_DIMENSIONS + 1];
	//long numDimensions; //OC080613 (moving to XOPSupport 6)

	for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	for(int j=0; j<numDims; j++) dimensionSizes[j] = dimSizes[j];
	
	char AuxRadWaveName[MAX_OBJ_NAME+1];
	WaveName(wavH, AuxRadWaveName);
	if(res = KillWave(wavH)) return res;
	if(res = MDMakeWave(&wavH, AuxRadWaveName, NIL, dimensionSizes, newType, 1)) return res;

	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetDataInNumWave(waveHndl wavH, double* pData, long Np, char* DataUnitStr)
{
	if((pData == 0) || (Np == 0)) return 0;

	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	int result;
	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	double* tData = pData;
	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
        for(long i=0; i<Np; i++) *(fp++) = (float)(*(tData++));
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
        for(long i=0; i<Np; i++) *(dp++) = (DOUBLE)(*(tData++));
	}

	if(DataUnitStr != 0)
	{
		if(result = MDSetWaveUnits(wavH, -1, DataUnitStr)) return result;
	}

	WaveHandleModified(wavH);
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetDataInNumWave(waveHndl wavH, float* pData, long Np, char* DataUnitStr)
{
	if((pData == 0) || (Np == 0)) return 0;

	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	int result;
	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	float* tData = pData;
	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
        for(long i=0; i<Np; i++) *(fp++) = (float)(*(tData++));
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
        for(long i=0; i<Np; i++) *(dp++) = (DOUBLE)(*(tData++));
	}

	if(DataUnitStr != 0)
	{
		if(result = MDSetWaveUnits(wavH, -1, DataUnitStr)) return result;
	}

	WaveHandleModified(wavH);
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetDataInNumWave(waveHndl wavH, int* pData, long Np, char* DataUnitStr)
{
	if((pData == 0) || (Np == 0)) return 0;

	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	int result;
	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	char* dataStartPtr = (char*)(*wavH) + dataOffset;

	int* tData = pData;
	if(waveType == NT_FP32)
	{
        float* fp = (float*)dataStartPtr;
        for(long i=0; i<Np; i++) *(fp++) = (float)(*(tData++));
	}
	else if(waveType == NT_FP64)
	{
        DOUBLE* dp = (DOUBLE*)dataStartPtr;
        for(long i=0; i<Np; i++) *(dp++) = (DOUBLE)(*(tData++));
	}

	if(DataUnitStr != 0)
	{
		if(result = MDSetWaveUnits(wavH, -1, DataUnitStr)) return result;
	}

	WaveHandleModified(wavH);
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetScaleNumWave(waveHndl wavH, int nDim, double* ArgStarts, double* ArgSteps, char** ArgValUnits)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if((waveType != NT_FP64) && (waveType != NT_FP32)) return NT_FP32_OR_NT_FP64_WAVE_REQUIRED;

	//int hState = MoveLockHandle(wavH); //OC080613 (moving to XOPSupport 6)
	int result = 0;

	if((ArgStarts != 0) && (ArgSteps != 0))
	{
		for(int i=0; i<nDim; i++)
		{
            if(result = MDSetWaveScaling(wavH, i, ArgSteps + i, ArgStarts + i)) return result;
		}
	}
	if(ArgValUnits != 0)
	{
		for(int i=0; i<nDim; i++)
		{
            if(result = MDSetWaveUnits(wavH, i, ArgValUnits[i])) return result;
		}
        if(result = MDSetWaveUnits(wavH, -1, ArgValUnits[nDim])) return result;
	}

	WaveHandleModified(wavH);
	//HSetState((Handle)wavH, hState); //OC080613 (moving to XOPSupport 6)
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrStringFromTextWave1D(waveHndl wavH, int& NumRows, char**& pStrData)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result = 0;
	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	if(numDimensions > 1) return TEXT_WAVE_1D_REQUIRED;

	long nRows = dimensionSizes[0];
	if(nRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;

	if(NumRows > 0)
	{
        if(nRows < NumRows) return TOO_SHORT_WAVE;
	}
	else NumRows = nRows;

	long RadIndices[MAX_DIMENSIONS+1];
	for(long k=0; k<=MAX_DIMENSIONS; k++) RadIndices[k] = 0;

	long totNumStr = NumRows;
	pStrData = new char*[totNumStr];
	char** tStrData = pStrData;

	//RadIndices[1] = i; // or [0]?
	for(long j=0; j<NumRows; j++)
	{
		RadIndices[0] = j; // or [1]?

		Handle textH = NewHandle(0);
		if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
		if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;

		int curStrLen = (int)strlen(*textH) + 10;
		*tStrData = new char[curStrLen];

		strcpy(*tStrData, *textH);
		DisposeHandle(textH);
		tStrData++;
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::GetArrStringFromTextWave2D(waveHndl wavH, int& NumRows, int& NumCols, char**& pStrData)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result = 0;
	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	if(numDimensions > 2) return TEXT_WAVE_2D_REQUIRED;

	long nRows = dimensionSizes[0];
	if(nRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	long nCols = dimensionSizes[1];
	if(nCols <= 0) nCols = 1;

	if(NumRows > 0)
	{
        if(nRows < NumRows) return TOO_SHORT_WAVE;
	}
	else NumRows = nRows;

	if(NumCols > 0)
	{
        if(nCols < NumCols) return TOO_SHORT_WAVE;
	}
	else NumCols = nCols;

	long RadIndices[MAX_DIMENSIONS+1];
	for(long k=0; k<=MAX_DIMENSIONS; k++) RadIndices[k] = 0;

	long totNumStr = NumRows*NumCols;
	pStrData = new char*[totNumStr];
	char** tStrData = pStrData;

	for(long i=0; i<NumCols; i++)
	{
		RadIndices[1] = i; // or [0]?
		for(long j=0; j<NumRows; j++)
		{
			RadIndices[0] = j; // or [1]?

			Handle textH = NewHandle(0);
			if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
			if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;

			int curStrLen = (int)strlen(*textH) + 10;
			*tStrData = new char[curStrLen];

			strcpy(*tStrData, *textH);
			DisposeHandle(textH);
			tStrData++;
		}
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetZeroTextWave2D(waveHndl wavH)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result = 0;
	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	if(numDimensions > 2) return TEXT_WAVE_2D_REQUIRED;

	long nRows = dimensionSizes[0];
	if(nRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	long nCols = dimensionSizes[1];
	if(nCols <= 0) nCols = 1;

	long RadIndices[MAX_DIMENSIONS+1];
	for(long k=0; k<=MAX_DIMENSIONS; k++) RadIndices[k] = 0;

	for(long i=0; i<nCols; i++)
	{
		RadIndices[1] = i; // or [0]?
		for(long j=0; j<nRows; j++)
		{
            RadIndices[0] = j; // or [1]?
            Handle textH = NewHandle(0);
            if(result = MDSetTextWavePointValue(wavH, RadIndices, textH)) return result;
            DisposeHandle(textH);
		}
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetZeroTextWave3D(waveHndl wavH)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result = 0;
	//long numDimensions;
	int numDimensions; //OC080613 (moving to XOPSupport 6)
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	//if(numDimensions > 2) return TEXT_WAVE_2D_REQUIRED;

	long nRows = dimensionSizes[0];
	//if(nRows <= 0) return ZERO_NUMBER_OF_ELEM_IN_WAVE;
	long nCols = dimensionSizes[1];
	//if(nCols <= 0) nCols = 1;
	long nChunks = dimensionSizes[2];
	
	if((nRows <= 0) && (nCols <= 0) && (nChunks <= 0)) return 0;

	long RadIndices[MAX_DIMENSIONS+1];
	for(long k=0; k<=MAX_DIMENSIONS; k++) RadIndices[k] = 0;
	
	for(long r=0; r<nChunks; r++)
	{
		RadIndices[2] = r; // or [0]?
		for(long i=0; i<nCols; i++)
		{
			RadIndices[1] = i; // or [0]?
			for(long j=0; j<nRows; j++)
			{
			    RadIndices[0] = j; // or [1]?
			   Handle textH = NewHandle(0);
			    if(result = MDSetTextWavePointValue(wavH, RadIndices, textH)) return result;
			    DisposeHandle(textH);
			}
		}
	}
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetDataInTextWave2D(waveHndl wavH, char** arrStr, int* arrLen, int numArr)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	if((numArr <= 0) || (arrLen == 0)) return 0;

	int res = SetZeroTextWave2D(wavH);
	if(res != 0) return res;

	long RadIndices[MAX_DIMENSIONS+1];
	for(long k=0; k<=MAX_DIMENSIONS; k++) RadIndices[k] = 0;

	char** tArrStr = arrStr;
	for(int i=0; i<numArr; i++)
	{
		RadIndices[1] = i; // or [0]?
		int curLen = arrLen[i];
		for(int j=0; j<curLen; j++)
		{
			RadIndices[0] = j; // or [1]?

            int AmOfBytes = (int)strlen(*tArrStr);
            Handle textH = NewHandle(AmOfBytes);
            strncpy(*textH, *tArrStr, AmOfBytes);
            if(res = MDSetTextWavePointValue(wavH, RadIndices, textH)) return res;
            DisposeHandle(textH);

			tArrStr++;
		}
	}
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorInterf::SetDataInTextWave3D(waveHndl wavH, char** arrStr, int numRows, int numCols, int numChunks)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	if((numRows == 0) || (numCols == 0) || (numChunks == 0)) return 0;

	int res = SetZeroTextWave3D(wavH);
	if(res != 0) return res;
	
	long RadIndices[MAX_DIMENSIONS+1];
	for(long k=0; k<=MAX_DIMENSIONS; k++) RadIndices[k] = 0;
	
	char** tArrStr = arrStr;
	for(int r=0; r<numChunks; r++)
	{
		RadIndices[2] = r; // or [0]?
		for(int i=0; i<numCols; i++)
		{
			RadIndices[1] = i; // or [0]?
			for(int j=0; j<numRows; j++)
			{
				RadIndices[0] = j; // or [1]?

				int AmOfBytes = (int)strlen(*tArrStr);
				Handle textH = NewHandle(AmOfBytes);
				strncpy(*textH, *tArrStr, AmOfBytes);
				if(res = MDSetTextWavePointValue(wavH, RadIndices, textH)) return res;
				DisposeHandle(textH);

				tArrStr++;
			}
		}
	}
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************
/**
int srTIgorSend::GetElecBeamThin(waveHndl wavH, double& I, double* pMom1, double& s0)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 7) return BAD_EL_BEAM_WAVE_FORMAT;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp = (DOUBLE*)dataStartPtr;

	double &Energy = pMom1[0], &x0 = pMom1[1], &dxds0 = pMom1[2], &z0 = pMom1[3], &dzds0 = pMom1[4];
	pMom1[5] = 0;
	
	Energy = *(dp++);
	I = *(dp++);
	x0 = *(dp++); // Assuming input in m !
	dxds0 = *(dp++); // dxds0 assumed in r at the input !
	z0 = *(dp++); // Assuming input in m !
	dzds0 = *(dp++); // dzds0 assumed in r at the input !
	s0 = *dp; // Assuming input in m !

	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::SetElecBeamThin(waveHndl wavH, double I, double* pMom1, double s0)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 7) return BAD_EL_BEAM_WAVE_FORMAT;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp = (DOUBLE*)dataStartPtr;

	double &Energy = pMom1[0], &x0 = pMom1[1], &dxds0 = pMom1[2], &z0 = pMom1[3], &dzds0 = pMom1[4];
	pMom1[5] = 0;
	
	*(dp++) = Energy;
	*(dp++) = I;
	*(dp++) = x0; // Assuming input in m !
	*(dp++) = dxds0; // dxds0 assumed in r at the input !
	*(dp++) = z0; // Assuming input in m !
	*(dp++) = dzds0; // dzds0 assumed in r at the input !
	*dp = s0; // Assuming input in m !

	HSetState((Handle)wavH, hState);
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetElecBeamThick(waveHndl wavH, double& I, double* pMom1, double* pMom2, int& nMom2, double& s0, int& TypeDistrTransverse, int& TypeDistrLongitudinal, double& ShortNoiseFactor)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 7) return BAD_EL_BEAM_WAVE_FORMAT;

	if(result = GetElecBeamThin(wavH, I, pMom1, s0)) return result;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp0 = (DOUBLE*)dataStartPtr;
	DOUBLE* dp = dp0;

// Second-order moments
	double &Mxx = pMom2[0], &Mxxp = pMom2[1], &Mxpxp = pMom2[2];
	double &Mzz = pMom2[3], &Mzzp = pMom2[4], &Mzpzp = pMom2[5]; 
	double &Mxz = pMom2[6], &Mxpz = pMom2[7], &Mxzp = pMom2[8], &Mxpzp = pMom2[9]; 
	double &Mee = pMom2[10], &Mss = pMom2[11];

	dp = dp0 + 13;
	Mee = (*dp)*(*dp); dp++; 

	dp = dp0 + 20;
	Mxx = *(dp++); // Assuming input in m^2 !
	Mxxp = *(dp++); // Assuming input in m !
	Mxpxp = *(dp++); // Assuming input in r^2 !
	Mzz = *(dp++); // Assuming input in m^2 !
	Mzzp = *(dp++); // Assuming input in m !
	Mzpzp = *(dp++); // Assuming input in r^2 !
	Mxz = *(dp++); // Assuming input in m^2 !
	Mxpz = *(dp++); // Assuming input in m !
	Mxzp = *(dp++); // Assuming input in m !
	Mxpzp = *(dp++); // Assuming input in r^2 !

	nMom2 = 11;

// More of Transverse & Longitudinal parameters (for SASE, coherent computations)
	if(numRows > 30)
	{
		dp = dp0 + 30;
		TypeDistrTransverse = (int)(*(dp++)); 
		TypeDistrLongitudinal = (int)(*(dp++)); 
		ShortNoiseFactor = *(dp++); 
		Mss = *(dp++); // Assuming input in m^2 !

		nMom2 = 12;
	}
	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::SetElecBeamThick(waveHndl wavH, double I, double* pMom1, double* pMom2, double s0, int TypeDistrTransverse, int TypeDistrLongitudinal, double ShortNoiseFactor)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 7) return BAD_EL_BEAM_WAVE_FORMAT;

	if(result = SetElecBeamThin(wavH, I, pMom1, s0)) return result;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp0 = (DOUBLE*)dataStartPtr;
	DOUBLE* dp = dp0;

// Second-order moments
	double &Mxx = pMom2[0], &Mxxp = pMom2[1], &Mxpxp = pMom2[2];
	double &Mzz = pMom2[3], &Mzzp = pMom2[4], &Mzpzp = pMom2[5]; 
	double &Mxz = pMom2[6], &Mxpz = pMom2[7], &Mxzp = pMom2[8], &Mxpzp = pMom2[9]; 
	double &Mee = pMom2[10], &Mss = pMom2[11];

	dp = dp0 + 13;
	*dp = sqrt(Mee); //Mee = (*dp)*(*dp); dp++; 

	dp = dp0 + 20;
	*(dp++) = Mxx; //Mxx = *(dp++); // Assuming input in m^2 !
	*(dp++) = Mxxp; //Mxxp = *(dp++); // Assuming input in m !
	*(dp++) = Mxpxp; //Mxpxp = *(dp++); // Assuming input in r^2 !
	*(dp++) = Mzz; //Mzz = *(dp++); // Assuming input in m^2 !
	*(dp++) = Mzzp; //Mzzp = *(dp++); // Assuming input in m !
	*(dp++) = Mzpzp; //Mzpzp = *(dp++); // Assuming input in r^2 !
	*(dp++) = Mxz; //Mxz = *(dp++); // Assuming input in m^2 !
	*(dp++) = Mxpz; //Mxpz = *(dp++); // Assuming input in m !
	*(dp++) = Mxzp; //Mxzp = *(dp++); // Assuming input in m !
	*(dp++) = Mxpzp; //Mxpzp = *(dp++); // Assuming input in r^2 !

// More of Transverse & Longitudinal parameters (for SASE, coherent computations)
	if(numRows > 30)
	{
		dp = dp0 + 30;
		*(dp++) = TypeDistrTransverse; //TypeDistrTransverse = (int)(*(dp++)); 
		*(dp++) = TypeDistrLongitudinal; //TypeDistrLongitudinal = (int)(*(dp++)); 
		*(dp++) = ShortNoiseFactor; //ShortNoiseFactor = *(dp++); 
		*dp = Mss; //Mss = *(dp++); // Assuming input in m^2 !
	}

	HSetState((Handle)wavH, hState);
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetElecBeamTwiss(waveHndl wavH, double* pHorTwiss, double* pVertTwiss)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 18) return BAD_EL_BEAM_WAVE_FORMAT;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* ElecBeamDataInIgor = (DOUBLE*)dataStartPtr;

// Twiss parameters
	double &BetaX = pHorTwiss[0], &AlphaX = pHorTwiss[1], &EtaX = pHorTwiss[2], &EtaPrX = pHorTwiss[3];
	double &BetaZ = pVertTwiss[0], &AlphaZ = pVertTwiss[1], &EtaZ = pVertTwiss[2], &EtaPrZ = pVertTwiss[3];
	
	BetaX = ElecBeamDataInIgor[8];
	AlphaX = ElecBeamDataInIgor[9];
	EtaX = ElecBeamDataInIgor[14];
	EtaPrX = ElecBeamDataInIgor[15];

	BetaZ = ElecBeamDataInIgor[11];
	AlphaZ = ElecBeamDataInIgor[12];
	EtaZ = ElecBeamDataInIgor[16];
	EtaPrZ = ElecBeamDataInIgor[17];

	//SrwElecEmx=$ElecName[7]
	//SrwElecBetax=$ElecName[8]
	//SrwElecAlphax=$ElecName[9]
	//SrwElecEmz=$ElecName[10]
	//SrwElecBetaz=$ElecName[11]
	//SrwElecAlphaz=$ElecName[12]
	//SrwElecSige=$ElecName[13]
	//SrwElecEtax=$ElecName[14]
	//SrwElecEtaxPr=$ElecName[15]
	//SrwElecEtaz=$ElecName[16]
	//SrwElecEtazPr=$ElecName[17]

	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::SetElecBeamEmitAndTwiss(waveHndl wavH, double HorEmit, double VertEmit, double* pHorTwiss, double* pVertTwiss)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 18) return BAD_EL_BEAM_WAVE_FORMAT;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* ElecBeamDataInIgor = (DOUBLE*)dataStartPtr;

// Twiss parameters
	double &BetaX = pHorTwiss[0], &AlphaX = pHorTwiss[1], &EtaX = pHorTwiss[2], &EtaPrX = pHorTwiss[3];
	double &BetaZ = pVertTwiss[0], &AlphaZ = pVertTwiss[1], &EtaZ = pVertTwiss[2], &EtaPrZ = pVertTwiss[3];
	
	ElecBeamDataInIgor[7] = HorEmit*(1.E+09);
	ElecBeamDataInIgor[8] = BetaX;
	ElecBeamDataInIgor[9] = AlphaX;
	ElecBeamDataInIgor[14] = EtaX;
	ElecBeamDataInIgor[15] = EtaPrX;

	ElecBeamDataInIgor[10] = VertEmit*(1.E+09);
	ElecBeamDataInIgor[11] = BetaZ;
	ElecBeamDataInIgor[12] = AlphaZ;
	ElecBeamDataInIgor[16] = EtaZ;
	ElecBeamDataInIgor[17] = EtaPrZ;

	//SrwElecEmx=$ElecName[7]
	//SrwElecBetax=$ElecName[8]
	//SrwElecAlphax=$ElecName[9]
	//SrwElecEmz=$ElecName[10]
	//SrwElecBetaz=$ElecName[11]
	//SrwElecAlphaz=$ElecName[12]
	//SrwElecSige=$ElecName[13]
	//SrwElecEtax=$ElecName[14]
	//SrwElecEtaxPr=$ElecName[15]
	//SrwElecEtaz=$ElecName[16]
	//SrwElecEtazPr=$ElecName[17]

	HSetState((Handle)wavH, hState);
	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetMagFieldTransvUnif(waveHndl wField, double FieldZeroTolerance, double& sStart, double& sStep, long& BLen, double*& pBxData, bool& BxIsZero, double*& pBzData, bool& BzIsZero)
{
	if(wField == NIL) return NOWAV;
	int waveType = WaveType(wField);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result;
	long RadIndices[MAX_DIMENSIONS];

	Handle textH = NewHandle(0L);
	RadIndices[0] = 0;
	if(result = MDGetTextWavePointValue(wField, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wBx = FetchWave(*textH);
	DisposeHandle(textH);

	textH = NewHandle(0L);
	RadIndices[0] = 1;
	if(result = MDGetTextWavePointValue(wField, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wBz = FetchWave(*textH);
	DisposeHandle(textH);

	waveHndl wavH = wBx;
	if(wavH == NIL) return NOWAV;
	waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	BLen = dimensionSizes[0];
	if((numDimensions == 0) || (BLen == 0)) return BAD_MAGN_FIELD_WAVE_FORMAT;

	DOUBLE Aux_sBxStep, Aux_sBxStart;
	if(result = MDGetWaveScaling(wavH, 0, &Aux_sBxStep, &Aux_sBxStart)) return result;
	sStart = (double)Aux_sBxStart;
	sStep = (double)Aux_sBxStep;

	pBxData = new double[BLen];
	if(pBxData == 0) return MEMORY_ALLOCATION_FAILURE;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) { delete[] pBxData; return result;}
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp0 = (DOUBLE*)dataStartPtr;

	double* tBxData = pBxData;
	double BxVal;
	for(int k=0; k<BLen; k++)
	{
		BxVal = *(dp0++);
		if(fabs(BxVal) > FieldZeroTolerance) BxIsZero = false;
		else BxVal = 0.;
		*(tBxData++) = BxVal;
	}
	HSetState((Handle)wavH, hState);

	wavH = wBz;
	if(wavH == NIL) return NOWAV;
	waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long BzLen = dimensionSizes[0];
	if((numDimensions == 0) || (BzLen == 0)) return BAD_MAGN_FIELD_WAVE_FORMAT;

	DOUBLE Aux_sBzStep, Aux_sBzStart;
	if(result = MDGetWaveScaling(wavH, 0, &Aux_sBzStep, &Aux_sBzStart)) return result;
	double sBzStart = (double)Aux_sBzStart;
	double sBzStep = (double)Aux_sBzStep;

	if((BzLen != BLen) || (fabs(sBzStart - sStart) > 1.E-05) || (fabs(sBzStep - sStep) > 1.E-05))
		return UNEQUAL_BX_BZ_DEFINITION_LIMITS; //to remove?

	pBzData = new double[BzLen];
	if(pBzData == 0) return MEMORY_ALLOCATION_FAILURE;

	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	hState = MoveLockHandle(wavH);
	dataStartPtr = (char*)(*wavH) + dataOffset;
	dp0 = (DOUBLE*)dataStartPtr;

	double* tBzData = pBzData;
	double BzVal;
	for(int j=0; j<BzLen; j++)
	{
		BzVal = *(dp0++);
		if(fabs(BzVal) > FieldZeroTolerance) BzIsZero = false;
		else BzVal = 0.;
		*(tBzData++) = BzVal;
	}
	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetMagFieldPeriodic(waveHndl wavH, double& PerLength, double& TotLength, int& AmOfHarm, int*& ArrHarmNo, char*& ArrXorZ, double*& ArrK, double*& ArrPhase, int& TypeOfUnd, double& TaperPar_TU, double& PhaseSh_OK, int& FldErrTypeSASE, double& FldErrRMS, double& NatFocNxSASE, double& NatFocNySASE, int& TaperTypeSASE, double& TaperStartSASE, double& TaperRelFldChgSASE)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result;
	long numDimensionsM;
	long dimensionSizesM[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensionsM, dimensionSizesM)) return result;
	long NumOfLines = dimensionSizesM[0];

	double BufValue;
	long RadIndices[MAX_DIMENSIONS];
	Handle textH;

	if(result = GetDoubleFromTextWave1D(wavH, 0, PerLength)) return result;
	if(result = GetDoubleFromTextWave1D(wavH, 1, TotLength)) return result;
	if(result = GetDoubleFromTextWave1D(wavH, 2, BufValue)) return result;

// Type of undulator at the input:
// 1- normal; 2- tapered; 3- optical clystron; 4- infinite; 
// Type of undulator in C code:
// 0- infinite; 1- normal; 2- tapered; 3- optical clystron.
	int TypeOfUndulatorAtInput = int(BufValue + 1.E-12);
	if(TypeOfUndulatorAtInput == 1) TypeOfUnd = 1; // Conventional
	else if(TypeOfUndulatorAtInput == 2) TypeOfUnd = 2; // Tapered
	else if(TypeOfUndulatorAtInput == 3) TypeOfUnd = 3; // Opt. Klystron
	else if(TypeOfUndulatorAtInput == 4) TypeOfUnd = 0; // Infinite

	if(TypeOfUnd == 2) // Tapered
	{
		if(result = GetDoubleFromTextWave1D(wavH, 3, TaperPar_TU)) return result;
	}
	else if(TypeOfUnd == 3) // Opt. Klystron
	{
		if(result = GetDoubleFromTextWave1D(wavH, 3, PhaseSh_OK)) return result;
	}

	//if(result = GetDoubleFromTextWave1D(wavH, 4, Fund_keV_per_GeV2)) return result;
	if(result = GetDoubleFromTextWave1D(wavH, 5, BufValue)) return result;
	AmOfHarm = int(BufValue + 1.E-12);
	if(AmOfHarm > 20) return TOO_MANY_MAG_FIELD_HARMONICS;

	//MagFldHarmArr = new srTMagFldHarm[AmOfHarm];
	ArrHarmNo = new int[AmOfHarm];
    ArrXorZ = new char[AmOfHarm];
	ArrK = new double[AmOfHarm];
	ArrPhase = new double[AmOfHarm];

	int* tArrHarmNo = ArrHarmNo;
    char* tArrXorZ = ArrXorZ;
	double* tArrK = ArrK;
	double* tArrPhase = ArrPhase;

	for(int i=0; i<AmOfHarm; i++)
	{
		textH = NewHandle(0L);
		RadIndices[0] = i + 6;
		if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
		if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
		waveHndl wHarm = FetchWave(*textH);
		DisposeHandle(textH);
		if(wHarm == NIL) return CAN_NOT_FIND_HARMONIC_WAVE;

		int waveType = WaveType(wHarm);
		if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

		long numDimensions;
		long dimensionSizes[MAX_DIMENSIONS+1];
		if(result = MDGetWaveDimensions(wHarm, &numDimensions, dimensionSizes)) return result;
		if((numDimensions > 1) || (dimensionSizes[0] < 4)) return IMPROPER_FIELD_HARMONIC_STRUCTURE;

		long dataOffset;
		if(result = MDAccessNumericWaveData(wHarm, kMDWaveAccessMode0, &dataOffset)) return result;
		int hState = MoveLockHandle(wHarm);
		char* dataStartPtr = (char*)(*wHarm) + dataOffset;
		DOUBLE* dp = (DOUBLE*)dataStartPtr;

		//srTMagFldHarm* pCurHarm = MagFldHarmArr + i;
		*(tArrHarmNo++) = int(*(dp++));
		*(tArrXorZ++) = (*(dp++) == 1.)? 'z' : 'x';
		*(tArrK++) = *(dp++);
		*(tArrPhase++) = *dp;

		HSetState((Handle)wHarm, hState);
	}

	if(NumOfLines > 30)
	{
		double BufVal;
		//if(result = GetDoubleFromTextWave1D(wavH, 30, BufVal)) return result; 
		//MagPer.NatFocTypeSASE = int(BufVal + 0.00001 - 1); // 0- natural, 1- other

		if(result = GetDoubleFromTextWave1D(wavH, 30, NatFocNxSASE)) return result; 
		if(result = GetDoubleFromTextWave1D(wavH, 31, NatFocNySASE)) return result; 
		if(result = GetDoubleFromTextWave1D(wavH, 32, BufVal)) return result; 
		FldErrTypeSASE = int(BufVal + 0.00001); // 0- No errors; 1- Uniform uncorrelated; 2- Uniform correlated; 3- Gaussian uncorrelated; 4- Gaussian correlated

		if(result = GetDoubleFromTextWave1D(wavH, 33, FldErrRMS)) return result;
		if(FldErrRMS == 0.) FldErrTypeSASE = 0;

		if(result = GetDoubleFromTextWave1D(wavH, 35, BufVal)) return result; 
		TaperTypeSASE = int(BufVal + 0.00001); // 0- No taper; 1- Linear; 2- Quadratic
		
		if(result = GetDoubleFromTextWave1D(wavH, 36, TaperStartSASE)) return result; 
		if(result = GetDoubleFromTextWave1D(wavH, 37, TaperRelFldChgSASE)) return result;
		if(TaperRelFldChgSASE == 0.) TaperTypeSASE = 0;
	}
	return 0;
}

//*************************************************************************

int srTIgorSend::GetMagFieldPeriodic(waveHndl wavH, double& PerLength, double& TotLength, int& AmOfHarm, srTMagFldHarm*& MagFldHarmArr, int& TypeOfUnd, double& TaperPar_TU, double& PhaseSh_OK, int& FldErrTypeSASE, double& FldErrRMS, double& NatFocNxSASE, double& NatFocNySASE, int& TaperTypeSASE, double& TaperStartSASE, double& TaperRelFldChgSASE)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result;
	long numDimensionsM;
	long dimensionSizesM[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensionsM, dimensionSizesM)) return result;
	long NumOfLines = dimensionSizesM[0];

	double BufValue;
	long RadIndices[MAX_DIMENSIONS];
	Handle textH;

	if(result = GetDoubleFromTextWave1D(wavH, 0, PerLength)) return result;
	if(result = GetDoubleFromTextWave1D(wavH, 1, TotLength)) return result;
	if(result = GetDoubleFromTextWave1D(wavH, 2, BufValue)) return result;

// Type of undulator at the input:
// 1- normal; 2- tapered; 3- optical clystron; 4- infinite; 
// Type of undulator in C code:
// 0- infinite; 1- normal; 2- tapered; 3- optical clystron.
	int TypeOfUndulatorAtInput = int(BufValue + 1.E-12);
	if(TypeOfUndulatorAtInput == 1) TypeOfUnd = 1; // Conventional
	else if(TypeOfUndulatorAtInput == 2) TypeOfUnd = 2; // Tapered
	else if(TypeOfUndulatorAtInput == 3) TypeOfUnd = 3; // Opt. Klystron
	else if(TypeOfUndulatorAtInput == 4) TypeOfUnd = 0; // Infinite

	if(TypeOfUnd == 2) // Tapered
	{
		if(result = GetDoubleFromTextWave1D(wavH, 3, TaperPar_TU)) return result;
	}
	else if(TypeOfUnd == 3) // Opt. Klystron
	{
		if(result = GetDoubleFromTextWave1D(wavH, 3, PhaseSh_OK)) return result;
	}

	//if(result = GetDoubleFromTextWave1D(wavH, 4, Fund_keV_per_GeV2)) return result;
	if(result = GetDoubleFromTextWave1D(wavH, 5, BufValue)) return result;
	AmOfHarm = int(BufValue + 1.E-12);
	if(AmOfHarm > 20) return TOO_MANY_MAG_FIELD_HARMONICS;

	MagFldHarmArr = new srTMagFldHarm[AmOfHarm];
	for(int i=0; i<AmOfHarm; i++)
	{
		textH = NewHandle(0L);
		RadIndices[0] = i + 6;
		if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
		if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
		waveHndl wHarm = FetchWave(*textH);
		DisposeHandle(textH);
		if(wHarm == NIL) return CAN_NOT_FIND_HARMONIC_WAVE;

		int waveType = WaveType(wHarm);
		if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

		long numDimensions;
		long dimensionSizes[MAX_DIMENSIONS+1];
		if(result = MDGetWaveDimensions(wHarm, &numDimensions, dimensionSizes)) return result;
		if((numDimensions > 1) || (dimensionSizes[0] < 4)) return IMPROPER_FIELD_HARMONIC_STRUCTURE;

		long dataOffset;
		if(result = MDAccessNumericWaveData(wHarm, kMDWaveAccessMode0, &dataOffset)) return result;
		int hState = MoveLockHandle(wHarm);
		char* dataStartPtr = (char*)(*wHarm) + dataOffset;
		DOUBLE* dp = (DOUBLE*)dataStartPtr;

		srTMagFldHarm* pCurHarm = MagFldHarmArr + i;
		pCurHarm->n = int(*(dp++));
		pCurHarm->XorZ = (*(dp++) == 1.)? 'z' : 'x';
		pCurHarm->K = *(dp++);
		pCurHarm->Phase = *dp;

		HSetState((Handle)wHarm, hState);
	}

	if(NumOfLines > 30)
	{
		double BufVal;
		//if(result = GetDoubleFromTextWave1D(wavH, 30, BufVal)) return result; 
		//MagPer.NatFocTypeSASE = int(BufVal + 0.00001 - 1); // 0- natural, 1- other

		if(result = GetDoubleFromTextWave1D(wavH, 30, NatFocNxSASE)) return result; 
		if(result = GetDoubleFromTextWave1D(wavH, 31, NatFocNySASE)) return result; 
		if(result = GetDoubleFromTextWave1D(wavH, 32, BufVal)) return result; 
		FldErrTypeSASE = int(BufVal + 0.00001); // 0- No errors; 1- Uniform uncorrelated; 2- Uniform correlated; 3- Gaussian uncorrelated; 4- Gaussian correlated

		if(result = GetDoubleFromTextWave1D(wavH, 33, FldErrRMS)) return result;
		if(FldErrRMS == 0.) FldErrTypeSASE = 0;

		if(result = GetDoubleFromTextWave1D(wavH, 35, BufVal)) return result; 
		TaperTypeSASE = int(BufVal + 0.00001); // 0- No taper; 1- Linear; 2- Quadratic
		
		if(result = GetDoubleFromTextWave1D(wavH, 36, TaperStartSASE)) return result; 
		if(result = GetDoubleFromTextWave1D(wavH, 37, TaperRelFldChgSASE)) return result;
		if(TaperRelFldChgSASE == 0.) TaperTypeSASE = 0;
	}
	return 0;
}

//*************************************************************************

int srTIgorSend::SetMagFieldPeriodic(waveHndl wavH, double PerLength, double TotLength, int AmOfHarm, int* ArrHarmNo, char* ArrXorZ, double* ArrK, double* ArrPhase, int TypeOfUnd, double TaperPar_TU, double PhaseSh_OK, double Fund_keV_per_GeV2)
{
	int res = 0;
	vector<string> StrCont;

	char buffer[200];
	sprintf(buffer, "%f", PerLength); StrCont.push_back(buffer); //[0]
	sprintf(buffer, "%f", TotLength); StrCont.push_back(buffer); //[1]
	sprintf(buffer, "%d", TypeOfUnd); StrCont.push_back(buffer); //[2]

	strcpy(buffer, " ");
	if(TypeOfUnd == 2) // Tapered
	{
		sprintf(buffer, "%f", TaperPar_TU); 
	}
	else if(TypeOfUnd == 3) // Opt. Klystron
	{
		sprintf(buffer, "%f", PhaseSh_OK); 
	}
    StrCont.push_back(buffer); //[3]
	
	sprintf(buffer, "%f", Fund_keV_per_GeV2); StrCont.push_back(buffer); //[4]
	sprintf(buffer, "%d", AmOfHarm); StrCont.push_back(buffer); //[5]
	
	long dimensionSizes[MAX_DIMENSIONS+1];
	dimensionSizes[0] = 4; //size of wave to create
	for(int i=1; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;

	long indices[MAX_DIMENSIONS];
	for(int j=0; j<MAX_DIMENSIONS; j++) indices[i] = 0;

	DOUBLE valueHarmWave[] = {0, 0};

	char NameHarmCommon[MAX_OBJ_NAME+1];
	NameHarmCommon[0] = '\0';
	WaveName(wavH, NameHarmCommon);
	int LenNameHarmCommon = strlen(NameHarmCommon);
	if(LenNameHarmCommon > 4) NameHarmCommon[LenNameHarmCommon - 4] = '\0';

	char NameHarmExt[] = "_fha";

	char HarmNumStr[MAX_OBJ_NAME+1];
	char NameHarm[MAX_OBJ_NAME+1];

	int *tArrHarmNo = ArrHarmNo;
	char *tArrXorZ = ArrXorZ;
	double *tArrK = ArrK;
	double *tArrPhase = ArrPhase;

	for(int i=1; i<=AmOfHarm; i++)
	{
        waveHndl wCurHarm;
		strcpy(NameHarm, NameHarmCommon);
		sprintf(HarmNumStr, "%d", i);
		strcat(NameHarm, HarmNumStr);
		strcat(NameHarm, NameHarmExt);
		if(res = MDMakeWave(&wCurHarm, NameHarm, NIL, dimensionSizes, NT_FP64, 1)) return res;
		StrCont.push_back(NameHarm);

		*indices = 0; *valueHarmWave = *(tArrHarmNo++);
		if(res = MDSetNumericWavePointValue(wCurHarm, indices, valueHarmWave)) return res;
		
		char CurPol = *(tArrXorZ++);
		char iArrXorZ = (((CurPol == 'v') || (CurPol == 'z'))? 1 : 2);
		*indices = 1; *valueHarmWave = iArrXorZ;
		if(res = MDSetNumericWavePointValue(wCurHarm, indices, valueHarmWave)) return res;

		*indices = 2; *valueHarmWave = *(tArrK++);
		if(res = MDSetNumericWavePointValue(wCurHarm, indices, valueHarmWave)) return res;

		*indices = 3; *valueHarmWave = *(tArrPhase++);
		if(res = MDSetNumericWavePointValue(wCurHarm, indices, valueHarmWave)) return res;
	}
    return UpdateTextWave1D(wavH, &StrCont);
}

//*************************************************************************

int srTIgorSend::GetMagFieldConstant(waveHndl wavH, double& Bx, double& Bz)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result;
	if(result = GetDoubleFromTextWave1D(wavH, 0, Bx)) return result;
	if(result = GetDoubleFromTextWave1D(wavH, 1, Bz)) return result;
	return 0;
}

//*************************************************************************

int srTIgorSend::IdentifyMagFieldType(waveHndl wavH, int& MagFieldType)
{ // 1: arbitrary, 2: periodic, 3: constant, 4: mag. optics, 5: container
	if(wavH == NIL) return NOWAV;

	char NameOfFieldWave[MAX_OBJ_NAME+1];
	WaveName(wavH, NameOfFieldWave);

	MagFieldType = IdentifyMagFieldTypeFromName(NameOfFieldWave);
	return 0;
}

//*************************************************************************

int srTIgorSend::IdentifyMagFieldTypeFromName(char* MagFieldName)
{ // 1: arbitrary, 2: periodic, 3: constant, 4: mag. optics, 5: container
	if(MagFieldName == 0) return 0;
	char* pEnding = strrchr(MagFieldName, '_');
	if(!strcmp(pEnding, "_mag")) return 1;
	else if(!strcmp(pEnding, "_map")) return 2;
	else if(!strcmp(pEnding, "_mac")) return 3;
	else if(!strcmp(pEnding, "_mgo")) return 4;
	else if(!strcmp(pEnding, "_mgc")) return 5;
	else return 0;
}

//*************************************************************************

int srTIgorSend::GetTrjDataPointers(waveHndl wavH_OutBtxData, waveHndl wavH_OutXData, waveHndl wavH_OutBtzData, waveHndl wavH_OutZData, double*& pOutBtxData, double*& pOutXData, double*& pOutBtzData, double*& pOutZData, int& hStateOutBtxData, int& hStateOutXData, int& hStateOutBtzData, int& hStateOutZData)
{
	if(wavH_OutBtxData == NIL) return NOWAV;
	if(WaveType(wavH_OutBtxData) != NT_FP64) return NT_FP64_WAVE_REQUIRED;
	if(wavH_OutXData == NIL) return NOWAV;
	if(WaveType(wavH_OutXData) != NT_FP64) return NT_FP64_WAVE_REQUIRED;
	if(wavH_OutBtzData == NIL) return NOWAV;
	if(WaveType(wavH_OutBtzData) != NT_FP64) return NT_FP64_WAVE_REQUIRED;
	if(wavH_OutZData == NIL) return NOWAV;
	if(WaveType(wavH_OutZData) != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	int result;
	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH_OutBtxData, kMDWaveAccessMode0, &dataOffset)) return result;
	hStateOutBtxData = MoveLockHandle(wavH_OutBtxData);
	pOutBtxData = (double*)((char*)(*wavH_OutBtxData) + dataOffset);
	if(result = MDAccessNumericWaveData(wavH_OutXData, kMDWaveAccessMode0, &dataOffset)) return result;
	hStateOutXData = MoveLockHandle(wavH_OutXData);
	pOutXData = (DOUBLE*)((char*)(*wavH_OutXData) + dataOffset);
	if(result = MDAccessNumericWaveData(wavH_OutBtzData, kMDWaveAccessMode0, &dataOffset)) return result;
	hStateOutBtzData = MoveLockHandle(wavH_OutBtzData);
	pOutBtzData = (DOUBLE*)((char*)(*wavH_OutBtzData) + dataOffset);
	if(result = MDAccessNumericWaveData(wavH_OutZData, kMDWaveAccessMode0, &dataOffset)) return result;
	hStateOutZData = MoveLockHandle(wavH_OutZData);
	pOutZData = (DOUBLE*)((char*)(*wavH_OutZData) + dataOffset);

	return 0;
}

//*************************************************************************

int srTIgorSend::FinishWorkingWithTrjDataPointers(waveHndl wOutHorAng, waveHndl wOutHorCoor, waveHndl wOutVerAng, waveHndl wOutVerCoor, int hStateHorAng, int hStateHorCoor, int hStateVerAng, int hStateVerCoor)
{
	HSetState((Handle)wOutHorAng, hStateHorAng);
	WaveHandleModified(wOutHorAng);
	HSetState((Handle)wOutHorCoor, hStateHorCoor);
	WaveHandleModified(wOutHorCoor);
	HSetState((Handle)wOutVerAng, hStateVerAng);
	WaveHandleModified(wOutVerAng);
	HSetState((Handle)wOutVerCoor, hStateVerCoor);
	WaveHandleModified(wOutVerCoor);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetWfrSampling(waveHndl wavH, double& s, double& zSt, double& zFi, int& nz, double& xSt, double& xFi, int& nx, double& eSt, double& eFi, int& ne, char* PhotEnUnits)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 5) return BAD_OBS_WAVE_FORMAT;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp0 = (DOUBLE*)dataStartPtr;

	dp0 += 4; // The fields 0 - 3 are not used

	s = *(dp0++); 

	strcpy(PhotEnUnits, "eV");

	eSt = *(dp0++); // Assuming input in eV !
	if(eSt <= 0.) return WAVELENGTH_SHOULD_BE_POSITIVE;
	eFi = *(dp0++); // Assuming input in eV !
	if(eFi <= 0.) return WAVELENGTH_SHOULD_BE_POSITIVE;
	ne = int(*(dp0++));

	xSt = *(dp0++); // Assuming input in m!
	xFi = *(dp0++); // Assuming input in m!
	nx = int(*(dp0++));
	//if(nx == 1)
	//{
	//	if((!DistrInfoDat.FluxComp) && (!DistrInfoDat.AllowAutoChoiceOfNxNzForPropagat))
	//	{
	//		double xMid = 0.5*(xSt + xFi);
	//		xSt = xMid; xFi = xMid;
	//	}
	//}

	zSt = *(dp0++); // Assuming input in m!
	zFi = *(dp0++); // Assuming input in m!
	nz = int(*(dp0++));
	//if(nz == 1)
	//{
	//	if((!DistrInfoDat.FluxComp) && (!DistrInfoDat.AllowAutoChoiceOfNxNzForPropagat))
	//	{
	//		double zMid = 0.5*(zSt + zFi);
	//		zSt = zMid; zFi = zMid;
	//	}
	//}
	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetPrecParamWfrSamplingForPropag(waveHndl wavH, bool& AllowAutoChoiceOfNxNzForPropag, double& NxNzOversamplingParam)
{
	int result;

	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp = (DOUBLE*)dataStartPtr;

	char LocAllowAutoChoiceOfNxNzForPropagat = char(*(dp++));
	AllowAutoChoiceOfNxNzForPropag = (LocAllowAutoChoiceOfNxNzForPropagat != 0);
	NxNzOversamplingParam = double(*dp);
	HSetState((Handle)wavH, hState);

	if(!AllowAutoChoiceOfNxNzForPropag) NxNzOversamplingParam = -1;

	return 0;
}

//*************************************************************************

int srTIgorSend::GetPrecParamElectricFieldComp(waveHndl wavH, int& IntegMeth, double& RelPrecOrStep, double& sStart, double& sEnd)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	int result;

	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 2) return BAD_RAD_INT_WAVE_FORMAT;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp0 = (DOUBLE*)dataStartPtr;

	IntegMeth = int(*(dp0++));
	if((IntegMeth < 0) || (IntegMeth > 2)) return BAD_RAD_INT_METH_VALUE;

	RelPrecOrStep = *(dp0++);
	if(RelPrecOrStep < 0.) return BAD_PREC_OR_STEP_VALUE;

	if(numRows > 2)
	{
		sStart = *(dp0++); // Assuming input in m !
		sEnd = *(dp0++); // Assuming input in m !
	}

//	MaxNumPoToSave = long(*(dp0++));
//// Testing
//	TryToApplyNearFieldResidual = true;
//	if(numRows == 6) // Switch for Trying to apply Near-Field residual term if the far-field one fails
//	{
//		TryToApplyNearFieldResidual = (char(*(dp0++)) != 0);
//	}
//// End Testing

	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetPrecParamStokesArbComp(waveHndl wavH, srTParPrecStokesArb* pPrecStokesArb)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);

	long dimensionSizes[MAX_DIMENSIONS+1];
	long dataOffset, numDimensions;
	int result;
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	long numRows = dimensionSizes[0];
	if(numRows < 2) return BAD_RAD_INT_WAVE_FORMAT;

	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp0 = (DOUBLE*)dataStartPtr;

	int IntOrFluxNo = int(*(dp0++));
	if(IntOrFluxNo == 2) pPrecStokesArb->IntOrFlux = 'i';
	else pPrecStokesArb->IntOrFlux = 'f';

    pPrecStokesArb->MethNo = int(*(dp0++));
	if((pPrecStokesArb->MethNo < 0) || (pPrecStokesArb->MethNo > 2)) return BAD_RAD_INT_METH_VALUE;

	pPrecStokesArb->RelPrecOrStep = *(dp0++);
	if(pPrecStokesArb->RelPrecOrStep < 0.) return BAD_PREC_OR_STEP_VALUE;

	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetSRWRadInData(waveHndl wavH, srTSRWRadInData* pSRWRadInData)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return IMPROPER_RADIATION_STRUCTURE;

	pSRWRadInData->wRad = wavH;

	int result;
	long dataOffset, numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	long RadIndices[MAX_DIMENSIONS];

// Hor. E data
	Handle textH = NewHandle(0L);
	RadIndices[0] = 0;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wRadX = FetchWave(*textH);
	if(wRadX == NIL) return IMPROPER_RADIATION_STRUCTURE;
	pSRWRadInData->wRadX = wRadX;
	if(result = MDAccessNumericWaveData(wRadX, kMDWaveAccessMode0, &dataOffset)) return result;
	pSRWRadInData->hStateRadX = MoveLockHandle(wRadX);
	char* dataStartPtr = (char*)(*wRadX) + dataOffset;
	pSRWRadInData->pBaseRadX = (float*)dataStartPtr;
	if(result = MDGetWaveDimensions(wRadX, &numDimensions, dimensionSizes)) return result;
	if(numDimensions != 3) return IMPROPER_RADIATION_STRUCTURE;
	pSRWRadInData->ne = dimensionSizes[0];
	pSRWRadInData->nx = dimensionSizes[1];
	pSRWRadInData->nz = dimensionSizes[2];
	DOUBLE eStep, eStart, xStep, xStart, zStep, zStart;
	if(result = MDGetWaveScaling(wRadX, 0, &eStep, &eStart)) return result;
	if(result = MDGetWaveScaling(wRadX, 1, &xStep, &xStart)) return result;
	if(result = MDGetWaveScaling(wRadX, 2, &zStep, &zStart)) return result;
	pSRWRadInData->eStep = eStep; pSRWRadInData->eStart = eStart;
	pSRWRadInData->xStep = xStep; pSRWRadInData->xStart = xStart;
	pSRWRadInData->zStep = zStep; pSRWRadInData->zStart = zStart;
	DisposeHandle(textH);

// Vert. E data
	textH = NewHandle(0L);
	RadIndices[0] = 1;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wRadZ = FetchWave(*textH);
	if(wRadZ == NIL) return IMPROPER_RADIATION_STRUCTURE;
	pSRWRadInData->wRadZ = wRadZ;
	if(result = MDAccessNumericWaveData(wRadZ, kMDWaveAccessMode0, &dataOffset)) return result;
	pSRWRadInData->hStateRadZ = MoveLockHandle(wRadZ);
	dataStartPtr = (char*)(*wRadZ) + dataOffset;
	pSRWRadInData->pBaseRadZ = (float*)dataStartPtr;
	if(result = MDGetWaveDimensions(wRadZ, &numDimensions, dimensionSizes)) return result;
	if(numDimensions != 3) return IMPROPER_RADIATION_STRUCTURE;
	if(pSRWRadInData->ne != dimensionSizes[0]) return IMPROPER_RADIATION_STRUCTURE;
	if(pSRWRadInData->nx != dimensionSizes[1]) return IMPROPER_RADIATION_STRUCTURE;
	if(pSRWRadInData->nz != dimensionSizes[2]) return IMPROPER_RADIATION_STRUCTURE;
	if(result = MDGetWaveScaling(wRadZ, 0, &eStep, &eStart)) return result;
	if(result = MDGetWaveScaling(wRadZ, 1, &xStep, &xStart)) return result;
	if(result = MDGetWaveScaling(wRadZ, 2, &zStep, &zStart)) return result;
	if((pSRWRadInData->eStep != eStep) || (pSRWRadInData->eStart != eStart)) return IMPROPER_RADIATION_STRUCTURE;
	if((pSRWRadInData->xStep != xStep) || (pSRWRadInData->xStart != xStart)) return IMPROPER_RADIATION_STRUCTURE;
	if((pSRWRadInData->zStep != zStep) || (pSRWRadInData->zStart != zStart)) return IMPROPER_RADIATION_STRUCTURE;
	DisposeHandle(textH);

// Presentation of rad.
	textH = NewHandle(0L);
	RadIndices[0] = 2;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(**textH == '0') pSRWRadInData->Pres = 0;
	else if(**textH == '1') pSRWRadInData->Pres = 1;
	else return IMPROPER_RADIATION_STRUCTURE;

	pSRWRadInData->LengthUnit = 0; // Length is in m
	pSRWRadInData->PhotEnergyUnit = 0; // Photon energy is in eV

// Electron Beam or Trajectory
	textH = NewHandle(0L);
	RadIndices[0] = 12;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wElecBeamOrTrj = FetchWave(*textH);
	if(wElecBeamOrTrj == NIL) return IMPROPER_RADIATION_STRUCTURE;

	int ElecBeamOrTrjWaveType = WaveType(wElecBeamOrTrj);
	if(ElecBeamOrTrjWaveType == TEXT_WAVE_TYPE) // Assume Trajectory, later modify if necessary
	{
		pSRWRadInData->wTrj = wElecBeamOrTrj;
		pSRWRadInData->hStateTrj = MoveLockHandle(wElecBeamOrTrj);
	}
	else // Assume Trajectory, later modify if necessary
	{
		pSRWRadInData->wElecBeam = wElecBeamOrTrj;
		if(result = MDAccessNumericWaveData(pSRWRadInData->wElecBeam, kMDWaveAccessMode0, &dataOffset)) return result;
		pSRWRadInData->hStateElecBeam = MoveLockHandle(pSRWRadInData->wElecBeam);
		dataStartPtr = (char*)(*(pSRWRadInData->wElecBeam)) + dataOffset;
		pSRWRadInData->pElecBeam = (DOUBLE*)dataStartPtr;
		DisposeHandle(textH);
	}

// 4x4 Matrix for e-beam Moments propagation
	textH = NewHandle(0L);
	RadIndices[0] = 13;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl w4x4PropMatr = FetchWave(*textH);
	if(w4x4PropMatr == NIL) return IMPROPER_RADIATION_STRUCTURE;
	pSRWRadInData->w4x4PropMatr = w4x4PropMatr;
	if(result = MDAccessNumericWaveData(w4x4PropMatr, kMDWaveAccessMode0, &dataOffset)) return result;
	pSRWRadInData->hState4x4PropMatr = MoveLockHandle(w4x4PropMatr);
	dataStartPtr = (char*)(*w4x4PropMatr) + dataOffset;
	pSRWRadInData->p4x4PropMatr = (DOUBLE*)dataStartPtr;
	DisposeHandle(textH);

// Moments of Hor. polarization rad.
	textH = NewHandle(0L);
	RadIndices[0] = 15;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wRadMomX = FetchWave(*textH);
	if(wRadMomX == NIL) return IMPROPER_RADIATION_STRUCTURE;
	pSRWRadInData->wMomX = wRadMomX;
	if(result = MDAccessNumericWaveData(wRadMomX, kMDWaveAccessMode0, &dataOffset)) return result;
	pSRWRadInData->hStateMomX = MoveLockHandle(wRadMomX);
	dataStartPtr = (char*)(*wRadMomX) + dataOffset;
	pSRWRadInData->pMomX = (float*)dataStartPtr;
	DisposeHandle(textH);

// Moments of Vert. polarization rad.
	textH = NewHandle(0L);
	RadIndices[0] = 16;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wRadMomZ = FetchWave(*textH);
	if(wRadMomZ == NIL) return IMPROPER_RADIATION_STRUCTURE;
	pSRWRadInData->wMomZ = wRadMomZ;
	if(result = MDAccessNumericWaveData(wRadMomZ, kMDWaveAccessMode0, &dataOffset)) return result;
	pSRWRadInData->hStateMomZ = MoveLockHandle(wRadMomZ);
	dataStartPtr = (char*)(*wRadMomZ) + dataOffset;
	pSRWRadInData->pMomZ = (float*)dataStartPtr;
	DisposeHandle(textH);

// Auxiliary Wave Front Data
	textH = NewHandle(0L);
	RadIndices[0] = 18;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;
	waveHndl wWfrAuxData = FetchWave(*textH);
	if(wWfrAuxData == NIL) return IMPROPER_RADIATION_STRUCTURE;
	pSRWRadInData->wWfrAuxData = wWfrAuxData;
	if(result = MDAccessNumericWaveData(wWfrAuxData, kMDWaveAccessMode0, &dataOffset)) return result;
	pSRWRadInData->hStateWfrAuxData = MoveLockHandle(wWfrAuxData);
	dataStartPtr = (char*)(*wWfrAuxData) + dataOffset;
	pSRWRadInData->pWfrAuxData = (DOUBLE*)dataStartPtr;
	DisposeHandle(textH);

	SetupSrwWfrAuxData(pSRWRadInData);

// Electric Field Units (to allow arb. units)
	textH = NewHandle(0L);
	RadIndices[0] = 19;
	if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
	if(**textH == '0') pSRWRadInData->ElecFldUnit = 0;
	else if(**textH == '1') pSRWRadInData->ElecFldUnit = 1;
	else return IMPROPER_RADIATION_STRUCTURE;

// Read any new elements of Rad here !!!
	return 0;
}

//*************************************************************************

int srTIgorSend::GetSRWStokesInData(waveHndl wavH, srTSRWStokesInData* pStokesAccessData)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP32) return IMPROPER_STOKES_STRUCTURE;

	int result;
  	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	if(numDimensions != 4) return IMPROPER_STOKES_STRUCTURE;
	if(dimensionSizes[0] != 4) return IMPROPER_STOKES_STRUCTURE;

	pStokesAccessData->wSto = wavH;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	pStokesAccessData->hStateSto = MoveLockHandle(wavH);

	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	pStokesAccessData->pBaseSto = (float*)dataStartPtr;

	pStokesAccessData->ne = dimensionSizes[1];
	pStokesAccessData->nx = dimensionSizes[2];
	pStokesAccessData->nz = dimensionSizes[3];

	DOUBLE eStep, eStart, xStep, xStart, zStep, zStart;
	if(result = MDGetWaveScaling(wavH, 1, &eStep, &eStart)) return result;
	if(result = MDGetWaveScaling(wavH, 2, &xStep, &xStart)) return result;
	if(result = MDGetWaveScaling(wavH, 3, &zStep, &zStart)) return result;
	pStokesAccessData->eStep = eStep; pStokesAccessData->eStart = eStart;
	pStokesAccessData->xStep = xStep; pStokesAccessData->xStart = xStart;
	pStokesAccessData->zStep = zStep; pStokesAccessData->zStart = zStart;
	return 0;
}

//*************************************************************************

int srTIgorSend::GetSRWPowDensInData(waveHndl wavH, srTSRWPowDensInData* pPowDensStructAccessData)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP32) return IMPROPER_POWER_DENSITY_STRUCTURE;

	int result;
  	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	if(numDimensions != 2) return IMPROPER_POWER_DENSITY_STRUCTURE;

	pPowDensStructAccessData->wPowDens = wavH;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	pPowDensStructAccessData->hStatePowDens = MoveLockHandle(wavH);

	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	pPowDensStructAccessData->pBasePowDens = (float*)dataStartPtr;

	pPowDensStructAccessData->nx = dimensionSizes[0];
	pPowDensStructAccessData->nz = dimensionSizes[1];

	DOUBLE xStep, xStart, zStep, zStart;
	if(result = MDGetWaveScaling(wavH, 0, &xStep, &xStart)) return result;
	if(result = MDGetWaveScaling(wavH, 1, &zStep, &zStart)) return result;

	pPowDensStructAccessData->xStep = xStep; pPowDensStructAccessData->xStart = xStart;
	pPowDensStructAccessData->zStep = zStep; pPowDensStructAccessData->zStart = zStart;
	return 0;
}

//*************************************************************************

int srTIgorSend::GetIntensExtractParam(waveHndl wExtractParam, int& PolarizCompon, int& Int_or_Phase, int& PlotType, int& TransvPres, double& ePh, double& x, double& z)
{
	int result = 0;
	long dataOffset, numDimensions, dimensionSizes[MAX_DIMENSIONS+1];
	int waveType = WaveType(wExtractParam);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	if(result = MDGetWaveDimensions(wExtractParam, &numDimensions, dimensionSizes)) return result;
	if(numDimensions != 1) return NEEDS_1D_WAVE;

	if(result = MDAccessNumericWaveData(wExtractParam, kMDWaveAccessMode0, &dataOffset)) return result;
	int hStateExtractParam = MoveLockHandle(wExtractParam);
	char* dataStartPtr = (char*)(*wExtractParam) + dataOffset;

	DOUBLE* pD0 = (DOUBLE*)(dataStartPtr);
	DOUBLE* pD = pD0;
	PolarizCompon = int(*(pD++));
	Int_or_Phase = int(*(pD++));
	PlotType = int(*(pD++));
	TransvPres = int(*(pD++));
	pD = pD0 + 10;
	ePh = *(pD++);
	x = *(pD++);
	z = *pD;

	HSetState((Handle)(wExtractParam), hStateExtractParam);
	WaveHandleModified(wExtractParam);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetIntensExtractData(waveHndl wExtractedData, int& hStateExtractedData, char*& pExtractedData)
{
	int result = 0;
	int waveType = WaveType(wExtractedData);
	if((waveType != NT_FP32) && (waveType != NT_FP64)) return NT_FP32_WAVE_REQUIRED;

	long dataOffset;
	if(result = MDAccessNumericWaveData(wExtractedData, kMDWaveAccessMode0, &dataOffset)) return result;
	hStateExtractedData = MoveLockHandle(wExtractedData);
	pExtractedData = (char*)(*wExtractedData) + dataOffset;
	return 0;
}

//*************************************************************************

int srTIgorSend::GetVectorOfStrings(waveHndl wavH, vector<string>* pStringVect)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return TEXT_WAVE_REQUIRED;

	int result;
	long numDimensions;
	long dimensionSizes[MAX_DIMENSIONS+1];
	if(result = MDGetWaveDimensions(wavH, &numDimensions, dimensionSizes)) return result;
	if(numDimensions > 1) return TEXT_WAVE_1D_REQUIRED;

	int AmOfRecords = int(*dimensionSizes);
	long RadIndices[MAX_DIMENSIONS];

	for(int k=0; k<AmOfRecords; k++)
	{
		Handle textH = NewHandle(0L);
		*RadIndices = k;
		if(result = MDGetTextWavePointValue(wavH, RadIndices, textH)) return result;
		if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;

		char* aString = new char[256];
		if(aString == 0) return MEMORY_ALLOCATION_FAILURE;
		char *tString = aString, *tH = *textH;

		for(int i=0; i<255; i++) 
		{
			*(tString++) = *tH;
			if(*(tH++) == '\0') break;
		}

		string stlString(aString);
		pStringVect->push_back(stlString);

		DisposeHandle(textH);
	}

	WaveHandleModified(wavH);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetVectorOfStrings(const char* StructName, vector<string>* pStringVect)
{
	waveHndl wavH = FetchWave(StructName);
	return GetVectorOfStrings(wavH, pStringVect);
}

//*************************************************************************

int srTIgorSend::WfrModify(int ActionNo, srTSRWRadInData* pNewRadStructAccessData, char PolarizComp)
{
	if(pNewRadStructAccessData == 0) return INCORRECT_WAVEFRONT_STRUCTURE;

	if(ActionNo == 0) return WfrDelete(pNewRadStructAccessData);
	else if(ActionNo == 1) return WfrCreateNew(pNewRadStructAccessData);
	else if(ActionNo == 2) return WfrModifyNeNxNz(pNewRadStructAccessData, PolarizComp);
	else if(ActionNo == 3) return WfrRename(pNewRadStructAccessData);
	else if(ActionNo == 4) return WfrGetNames(pNewRadStructAccessData);

	return 0;
}

//*************************************************************************

int srTIgorSend::WfrDelete(srTSRWRadInData* pNewRadStructAccessData)
{
	if(pNewRadStructAccessData == 0) return INCORRECT_WAVEFRONT_STRUCTURE;
	srTSRWRadInData& RadData = *pNewRadStructAccessData;

	int result;
	if(RadData.wRadX_ && (RadData.wRadX != NIL))
	{
		HSetState((Handle)(RadData.wRadX), RadData.hStateRadX);
		if(result = KillWave(RadData.wRadX)) return result;
	}
	if(RadData.wRadZ_ && (RadData.wRadZ != NIL))
	{
		HSetState((Handle)(RadData.wRadZ), RadData.hStateRadZ);
		if(result = KillWave(RadData.wRadZ)) return result;
	}
	if(RadData.wRad_ && (RadData.wRad != NIL))
	{
		if(result = KillWave(RadData.wRad)) return result;
	}
	if(RadData.wElecBeam_ && (RadData.wElecBeam != NIL))
	{
		HSetState((Handle)(RadData.wElecBeam), RadData.hStateElecBeam);
		if(result = KillWave(RadData.wElecBeam)) return result;
	}
	if(RadData.wTrj_ && (RadData.wTrj != NIL))
	{
		HSetState((Handle)(RadData.wTrj), RadData.hStateTrj);
		if(result = KillWave(RadData.wTrj)) return result;
	}
	if(RadData.w4x4PropMatr_ && (RadData.w4x4PropMatr != NIL))
	{
		HSetState((Handle)(RadData.w4x4PropMatr), RadData.hState4x4PropMatr);
		if(result = KillWave(RadData.w4x4PropMatr)) return result;
	}
	if(RadData.wMomX_ && (RadData.wMomX != NIL))
	{
		HSetState((Handle)(RadData.wMomX), RadData.hStateMomX);
		if(result = KillWave(RadData.wMomX)) return result;
	}
	if(RadData.wMomZ_ && (RadData.wMomZ != NIL))
	{
		HSetState((Handle)(RadData.wMomZ), RadData.hStateMomZ);
		if(result = KillWave(RadData.wMomZ)) return result;
	}
	if(RadData.wWfrAuxData_ && (RadData.wWfrAuxData != NIL))
	{
		HSetState((Handle)(RadData.wWfrAuxData), RadData.hStateWfrAuxData);
		if(result = KillWave(RadData.wWfrAuxData)) return result;
	}
	return 0;
}

//*************************************************************************

int srTIgorSend::WfrRename(srTSRWRadInData* pNewRadStructAccessData)
{
	if(pNewRadStructAccessData == 0) return INCORRECT_WAVEFRONT_STRUCTURE;
	srTSRWRadInData& RadAccessData = *pNewRadStructAccessData;

	int result;
	char AuxWaveName[MAX_OBJ_NAME+1], AuxNewWaveName[MAX_OBJ_NAME+1];
	char AuxRadExtens[6], AuxRadFieldExtens[6];
	const int RadExtensLen = 4;
	const int RadFieldExtensLen = 5;
	int i;

	DataFolderHandle dataFolderH;
	if(result = GetWavesDataFolder(RadAccessData.wRad, &dataFolderH)) return result;

	if(RadAccessData.NameRad == 0) return INCORRECT_WAVEFRONT_STRUCTURE;
	int LenNewName = strlen(RadAccessData.NameRad);
	if(LenNewName == 0) return INCORRECT_WAVEFRONT_STRUCTURE;

//Text Wave
	WaveName(RadAccessData.wRad, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	int OldNameLen = strlen(AuxWaveName);
	char *tAuxWaveName = AuxWaveName + (OldNameLen - RadExtensLen);
	for(i=0; i<RadExtensLen; i++) AuxRadExtens[i] = *(tAuxWaveName++);
	AuxRadExtens[RadExtensLen] = '\0';

	char NewName[MAX_OBJ_NAME+1];
	strcpy(NewName, RadAccessData.NameRad);
	if(LenNewName > RadExtensLen)
	{//remove extension, if any
		int OffsetExtens = LenNewName - RadExtensLen;
		char* pTestExtens = NewName + OffsetExtens;
		if(strcmp(pTestExtens, AuxRadExtens) == 0) NewName[OffsetExtens] = '\0';
	}

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;

//Num. Wave Ex
	WaveName(RadAccessData.wRadX, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	OldNameLen = strlen(AuxWaveName);
	tAuxWaveName = AuxWaveName + (OldNameLen - RadFieldExtensLen);
	for(i=0; i<RadFieldExtensLen; i++) AuxRadFieldExtens[i] = *(tAuxWaveName++);
	AuxRadFieldExtens[RadFieldExtensLen] = '\0';

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadFieldExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;

//Num. Wave Ez
	WaveName(RadAccessData.wRadZ, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	OldNameLen = strlen(AuxWaveName);
	tAuxWaveName = AuxWaveName + (OldNameLen - RadFieldExtensLen);
	for(i=0; i<RadFieldExtensLen; i++) AuxRadFieldExtens[i] = *(tAuxWaveName++);
	AuxRadFieldExtens[RadFieldExtensLen] = '\0';

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadFieldExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;

//Electron Beam or Trajectory
	waveHndl wElecOrTraj = (RadAccessData.wElecBeam != NIL)? RadAccessData.wElecBeam : RadAccessData.wTrj;
	WaveName(wElecOrTraj, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	OldNameLen = strlen(AuxWaveName);
	tAuxWaveName = AuxWaveName + (OldNameLen - RadExtensLen);
	for(i=0; i<RadExtensLen; i++) AuxRadExtens[i] = *(tAuxWaveName++);
	AuxRadExtens[RadExtensLen] = '\0';

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;

//4x4PropMatr
	WaveName(RadAccessData.w4x4PropMatr, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	OldNameLen = strlen(AuxWaveName);
	tAuxWaveName = AuxWaveName + (OldNameLen - RadExtensLen);
	for(i=0; i<RadExtensLen; i++) AuxRadExtens[i] = *(tAuxWaveName++);
	AuxRadExtens[RadExtensLen] = '\0';

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;

//MomX
	WaveName(RadAccessData.wMomX, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	OldNameLen = strlen(AuxWaveName);
	tAuxWaveName = AuxWaveName + (OldNameLen - RadFieldExtensLen);
	for(i=0; i<RadFieldExtensLen; i++) AuxRadFieldExtens[i] = *(tAuxWaveName++);
	AuxRadFieldExtens[RadFieldExtensLen] = '\0';

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadFieldExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;

//MomZ
	WaveName(RadAccessData.wMomZ, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	OldNameLen = strlen(AuxWaveName);
	tAuxWaveName = AuxWaveName + (OldNameLen - RadFieldExtensLen);
	for(i=0; i<RadFieldExtensLen; i++) AuxRadFieldExtens[i] = *(tAuxWaveName++);
	AuxRadFieldExtens[RadFieldExtensLen] = '\0';

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadFieldExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;

//WfrAuxData
	WaveName(RadAccessData.wWfrAuxData, AuxWaveName);
	if(*AuxWaveName == '\0') return IMPROPER_RADIATION_STRUCTURE;

	OldNameLen = strlen(AuxWaveName);
	tAuxWaveName = AuxWaveName + (OldNameLen - RadExtensLen);
	for(i=0; i<RadExtensLen; i++) AuxRadExtens[i] = *(tAuxWaveName++);
	AuxRadExtens[RadExtensLen] = '\0';

	strcpy(AuxNewWaveName, NewName);
	strcat(AuxNewWaveName, AuxRadExtens);
	if(result = RenameDataFolderObject(dataFolderH, WAVE_OBJECT, AuxWaveName, AuxNewWaveName)) return result;
	
	return 0;
}

//*************************************************************************

int srTIgorSend::WfrGetNames(srTSRWRadInData* pNewRadStructAccessData)
{
	if(pNewRadStructAccessData == 0) return INCORRECT_WAVEFRONT_STRUCTURE;
	srTSRWRadInData& RadAccessData = *pNewRadStructAccessData;
	srTSRWRadInData& Names = *pNewRadStructAccessData;

	WaveName(RadAccessData.wRad, Names.NameRad);
	if(*(Names.NameRad) == '\0') return IMPROPER_RADIATION_STRUCTURE;

	WaveName(RadAccessData.wRadX, Names.NameRadX);
	if(*(Names.NameRadX) == '\0') return IMPROPER_RADIATION_STRUCTURE;
	WaveName(RadAccessData.wRadZ, Names.NameRadZ);
	if(*(Names.NameRadZ) == '\0') return IMPROPER_RADIATION_STRUCTURE;

	if(RadAccessData.wElecBeam != NIL)
	{
		WaveName(RadAccessData.wElecBeam, Names.NameElecBeam);
		if(*(Names.NameElecBeam) == '\0') return IMPROPER_RADIATION_STRUCTURE;
	}
	if(RadAccessData.wTrj != NIL)
	{
		WaveName(RadAccessData.wTrj, Names.NameTrj);
		if(*(Names.NameTrj) == '\0') return IMPROPER_RADIATION_STRUCTURE;
	}

	WaveName(RadAccessData.w4x4PropMatr, Names.Name4x4PropMatr);
	if(*(Names.Name4x4PropMatr) == '\0') return IMPROPER_RADIATION_STRUCTURE;

	WaveName(RadAccessData.wMomX, Names.NameMomX);
	if(*(Names.NameMomX) == '\0') return IMPROPER_RADIATION_STRUCTURE;
	WaveName(RadAccessData.wMomZ, Names.NameMomZ);
	if(*(Names.NameMomZ) == '\0') return IMPROPER_RADIATION_STRUCTURE;

	WaveName(RadAccessData.wWfrAuxData, Names.NameWfrAuxData);
	if(*(Names.NameWfrAuxData) == '\0') return IMPROPER_RADIATION_STRUCTURE;

	return 0;
}

//*************************************************************************

int srTIgorSend::WfrCreateNew(srTSRWRadInData* pNewRadStructAccessData)
{
	if(pNewRadStructAccessData == 0) return INCORRECT_WAVEFRONT_STRUCTURE;
	srTSRWRadInData& RadData = *pNewRadStructAccessData;

//- Rad. text wave
//- Ex wave
//- Ez wave
//Program more when mecessary !!!
	int result;

	long RadIndices[MAX_DIMENSIONS];
	long dimensionSizes[MAX_DIMENSIONS+1];
	long dataOffset;
	char CharBuf[256];
	int AmOfBytes;

//Text Wave
	if((RadData.wRad == NIL) && (*(RadData.NameRad) != '\0'))
	{
		for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
		dimensionSizes[0] = 22; // Don't forget to increase when needed !!!
		if(result = MDMakeWave(&(RadData.wRad), RadData.NameRad, NIL, dimensionSizes, TEXT_WAVE_TYPE, 1)) return result;
	}

	RadIndices[0] = 2; // Presentation
	AmOfBytes = sprintf(CharBuf, "%d", char(RadData.Pres));
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;

	DOUBLE eStep = RadData.eStep, eStart = RadData.eStart;
	RadIndices[0] = 3; // eStep
	AmOfBytes = sprintf(CharBuf, "%g", eStep);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 4; // eStart
	AmOfBytes = sprintf(CharBuf, "%g", eStart);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;

	DOUBLE xStep = RadData.xStep, xStart = RadData.xStart;
	DOUBLE zStep = RadData.zStep, zStart = RadData.zStart;
	DOUBLE RobsX = RadData.RobsX, RobsZ = RadData.RobsZ;
	DOUBLE RobsXAbsErr = RadData.RobsXAbsErr, RobsZAbsErr = RadData.RobsZAbsErr;
	DOUBLE xc = RadData.xc, zc = RadData.zc;

	RadIndices[0] = 5; // xStep
	AmOfBytes = sprintf(CharBuf, "%g", xStep);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 6; // xStart
	AmOfBytes = sprintf(CharBuf, "%g", xStart);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 7; // zStep
	AmOfBytes = sprintf(CharBuf, "%g", zStep);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 8; // zStart
	AmOfBytes = sprintf(CharBuf, "%g", zStart);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 9; // Quality of Propag.
	AmOfBytes = sprintf(CharBuf, "%i", 1);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 14; // Linearity of Transf.
	AmOfBytes = sprintf(CharBuf, "%i", 1);
	if(result = UpdateNumberPositionInSRWRad(&RadData, RadIndices, CharBuf, AmOfBytes)) return result;

	char TransvUnits[MAX_UNIT_CHARS + 1];
	if(RadData.Pres == 0) *TransvUnits = 'm';
	else *TransvUnits = 'q';
	TransvUnits[1] = '\0';

//Ex
	if((RadData.wRadX == NIL) && (*(RadData.NameRadX) != '\0'))
	{
		for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
		dimensionSizes[0] = RadData.ne;
		dimensionSizes[1] = RadData.nx;
		dimensionSizes[2] = RadData.nz;

		if(result = MDMakeWave(&(RadData.wRadX), RadData.NameRadX, NIL, dimensionSizes, (NT_FP32 | NT_CMPLX), 1)) return result;

		if(result = MDSetWaveUnits(RadData.wRadX, 0, "eV")) return result;
		if(result = MDSetWaveUnits(RadData.wRadX, 1, "m")) return result;
		if(result = MDSetWaveUnits(RadData.wRadX, 2, "m")) return result;

		if(result = MDSetWaveScaling(RadData.wRadX, 0, &eStep, &eStart)) return result;
		if(result = MDSetWaveScaling(RadData.wRadX, 1, &xStep, &xStart)) return result;
		if(result = MDSetWaveScaling(RadData.wRadX, 2, &zStep, &zStart)) return result;

		if(result = MDAccessNumericWaveData(RadData.wRadX, kMDWaveAccessMode0, &dataOffset)) return result;
		RadData.hStateRadX = MoveLockHandle(RadData.wRadX);
		RadData.pBaseRadX = (float*)((char*)(*(RadData.wRadX)) + dataOffset);
	}
	if(result = UpdateTextPositionInSRWRad(&RadData, 0, RadData.NameRadX)) return result;

//Ez
	if((RadData.wRadZ == NIL) && (*(RadData.NameRadZ) != '\0'))
	{
		for(int i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
		dimensionSizes[0] = RadData.ne;
		dimensionSizes[1] = RadData.nx;
		dimensionSizes[2] = RadData.nz;

		if(result = MDMakeWave(&(RadData.wRadZ), RadData.NameRadZ, NIL, dimensionSizes, (NT_FP32 | NT_CMPLX), 1)) return result;

		if(result = MDSetWaveUnits(RadData.wRadZ, 0, "eV")) return result;
		if(result = MDSetWaveUnits(RadData.wRadZ, 1, "m")) return result;
		if(result = MDSetWaveUnits(RadData.wRadZ, 2, "m")) return result;

		if(result = MDSetWaveScaling(RadData.wRadZ, 0, &eStep, &eStart)) return result;
		if(result = MDSetWaveScaling(RadData.wRadZ, 1, &xStep, &xStart)) return result;
		if(result = MDSetWaveScaling(RadData.wRadZ, 2, &zStep, &zStart)) return result;

		if(result = MDAccessNumericWaveData(RadData.wRadZ, kMDWaveAccessMode0, &dataOffset)) return result;
		RadData.hStateRadZ = MoveLockHandle(RadData.wRadZ);
		RadData.pBaseRadZ = (float*)((char*)(*(RadData.wRadZ)) + dataOffset);
	}
	if(result = UpdateTextPositionInSRWRad(&RadData, 1, RadData.NameRadZ)) return result;
	
//E-beam
	if((RadData.wElecBeam == NIL) && (*(RadData.NameElecBeam) != '\0'))
	{
		// Program when necessary
	}
	if(*(RadData.NameElecBeam) != '\0')
	{
		if(result = UpdateTextPositionInSRWRad(&RadData, 12, RadData.NameElecBeam)) return result;
	}

// Trajectory
	if((RadData.wTrj == NIL) && (*(RadData.NameTrj) != '\0'))
	{
		// Program when necessary
	}
	if(*(RadData.NameTrj) != '\0')
	{
		if(result = UpdateTextPositionInSRWRad(&RadData, 12, RadData.NameTrj)) return result;
	}

// 4x4 Matrix for e-beam Moments propagation
	if((RadData.w4x4PropMatr == NIL) && (*(RadData.Name4x4PropMatr) != '\0'))
	{
		// Program when necessary
	}
	if(result = UpdateTextPositionInSRWRad(&RadData, 13, RadData.Name4x4PropMatr)) return result;

// Radiation Moments
	if((RadData.wMomX == NIL) && (*(RadData.NameMomX) != '\0'))
	{
		// Program when necessary
	}
	if(result = UpdateTextPositionInSRWRad(&RadData, 15, RadData.NameMomX)) return result;

	if((RadData.wMomZ == NIL) && (*(RadData.NameMomZ) != '\0'))
	{
		// Program when necessary
	}
	if(result = UpdateTextPositionInSRWRad(&RadData, 16, RadData.NameMomZ)) return result;

// Auxiliary Wave Front Data
	if((RadData.wWfrAuxData == NIL) && (*(RadData.NameWfrAuxData) != '\0'))
	{
		// Program when necessary
	}
	if(result = UpdateTextPositionInSRWRad(&RadData, 18, RadData.NameWfrAuxData)) return result;

// Add here treatment of new Rad data members, if any.

	WaveHandleModified(RadData.wRad);

	return 0;
}

//*************************************************************************

int srTIgorSend::WfrModifyNeNxNz(srTSRWRadInData* pNewRadStructAccessData, char PolarizComp)
{
	char TreatPolCompX = ((PolarizComp == 0) || (PolarizComp == 'x'));
	char TreatPolCompZ = ((PolarizComp == 0) || (PolarizComp == 'z'));

	long dimensionSizes[MAX_DIMENSIONS+1];
	int i;
	for(i=0; i<=MAX_DIMENSIONS; i++) dimensionSizes[i] = 0;
	dimensionSizes[0] = pNewRadStructAccessData->ne;
	dimensionSizes[1] = pNewRadStructAccessData->nx;
	dimensionSizes[2] = pNewRadStructAccessData->nz;

	DOUBLE StepE = pNewRadStructAccessData->eStep, StepX = pNewRadStructAccessData->xStep, StepZ = pNewRadStructAccessData->zStep;
	DOUBLE StartE = pNewRadStructAccessData->eStart, StartX = pNewRadStructAccessData->xStart, StartZ = pNewRadStructAccessData->zStart;

	char AuxRadWaveName[MAX_OBJ_NAME+1];
	int result;
	long dataOffset;

	if(TreatPolCompX)
	{
		HSetState((Handle)(pNewRadStructAccessData->wRadX), pNewRadStructAccessData->hStateRadX);

		WaveName(pNewRadStructAccessData->wRadX, AuxRadWaveName);
		result = KillWave(pNewRadStructAccessData->wRadX);
		if(result = MDMakeWave(&(pNewRadStructAccessData->wRadX), AuxRadWaveName, NIL, dimensionSizes, (NT_FP32 | NT_CMPLX), 1)) return result;

		if(result = MDSetWaveUnits(pNewRadStructAccessData->wRadX, 0, "eV")) return result;
		if(result = MDSetWaveUnits(pNewRadStructAccessData->wRadX, 1, "m")) return result;
		if(result = MDSetWaveUnits(pNewRadStructAccessData->wRadX, 2, "m")) return result;

		if(result = MDSetWaveScaling(pNewRadStructAccessData->wRadX, 0, &StepE, &StartE)) return result;
		if(result = MDSetWaveScaling(pNewRadStructAccessData->wRadX, 1, &StepX, &StartX)) return result;
		if(result = MDSetWaveScaling(pNewRadStructAccessData->wRadX, 2, &StepZ, &StartZ)) return result;

		if(result = MDAccessNumericWaveData(pNewRadStructAccessData->wRadX, kMDWaveAccessMode0, &dataOffset)) return result;
		pNewRadStructAccessData->hStateRadX = MoveLockHandle(pNewRadStructAccessData->wRadX);
		pNewRadStructAccessData->pBaseRadX = (float*)((char*)(*(pNewRadStructAccessData->wRadX)) + dataOffset);
	}
	if(TreatPolCompZ)
	{
		HSetState((Handle)(pNewRadStructAccessData->wRadZ), pNewRadStructAccessData->hStateRadZ);

		WaveName(pNewRadStructAccessData->wRadZ, AuxRadWaveName);
		result = KillWave(pNewRadStructAccessData->wRadZ);
		if(result = MDMakeWave(&(pNewRadStructAccessData->wRadZ), AuxRadWaveName, NIL, dimensionSizes, (NT_FP32 | NT_CMPLX), 1)) return result;

		if(result = MDSetWaveUnits(pNewRadStructAccessData->wRadZ, 0, "eV")) return result;
		if(result = MDSetWaveUnits(pNewRadStructAccessData->wRadZ, 1, "m")) return result;
		if(result = MDSetWaveUnits(pNewRadStructAccessData->wRadZ, 2, "m")) return result;

		if(result = MDSetWaveScaling(pNewRadStructAccessData->wRadZ, 0, &StepE, &StartE)) return result;
		if(result = MDSetWaveScaling(pNewRadStructAccessData->wRadZ, 1, &StepX, &StartX)) return result;
		if(result = MDSetWaveScaling(pNewRadStructAccessData->wRadZ, 2, &StepZ, &StartZ)) return result;

		if(result = MDAccessNumericWaveData(pNewRadStructAccessData->wRadZ, kMDWaveAccessMode0, &dataOffset)) return result;
		pNewRadStructAccessData->hStateRadZ = MoveLockHandle(pNewRadStructAccessData->wRadZ);
		pNewRadStructAccessData->pBaseRadZ = (float*)((char*)(*(pNewRadStructAccessData->wRadZ)) + dataOffset);
	}

	long RadIndices[MAX_DIMENSIONS];
	int AmOfBytes;
	char CharBuf[15];
	Handle textH;

	RadIndices[0] = 3; // eStep
	for(i=0; i<15; i++) CharBuf[i] = 0;
	AmOfBytes = sprintf(CharBuf, "%g", pNewRadStructAccessData->eStep);
	textH = NewHandle(AmOfBytes);
	strncpy(*textH, CharBuf, AmOfBytes);
	if(result = MDSetTextWavePointValue(pNewRadStructAccessData->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);

	RadIndices[0] = 4; // eStart
	for(i=0; i<15; i++) CharBuf[i] = 0;
	AmOfBytes = sprintf(CharBuf, "%g", pNewRadStructAccessData->eStart);
	textH = NewHandle(AmOfBytes);
	strncpy(*textH, CharBuf, AmOfBytes);
	if(result = MDSetTextWavePointValue(pNewRadStructAccessData->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);

	RadIndices[0] = 5; // xStep
	for(i=0; i<15; i++) CharBuf[i] = 0;
	AmOfBytes = sprintf(CharBuf, "%g", pNewRadStructAccessData->xStep);
	textH = NewHandle(AmOfBytes);
	strncpy(*textH, CharBuf, AmOfBytes);
	if(result = MDSetTextWavePointValue(pNewRadStructAccessData->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);
	
	RadIndices[0] = 6; // xStart
	for(i=0; i<15; i++) CharBuf[i] = 0;
	AmOfBytes = sprintf(CharBuf, "%g", pNewRadStructAccessData->xStart);
	textH = NewHandle(AmOfBytes);
	strncpy(*textH, CharBuf, AmOfBytes);
	if(result = MDSetTextWavePointValue(pNewRadStructAccessData->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);

	RadIndices[0] = 7; // zStep
	for(i=0; i<15; i++) CharBuf[i] = 0;
	AmOfBytes = sprintf(CharBuf, "%g", pNewRadStructAccessData->zStep);
	textH = NewHandle(AmOfBytes);
	strncpy(*textH, CharBuf, AmOfBytes);
	if(result = MDSetTextWavePointValue(pNewRadStructAccessData->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);
	
	RadIndices[0] = 8; // zStart
	for(i=0; i<15; i++) CharBuf[i] = 0;
	AmOfBytes = sprintf(CharBuf, "%g", pNewRadStructAccessData->zStart);
	textH = NewHandle(AmOfBytes);
	strncpy(*textH, CharBuf, AmOfBytes);
	if(result = MDSetTextWavePointValue(pNewRadStructAccessData->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);
	return 0;
}

//*************************************************************************

int srTIgorSend::FinishWorkingWithSRWRadStruct(srTSRWRadInData* pSRWRadStructAccessData)
{
	if(pSRWRadStructAccessData == 0) return 0;
	if((pSRWRadStructAccessData->pBaseRadX == 0) && (pSRWRadStructAccessData->pBaseRadZ == 0)) return 0;

	long RadIndices[MAX_DIMENSIONS];
	int AmOfBytes, i, result;
	char CharBuf[15];
	for(i=0; i<15; i++) CharBuf[i] = 0;

	RadIndices[0] = 2; // Presentation
	AmOfBytes = sprintf(CharBuf, "%d", char(pSRWRadStructAccessData->Pres));
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;

	DOUBLE eStep = pSRWRadStructAccessData->eStep, eStart = pSRWRadStructAccessData->eStart;
	RadIndices[0] = 3; // eStep
	AmOfBytes = sprintf(CharBuf, "%g", eStep);
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 4; // eStart
	AmOfBytes = sprintf(CharBuf, "%g", eStart);
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;

	DOUBLE xStep = pSRWRadStructAccessData->xStep, xStart = pSRWRadStructAccessData->xStart;
	DOUBLE zStep = pSRWRadStructAccessData->zStep, zStart = pSRWRadStructAccessData->zStart;
	DOUBLE RobsX = pSRWRadStructAccessData->RobsX, RobsZ = pSRWRadStructAccessData->RobsZ;
	DOUBLE RobsXAbsErr = pSRWRadStructAccessData->RobsXAbsErr, RobsZAbsErr = pSRWRadStructAccessData->RobsZAbsErr;
	DOUBLE xc = pSRWRadStructAccessData->xc, zc = pSRWRadStructAccessData->zc;

	RadIndices[0] = 5; // xStep
	AmOfBytes = sprintf(CharBuf, "%g", xStep);
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 6; // xStart
	AmOfBytes = sprintf(CharBuf, "%g", xStart);
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 7; // zStep
	AmOfBytes = sprintf(CharBuf, "%g", zStep);
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;
	RadIndices[0] = 8; // zStart
	AmOfBytes = sprintf(CharBuf, "%g", zStart);
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;

	RadIndices[0] = 19; // Elec. fld. units
	AmOfBytes = sprintf(CharBuf, "%d", char(pSRWRadStructAccessData->ElecFldUnit));
	if(result = UpdateNumberPositionInSRWRad(pSRWRadStructAccessData, RadIndices, CharBuf, AmOfBytes)) return result;

	char TransvUnits[MAX_UNIT_CHARS + 1];
	if(pSRWRadStructAccessData->Pres == 0) *TransvUnits = 'm';
	else *TransvUnits = 'q';
	TransvUnits[1] = '\0';

	if(pSRWRadStructAccessData->wRadX != NIL)
	{
		HSetState((Handle)(pSRWRadStructAccessData->wRadX), pSRWRadStructAccessData->hStateRadX);
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wRadX, 0, &eStep, &eStart)) return result;
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wRadX, 1, &xStep, &xStart)) return result;
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wRadX, 2, &zStep, &zStart)) return result;

		if(result = MDSetWaveUnits(pSRWRadStructAccessData->wRadX, COLUMNS, TransvUnits)) return result;
		if(result = MDSetWaveUnits(pSRWRadStructAccessData->wRadX, LAYERS, TransvUnits)) return result;

		WaveHandleModified(pSRWRadStructAccessData->wRadX);
	}

	if(pSRWRadStructAccessData->wRadZ != NIL)
	{
		HSetState((Handle)(pSRWRadStructAccessData->wRadZ), pSRWRadStructAccessData->hStateRadZ);
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wRadZ, 0, &eStep, &eStart)) return result;
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wRadZ, 1, &xStep, &xStart)) return result;
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wRadZ, 2, &zStep, &zStart)) return result;

		if(result = MDSetWaveUnits(pSRWRadStructAccessData->wRadZ, COLUMNS, TransvUnits)) return result;
		if(result = MDSetWaveUnits(pSRWRadStructAccessData->wRadZ, LAYERS, TransvUnits)) return result;

		WaveHandleModified(pSRWRadStructAccessData->wRadZ);
	}
// Don't work with El. field waves after this point.

// E-beam
	if(pSRWRadStructAccessData->wElecBeam != NIL)
	{
		HSetState((Handle)(pSRWRadStructAccessData->wElecBeam), pSRWRadStructAccessData->hStateElecBeam);
		WaveHandleModified(pSRWRadStructAccessData->wElecBeam);
	}

// Trajectory
	if(pSRWRadStructAccessData->wTrj != NIL)
	{
		HSetState((Handle)(pSRWRadStructAccessData->wTrj), pSRWRadStructAccessData->hStateTrj);
		WaveHandleModified(pSRWRadStructAccessData->wTrj);
	}

// 4x4 Matrix for e-beam Moments propagation
	if(pSRWRadStructAccessData->w4x4PropMatr != NIL)
	{
		HSetState((Handle)(pSRWRadStructAccessData->w4x4PropMatr), pSRWRadStructAccessData->hState4x4PropMatr);
		WaveHandleModified(pSRWRadStructAccessData->w4x4PropMatr);
	}

// Radiation Moments
	if(pSRWRadStructAccessData->wMomX != NIL)
	{
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wMomX, 1, &eStep, &eStart)) return result;
		HSetState((Handle)(pSRWRadStructAccessData->wMomX), pSRWRadStructAccessData->hStateMomX);
		WaveHandleModified(pSRWRadStructAccessData->wMomX);
	}
	if(pSRWRadStructAccessData->wMomZ != NIL)
	{
		if(result = MDSetWaveScaling(pSRWRadStructAccessData->wMomZ, 1, &eStep, &eStart)) return result;
		HSetState((Handle)(pSRWRadStructAccessData->wMomZ), pSRWRadStructAccessData->hStateMomZ);
		WaveHandleModified(pSRWRadStructAccessData->wMomZ);
	}

// Auxiliary Wave Front Data
	if(pSRWRadStructAccessData->wWfrAuxData != NIL)
	{
		//pSRWRadStructAccessData->UpdateSrwWfrAuxData();
		UpdateSrwWfrAuxData(pSRWRadStructAccessData);
		
		HSetState((Handle)(pSRWRadStructAccessData->wWfrAuxData), pSRWRadStructAccessData->hStateWfrAuxData);
		WaveHandleModified(pSRWRadStructAccessData->wWfrAuxData);
	}

// Add here treatment of new Rad data members, if any.

	WaveHandleModified(pSRWRadStructAccessData->wRad);
	return 0;
}

//*************************************************************************

int srTIgorSend::FinishWorkingWithSRWStokesStruct(srTSRWStokesInData* pStokesAccessData)
{
	if(pStokesAccessData == 0) return 0;
	HSetState((Handle)(pStokesAccessData->wSto), pStokesAccessData->hStateSto);
	WaveHandleModified(pStokesAccessData->wSto);
	return 0;
}

//*************************************************************************

int srTIgorSend::FinishWorkingWithSRWPowDensStruct(srTSRWPowDensInData* pPowDensAccessData)
{
	if(pPowDensAccessData == 0) return 0;
	HSetState((Handle)(pPowDensAccessData->wPowDens), pPowDensAccessData->hStatePowDens);
	WaveHandleModified(pPowDensAccessData->wPowDens);
	return 0;
}

//*************************************************************************

int srTIgorSend::FinishWorkingWithWave(srTIgorWaveAccessData* pWaveAccessData)
{
	int result = 0;
	if(pWaveAccessData == 0) return 0;
	if(pWaveAccessData->pWaveData == 0) return 0;

	HSetState((Handle)(pWaveAccessData->wHndl), pWaveAccessData->hState);

	if(pWaveAccessData->AmOfDims > -1)
	{
		DOUBLE xStart = pWaveAccessData->DimStartValues[0];
		DOUBLE xStep = pWaveAccessData->DimSteps[0];
		if((xStart < 1.E+23) && (xStep < 1.E+23))
			if(result = MDSetWaveScaling(pWaveAccessData->wHndl, ROWS, &xStep, &xStart)) return result;
		if(*(pWaveAccessData->DimUnits[0]) != '\0')
			if(result = MDSetWaveUnits(pWaveAccessData->wHndl, ROWS, pWaveAccessData->DimUnits[0])) return result;

		if(pWaveAccessData->AmOfDims > 1)
		{
			DOUBLE yStart = pWaveAccessData->DimStartValues[1];
			DOUBLE yStep = pWaveAccessData->DimSteps[1];
			if((yStart < 1.E+23) && (yStep < 1.E+23))
				if(result = MDSetWaveScaling(pWaveAccessData->wHndl, COLUMNS, &yStep, &yStart)) return result;
			if(*(pWaveAccessData->DimUnits[1]) != '\0')
				if(result = MDSetWaveUnits(pWaveAccessData->wHndl, COLUMNS, pWaveAccessData->DimUnits[1])) return result;
		}
		if(pWaveAccessData->AmOfDims > 2)
		{
			DOUBLE zStart = pWaveAccessData->DimStartValues[2];
			DOUBLE zStep = pWaveAccessData->DimSteps[2];
			if((zStart < 1.E+23) && (zStep < 1.E+23))
				if(result = MDSetWaveScaling(pWaveAccessData->wHndl, LAYERS, &zStep, &zStart)) return result;
			if(*(pWaveAccessData->DimUnits[2]) != '\0')
				if(result = MDSetWaveUnits(pWaveAccessData->wHndl, LAYERS, pWaveAccessData->DimUnits[2])) return result;
		}
		if(pWaveAccessData->AmOfDims > 3)
		{
			DOUBLE tStart = pWaveAccessData->DimStartValues[3];
			DOUBLE tStep = pWaveAccessData->DimSteps[3];
			if((tStart < 1.E+23) && (tStep < 1.E+23))
				if(result = MDSetWaveScaling(pWaveAccessData->wHndl, CHUNKS, &tStep, &tStart)) return result;
			if(*(pWaveAccessData->DimUnits[3]) != '\0')
				if(result = MDSetWaveUnits(pWaveAccessData->wHndl, CHUNKS, pWaveAccessData->DimUnits[3])) return result;
		}
	}
	if(*(pWaveAccessData->DataUnits) != '\0')
		if(result = MDSetWaveUnits(pWaveAccessData->wHndl, -1, pWaveAccessData->DataUnits)) return result;

	WaveHandleModified(pWaveAccessData->wHndl);
	pWaveAccessData->pWaveData = 0;
	return 0;
}

//*************************************************************************

int srTIgorSend::UpdateNumberPositionInSRWRad(srTSRWRadInData* pSRWRadStructAccessData, long* RadIndices, char* CharBuf, int AmOfBytes)
{
	char* tCharBuf = CharBuf;

	Handle textH = NewHandle(AmOfBytes);
	strncpy(*textH, CharBuf, AmOfBytes);
	int result;
	if(result = MDSetTextWavePointValue(pSRWRadStructAccessData->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);

	for(int i=0; i<15; i++) *(tCharBuf++) = 0;
	return 0;
}

//*************************************************************************

int srTIgorSend::SetupSrwWfrAuxData(srTSRWRadInData* p)
{
	if(p == 0) return 0;
	DOUBLE *pWfrAuxData = p->pWfrAuxData;

	if(pWfrAuxData == 0) return 0;

	p->RobsX = *pWfrAuxData;
	p->RobsZ = *(pWfrAuxData+1);
	p->RobsXAbsErr = *(pWfrAuxData+2);
	p->RobsZAbsErr = *(pWfrAuxData+3);
	p->xWfrMin = *(pWfrAuxData+4);
	p->xWfrMax = *(pWfrAuxData+5);
	p->zWfrMin = *(pWfrAuxData+6);
	p->zWfrMax = *(pWfrAuxData+7);
	p->UnderSamplingX = *(pWfrAuxData+8);
	p->UnderSamplingZ = *(pWfrAuxData+9);
	p->xc = *(pWfrAuxData+10);
	p->zc = *(pWfrAuxData+11);
	return 0;
}

//*************************************************************************

int srTIgorSend::UpdateSrwWfrAuxData(srTSRWRadInData* p)
{
	if(p == 0) return 0;
	DOUBLE *pWfrAuxData = p->pWfrAuxData;

	if(pWfrAuxData == 0) return 0;

	*pWfrAuxData = p->RobsX;
	*(pWfrAuxData+1) = p->RobsZ;
	*(pWfrAuxData+2) = p->RobsXAbsErr;
	*(pWfrAuxData+3) = p->RobsZAbsErr;
	*(pWfrAuxData+4) = p->xWfrMin;
	*(pWfrAuxData+5) = p->xWfrMax;
	*(pWfrAuxData+6) = p->zWfrMin;
	*(pWfrAuxData+7) = p->zWfrMax;
	*(pWfrAuxData+8) = p->UnderSamplingX;
	*(pWfrAuxData+9) = p->UnderSamplingZ;
	*(pWfrAuxData+10) = p->xc;
	*(pWfrAuxData+11) = p->zc;
	return 0;
}

//*************************************************************************

int srTIgorSend::UpdateTextPositionInSRWRad(srTSRWRadInData* pRad, int Index, char* Text)
{
	int result;
	long RadIndices[MAX_DIMENSIONS]; RadIndices[0] = Index;
	int AmOfBytes = strlen(Text);
	Handle textH = NewHandle(AmOfBytes);
	strncpy(*textH, Text, AmOfBytes);
	if(result = MDSetTextWavePointValue(pRad->wRad, RadIndices, textH)) return result;
	DisposeHandle(textH);
	return 0;
}

//*************************************************************************

int srTIgorSend::SetupExtractedWaveData(srTSRWRadInData* pRad, int Int_or_Phase, int PlotType, int TransvPres, char* pExtrData, waveHndl wavH, int hStateExtrData, srTIgorWaveAccessData* pExtrWaveData)
{
	if((pRad == 0) || (pExtrWaveData == 0)) return 0;
	srTIgorWaveAccessData& ExtrWaveData = *pExtrWaveData;
	srTSRWRadInData& RadAccessData = *pRad;

	ExtrWaveData.wHndl = wavH;
	ExtrWaveData.hState = hStateExtrData;

	ExtrWaveData.pWaveData = pExtrData;
	if(Int_or_Phase != 2) 
	{
		*(ExtrWaveData.WaveType) = 'f';
	}
	else
	{
		*(ExtrWaveData.WaveType) = 'd';
	}

	int PT = PlotType;
	ExtrWaveData.AmOfDims = ((PT >= 0) && (PT < 3))? 1 : ((PT < 6)? 2 : 3);
	char TransvUnitsChar = (TransvPres == 0)? 'm' : 'q';

	char* pUnit0 = *(ExtrWaveData.DimUnits);
	char* pUnit1 = *(ExtrWaveData.DimUnits + 1);
	char* pUnit2 = *(ExtrWaveData.DimUnits + 2);

	for(int i=0; i<3; i++)
	{
		*(pUnit0 + i) = '\0'; *(pUnit1 + i) = '\0'; *(pUnit2 + i) = '\0';
	}

	if(PT == 0) // vs e
	{
		*(ExtrWaveData.DimSizes) = RadAccessData.ne;
		*(ExtrWaveData.DimStartValues) = RadAccessData.eStart;
		*(ExtrWaveData.DimSteps) = RadAccessData.eStep;
		*pUnit0 = 'e'; *(pUnit0 + 1) = 'V';
	}
	else if(PT == 1) // vs x
	{
		*(ExtrWaveData.DimSizes) = RadAccessData.nx;
		*(ExtrWaveData.DimStartValues) = RadAccessData.xStart;
		*(ExtrWaveData.DimSteps) = RadAccessData.xStep;
		*pUnit0 = TransvUnitsChar;
	}
	else if(PT == 2) // vs z
	{
		*(ExtrWaveData.DimSizes) = RadAccessData.nz;
		*(ExtrWaveData.DimStartValues) = RadAccessData.zStart;
		*(ExtrWaveData.DimSteps) = RadAccessData.zStep;
		*pUnit0 = TransvUnitsChar;
	}
	else if(PT == 3) // vs x&z
	{
		*(ExtrWaveData.DimSizes) = RadAccessData.nx; *(ExtrWaveData.DimSizes + 1) = RadAccessData.nz;
		*(ExtrWaveData.DimStartValues) = RadAccessData.xStart; *(ExtrWaveData.DimStartValues + 1) = RadAccessData.zStart; 
		*(ExtrWaveData.DimSteps) = RadAccessData.xStep; *(ExtrWaveData.DimSteps + 1) = RadAccessData.zStep; 
		*pUnit0 = TransvUnitsChar;
		*pUnit1 = TransvUnitsChar;
	}
	else if(PT == 4) // vs e&x
	{
		*(ExtrWaveData.DimSizes) = RadAccessData.ne; *(ExtrWaveData.DimSizes + 1) = RadAccessData.nx;
		*(ExtrWaveData.DimStartValues) = RadAccessData.eStart; *(ExtrWaveData.DimStartValues + 1) = RadAccessData.xStart; 
		*(ExtrWaveData.DimSteps) = RadAccessData.eStep; *(ExtrWaveData.DimSteps + 1) = RadAccessData.xStep; 
		*pUnit0 = 'e'; *(pUnit0 + 1) = 'V';
		*pUnit1 = TransvUnitsChar;
	}
	else if(PT == 5) // vs e&z
	{
		*(ExtrWaveData.DimSizes) = RadAccessData.ne; *(ExtrWaveData.DimSizes + 1) = RadAccessData.nz;
		*(ExtrWaveData.DimStartValues) = RadAccessData.eStart; *(ExtrWaveData.DimStartValues + 1) = RadAccessData.zStart; 
		*(ExtrWaveData.DimSteps) = RadAccessData.eStep; *(ExtrWaveData.DimSteps + 1) = RadAccessData.zStep; 
		*pUnit0 = 'e'; *(pUnit0 + 1) = 'V';
		*pUnit1 = TransvUnitsChar;
	}
	else if(PT == 6) // vs e&x&z
	{
		*(ExtrWaveData.DimSizes) = RadAccessData.ne; *(ExtrWaveData.DimSizes + 1) = RadAccessData.nx; *(ExtrWaveData.DimSizes + 2) = RadAccessData.nz;
		*(ExtrWaveData.DimStartValues) = RadAccessData.eStart; *(ExtrWaveData.DimStartValues + 1) = RadAccessData.xStart; *(ExtrWaveData.DimStartValues + 2) = RadAccessData.zStart;
		*(ExtrWaveData.DimSteps) = RadAccessData.eStep; *(ExtrWaveData.DimSteps + 1) = RadAccessData.xStep; *(ExtrWaveData.DimSteps + 2) = RadAccessData.zStep; 
		*pUnit0 = 'e'; *(pUnit0 + 1) = 'V';
		*pUnit1 = TransvUnitsChar;
		*pUnit2 = TransvUnitsChar;
	}
	return 0;
}

//*************************************************************************

int srTIgorSend::GetPrecParamStokesPerComp(waveHndl wavH, int& InitHarm, int& FinHarm, double& Kns, double& Knphi, char& IntensityOrFlux)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	int result;
	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp0 = (DOUBLE*)dataStartPtr;
	DOUBLE* dp = dp0;

	InitHarm = int(*(dp++));
	FinHarm = int(*(dp++));
	Kns = *(dp++);
	Knphi = *(dp++);
	IntensityOrFlux = 'i';
	if(*dp == 1) IntensityOrFlux = 'f';
	else if(*dp == 2) IntensityOrFlux = 'i';
	else if(*dp == 3) IntensityOrFlux = 'a';

	HSetState((Handle)wavH, hState);
	return 0;
}


//*************************************************************************

int srTIgorSend::GetPrecParamPowDensComp(waveHndl wavH, double& PrecFact, int& Method)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	int result;
	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp = (DOUBLE*)dataStartPtr;

	PrecFact = *(dp++);
	Method = int(*dp + 0.00001);

	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetPrecParamMagArb2Per(waveHndl wavH, double& RelPrec, int& MaxAmOfHarm)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != NT_FP64) return NT_FP64_WAVE_REQUIRED;

	int result;
	long dataOffset;
	if(result = MDAccessNumericWaveData(wavH, kMDWaveAccessMode0, &dataOffset)) return result;
	int hState = MoveLockHandle(wavH);
	char* dataStartPtr = (char*)(*wavH) + dataOffset;
	DOUBLE* dp = (DOUBLE*)dataStartPtr;

	RelPrec = *(dp++);
	MaxAmOfHarm = int(*dp + 0.00001);

	HSetState((Handle)wavH, hState);
	return 0;
}

//*************************************************************************

int srTIgorSend::GetGaussianBeam(waveHndl& wavH, double& Phot, int& Polar, double& SigmaX, double& SigmaZ, int& mx, int& mz, double* pMom1, double& s0, double& SigmaT, int& TypeDistrT)
{
	if(wavH == NIL) return NOWAV;
	int waveType = WaveType(wavH);
	if(waveType != TEXT_WAVE_TYPE) return IMPROPER_GSN_BEAM_STRUCTURE;

    double &x0 = pMom1[0], &dxds0 = pMom1[1], &z0 = pMom1[2], &dzds0 = pMom1[3];
    pMom1[4] = 0;

	int result = 0;

// Electron Beam, if defined
	long Indices[MAX_DIMENSIONS];
	Handle textH = NewHandle(0L);
	Indices[0] = 0;
	if(result = MDGetTextWavePointValue(wavH, Indices, textH)) return result;
	if(PtrAndHand("\0", textH, 1)) return MEMORY_ALLOCATION_FAILURE;

	double AmOfElectronsPerSec = 1.;
	bool ElecBeamIsDefined = ((*textH != 0) && (strlen(*textH) > 0));
	if(ElecBeamIsDefined)
	{
		waveHndl wavH_AuxElectronBeam = FetchWave(*textH);
		if(wavH_AuxElectronBeam == NIL) return IMPROPER_GSN_BEAM_STRUCTURE;

		double ElecMom1[6];
		double I;
		if(result = GetElecBeamThin(wavH_AuxElectronBeam, I, ElecMom1, s0)) return result;

		x0 = ElecMom1[1]; dxds0 = ElecMom1[2], z0 = ElecMom1[3], dzds0 = ElecMom1[4];

		double ElecChargeC = 1.602189246E-19;
		AmOfElectronsPerSec = I/ElecChargeC;
	}
	else
	{
        GetDoubleFromTextWave1D(wavH, 8, s0);
        GetDoubleFromTextWave1D(wavH, 9, x0);
        GetDoubleFromTextWave1D(wavH, 10, dxds0);
        GetDoubleFromTextWave1D(wavH, 11, z0);
        GetDoubleFromTextWave1D(wavH, 12, dzds0);
	}
	DisposeHandle(textH);

// Gaussian Beam parameters
	if(result = GetDoubleFromTextWave1D(wavH, 1, SigmaX)) return result;
	if(SigmaX <= 0.) return BAD_WAIST_SIZE;

	if(result = GetDoubleFromTextWave1D(wavH, 2, SigmaZ)) return result;
	if(SigmaZ <= 0.) return BAD_WAIST_SIZE;

	double dmx = 0;
	if(result = GetDoubleFromTextWave1D(wavH, 3, dmx)) return result;
	if(dmx < 0.) return BAD_MODE_ORDER;
	mx = int(dmx + 0.000001);

	double dmz = 0;
	if(result = GetDoubleFromTextWave1D(wavH, 4, dmz)) return result;
	if(dmz < 0.) return BAD_MODE_ORDER;
	mz = int(dmz + 0.000001);

	if(result = GetDoubleFromTextWave1D(wavH, 5, Phot)) return result;
	if(Phot <= 0.) return BAD_PHOTON_NUMBER;
	Phot *= AmOfElectronsPerSec;

	double dPolar;
	if(result = GetDoubleFromTextWave1D(wavH, 6, dPolar)) return result;
	if((dPolar < 1.) || (dPolar > 6.)) return BAD_POLAR_SPECIFIER;
	Polar = int(dPolar);

	GetDoubleFromTextWave1D(wavH, 14, SigmaT);

	double dTypeDistrT = 1.;
	GetDoubleFromTextWave1D(wavH, 15, dTypeDistrT);
	if(dTypeDistrT != 0.) TypeDistrT = int(dTypeDistrT + 0.000001);

	return 0;
}

//*************************************************************************

int srTIgorSend::GetAndSetElecBeamThick(int* piElecBeam, waveHndl& wavH)
{
    double I, s0_ebm, Mom1Arr[6], Mom2Arr[30];
	int nMom2;
	int TypeDistrTrans, TypeDistrLong;
	double NoiseFactor;

	int res = 0;
	if(res = GetElecBeamThick(wavH, I, Mom1Arr, Mom2Arr, nMom2, s0_ebm, TypeDistrTrans, TypeDistrLong, NoiseFactor)) return res;
	if(res = srElecBeamSet(piElecBeam, I, Mom1Arr, 5, Mom2Arr, nMom2, s0_ebm)) return res;
	return 0;
}

//*************************************************************************

int srTIgorSend::GetAndSetElecBeamThin(int* piElecBeam, waveHndl& wavH)
{
	double I, sElecBeam, Mom1Arr[6];
	int res = 0;
	if(res = GetElecBeamThin(wavH, I, Mom1Arr, sElecBeam)) return res;
	if(res = srElecBeamSet(piElecBeam, I, Mom1Arr, 5, 0, 0, sElecBeam)) return res;
	return 0;
}

//*************************************************************************

int srTIgorSend::GetAndSetGaussianBeam(int* piGsnBeam, waveHndl& wavH)
{
	double Phot, SigmaX, SigmaZ, Mom1Arr[5], s0, SigmaT;
	int Polar, mx, mz, TypeDistrT;
	int res = 0;
	if(res = GetGaussianBeam(wavH, Phot, Polar, SigmaX, SigmaZ, mx, mz, Mom1Arr, s0, SigmaT, TypeDistrT)) return res;
	if(res = srGausBeamSet(piGsnBeam, Phot, Polar, SigmaX, mx, SigmaZ, mz, SigmaT, TypeDistrT, Mom1Arr, s0)) return res;
	return 0;
}

//*************************************************************************

int srTIgorSend::GetAndSetMagFieldGen(int* piMagFld, waveHndl& wavH)
{
	int res = 0;
    int MagFieldType = 0;
    if(res = IdentifyMagFieldType(wavH, MagFieldType)) return res;

	if(MagFieldType == 1) // 1: arbitrary transversely uniform
	{
        double FieldAbsZeroTol = 1.E-12, sStartB, sStepB, *pBH, *pBV;
        long NpB;
        bool BHIsZero, BVIsZero;
		if(res = GetMagFieldTransvUnif(wavH, FieldAbsZeroTol, sStartB, sStepB, NpB, pBH, BHIsZero, pBV, BVIsZero)) return res;
        if(res = srMagFldTrUnifSet(piMagFld, sStartB, sStepB, NpB, pBH, pBV)) return res;
	}
    else if(MagFieldType == 2) // 2: periodic
	{
		double PerLength, TotLength, TaperPar_TU, PhaseSh_OK, FldErrRMS, NatFocNxSASE, NatFocNySASE, TaperStartSASE, TaperRelFldChgSASE;
		int AmOfHarm, TypeOfUnd, FldErrTypeSASE, TaperTypeSASE;
		//TypeOfUnd: 0- infinite; 1- normal; 2- tapered; 3- optical clystron

		int *ArrHarmNo = 0;
		char *ArrXorZ = 0;
		double *ArrK = 0, *ArrPhase = 0;
		if(res = GetMagFieldPeriodic(wavH, PerLength, TotLength, AmOfHarm, ArrHarmNo, ArrXorZ, ArrK, ArrPhase, TypeOfUnd, TaperPar_TU, PhaseSh_OK, FldErrTypeSASE, FldErrRMS, NatFocNxSASE, NatFocNySASE, TaperTypeSASE, TaperStartSASE, TaperRelFldChgSASE)) return res;

		double SpecPar = 0.;
		char Type = 'c'; //'c'- conventional, 't'- tapered, 'k'- optical klystron, 'i'- infinite
		if(TypeOfUnd == 0) Type = 'i';
		else if(TypeOfUnd == 1) Type = 'c';
		else if(TypeOfUnd == 2) { Type = 't'; SpecPar = TaperPar_TU;}
		else if(TypeOfUnd == 3) { Type = 'k'; SpecPar = PhaseSh_OK;}

		if(res = srMagFldPerSet(piMagFld, PerLength, TotLength, 0., AmOfHarm, ArrHarmNo, ArrXorZ, ArrK, ArrPhase, Type, SpecPar)) return res;

		if(ArrHarmNo != 0) delete[] ArrHarmNo;
		if(ArrXorZ != 0) delete[] ArrXorZ;
		if(ArrK != 0) delete[] ArrK;
		if(ArrPhase != 0) delete[] ArrPhase;
	}
    else if(MagFieldType == 3) // 3: constant
	{
        double BH, BV;
        if(res = GetMagFieldConstant(wavH, BH, BV)) return res;
        if(res = srMagFldConstSet(piMagFld, BH, BV)) return res;
	}
    else if(MagFieldType == 4) // 4: mag. optics
	{
		//to implement !!!
	}
    else if(MagFieldType == 5) // 5: container
	{
		vector<string> NameVect;
		if(res = GetVectorOfStrings(wavH, &NameVect)) return res;
		int AmOfElems = NameVect.size();
		if(AmOfElems == 0) return EMPTY_MAG_FIELD_CONTAINER;
		int* IndsMagElems = new int[AmOfElems];
		int* tIndsMagElems = IndsMagElems;

		for(int i=0; i<AmOfElems; i++)
		{
			*tIndsMagElems = 0;

			const char* pCurName = NameVect[i].c_str();
			if(pCurName == 0) continue;
            waveHndl wCurElem = FetchWave(pCurName);
			if(wCurElem == NIL) return CAN_NOT_FIND_MAG_FIELD_WAVE;

			if(res = GetAndSetMagFieldGen(tIndsMagElems, wCurElem)) return res;
            tIndsMagElems++;
		}
		if(res = srMagFldContSet(piMagFld, IndsMagElems, AmOfElems)) return res;
		if(IndsMagElems != 0) delete[] IndsMagElems;
	}
	return 0;
}

//*************************************************************************

int srTIgorSend::GetAndSetWfrSampling(int* piWfrSmp, waveHndl& wavH)
{
	int res = 0;
	double yObs, zSt, zFi, xSt, xFi, eSt, eFi;
	int nz, nx, ne;
	char PhotEnUnits[10];
	if(res = GetWfrSampling(wavH, yObs, zSt, zFi, nz, xSt, xFi, nx, eSt, eFi, ne, PhotEnUnits)) return res;
	if(res = srWfrSmpSet(piWfrSmp, yObs, xSt, xFi, nx, zSt, zFi, nz, eSt, eFi, ne, PhotEnUnits)) return res;
	return 0;
}
**/
//*************************************************************************

int srTIgorAccessNumFunc2Waves::Initialize(char* InFuncName, int NumVar)
{
	if(InFuncName == 0) return 0;
	if(NumVar == 0) return 0;

	char varName[30];
	strcpy(varName, "v_");

	int LenInFuncName = (int)strlen(InFuncName);
	if(LenInFuncName > 28) LenInFuncName = 28;
    strncat(varName, InFuncName, LenInFuncName);
	strcpy(ResVarName, varName);

	double zero=0, dummy=0;
	int res=0;
	if(res = SetIgorFloatingVar(varName, &zero, 1)) return res;
	//int varType = FetchNumVar(ResVarName, &zero, &dummy);
	//if((varType != NT_FP32) && (varType != NT_FP64))
	//{
	//	Variable(ResVarName, NT_FP64);
	//}

	strcpy(varName, "w1_");
	strncat(varName, InFuncName, LenInFuncName);
	if(res = MakeWave(&wArg1, varName, NumVar, NT_FP64, 1)) return res;
	strcpy(ArgWaveName1, varName);

	strcpy(varName, "w2_");
	strncat(varName, InFuncName, LenInFuncName);
	if(res = MakeWave(&wArg2, varName, NumVar + 1, NT_FP64, 1)) return res;
	strcpy(ArgWaveName2, varName);

	strcpy(FuncName, InFuncName);
	NumVars = NumVar;

	strcpy(CommandStr, ResVarName);
	strcat(CommandStr, " = ");
	strcat(CommandStr, FuncName);
	strcat(CommandStr, "(");
	strcat(CommandStr, ArgWaveName1);
	strcat(CommandStr, ", ");
	strcat(CommandStr, ArgWaveName2);
	strcat(CommandStr, ");");
	return 0;
}

//*************************************************************************

double srTIgorAccessNumFunc2Waves::CalcFunc(double* pArgs, double* pArgsMin, double FMin)
{
	int res=0;
	double *pdData1=0, *pdData2=0;
	float* pfData=0;
	long LenData=0;
	int hState1, hState2;
	if(res = srTIgorInterf::GetDataPtrFromWaveDoubleOrFloat1D(wArg1, pdData1, pfData, LenData, hState1)) throw res;
	if(res = srTIgorInterf::GetDataPtrFromWaveDoubleOrFloat1D(wArg2, pdData2, pfData, LenData, hState2)) throw res;

    double *tArgs = pArgs, *tArgsMin = pArgsMin, *tData1 = pdData1, *tData2 = pdData2;
	for(int i=0; i<NumVars; i++) 
	{
        *(tData1++) = *(tArgs++);
        *(tData2++) = *(tArgsMin++);
	}
	*tData2 = FMin;
    if(res = srTIgorInterf::ReleaseWave(wArg1, hState1)) throw res;
    if(res = srTIgorInterf::ReleaseWave(wArg2, hState2)) throw res;

	if(res = XOPSilentCommand(CommandStr)) throw res;
	DOUBLE ResValue, Dummy;
	FetchNumVar(ResVarName, &ResValue, &Dummy);

	return (double)ResValue;
}

//*************************************************************************
