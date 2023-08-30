#include "CIMChip.hpp"
#include <stdio.h>

unsigned int p_sram[100000000] = { 0 };

CIM cim;
Four_CIMs cims;
SRAM_CIM chip(p_sram, 100 * N_CIM_ROW * N_CIM_COL, 100 * N_CIM_ROW, 100 * N_CIM_COL, 100 * N_CIM_COL, 4U, 1U, 1u);
Conv3x3<3, 3, 257, 10> conv;
//FullConnect<2000, 300> fc;
//MaxPool<20, 20, 10> maxpool;

void PrintArray(int* arr, int bias, int n) {
	for (int i = 0; i < n; i++) {
		//if (i % 16 == 15) { std::cout << arr[bias + i] << " " << std::endl; }
		//else { std::cout << arr[bias + i] << " "; }
		if (i % 10 == 9) { printf("%5d \n", arr[bias + i]); }
		else { printf("%5d ", arr[bias + i]); }
	}
}

// Conv3X3²âÊÔ
int XT[3 * 3 * 257] = { 0 };
int WT[10 * 3 * 3 * 257] = { 0 };
int OT[3 * 3 * 10] = { 0 };

// FullConnect²âÊÔ
//int XT[2000] = { 0 };
//int WT[2000 * 300] = { 0 };
//int OT[300] = { 0 };

// MaxPool²âÊÔ
//int XT[20 * 20 * 10] = { 0 };
//int OT[10 * 10 * 10] = { 0 };

int main() {

	// CIM²âÊÔ
	/*unsigned int Weights[4 * N_CIM_ROW * N_CIM_COL] = { 0 };
	unsigned int ToInput[4 * N_CIM_ROW] = { 0 };
	unsigned int Output[4 * N_CIM_COL] = { 0 };
	for (int i = 0; i < 4 * N_CIM_ROW * N_CIM_COL; i++) {
		Weights[i] += 2;
	}
	for (int i = 0; i < 4 * N_CIM_ROW; i++) {
		ToInput[i] += 1;
	}
	cim.SetToInput(ToInput, 0);
	cim.SetWeights(Weights, 0);
	cim.FullMulAdd(8);
	cim.ReadOutput(Output, 0);
	PrintArray(Output, 0, N_CIM_COL);*/


	// Four_CIMs²âÊÔ
	/*unsigned int Weights[4 * N_CIM_ROW * N_CIM_COL] = { 0 };
	unsigned int ToInput[4 * N_CIM_ROW] = { 0 };
	unsigned int Output[4 * N_CIM_COL] = { 0 };
	for (int i = 0; i < 4 * N_CIM_ROW * N_CIM_COL; i++) {
		Weights[i] += 1;
	}
	for (int i = 0; i < 4 * N_CIM_ROW; i++) {
		ToInput[i] += 1;
	}
	cims.SetMode(2);
	cims.SetWeights(Weights, 0);
	cims.SetToInput(ToInput, 0);
	cims.FullMulAdd(8);
	cims.ReadOutput(Output, 0);
	PrintArray(Output, 0, 1 * N_CIM_COL);*/

	// SRAM_CIM²âÊÔ
	/*unsigned int Weights[4 * N_CIM_ROW * N_CIM_COL] = { 0 };
	unsigned int ToInput[4 * N_CIM_ROW] = { 0 };
	unsigned int Output[8 * N_CIM_COL] = { 0 };
	for (int i = 0; i < 4 * N_CIM_ROW * N_CIM_COL; i++) {
		Weights[i] += 1;
	}
	for (int i = 0; i < 4 * N_CIM_ROW; i++) {
		ToInput[i] += 1;
	}
	chip.WriteToInput(0, ToInput);
	chip.WriteWeight(0, Weights);
	chip.WriteToInput(0, ToInput);
	chip.WriteWeight(0, Weights);
	chip.Calculate();
	chip.ReadOutput(0, Output);
	PrintArray(Output, 0, 4 * N_CIM_COL);*/

	// Conv3x3²âÊÔ

	for (int i = 0; i < 3 * 3 * 257; i++) {
		XT[i] += 1;
	}
	for (int i = 0; i < 10 * 3 * 3 * 257; i++) {
		WT[i] += 1;
	}
	conv.Conv(XT, 0, WT, 0, OT, 0, chip);
	PrintArray(OT, 0, 3 * 3 * 10);
	printf("calculating times: %d\nwrite amount: %d\nwrite bits: %d\nread amount: %d\nread bits: %d"
			, chip.Cal, chip.Write, chip.WriteBits, chip.Read, chip.ReadBits);
	// FullConnect²âÊÔ
	//for (int i = 0; i < 2000; i++) {
	//	XT[i] += 1;
	//}
	//for (int i = 0; i < 2000 * 300; i++) {
	//	WT[i] += 1;
	//}
	//fc.FC(XT, 0, WT, 0, OT, 0, chip);

	// MaxPool²âÊÔ
	//for (int i = 0; i < 20 * 20 * 10; i += 7) {
	//	XT[i] += 1;
	//}
	//for (int i = 1; i < 20 * 20 * 10; i += 5) {
	//	XT[i] += 2;
	//}
	//for (int i = 2; i < 20 * 20 * 10; i += 5) {
	//	XT[i] += 3;
	//}
	//maxpool.MP(XT, 0, OT, 0);

	//PrintArray(OT, 0, 10 * 10 * 10);
	return 0;
}