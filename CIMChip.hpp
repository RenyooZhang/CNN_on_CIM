#ifndef CIMChip_hpp
#define CIMChip_hpp

#include <bitset>

/*---------------------------------------CIM----------------------------------------*/

#define N_CIM_ROW (256U)
#define N_CIM_COL (64U)

// һ����дInput��һά����дWeight�������һά����Output
class CIM {
public:
	// дToInput��array��Դ��bias��array��ƫ�ƣ�һ��һ��д
	void SetToInput(unsigned int* arr, unsigned int bias);
	// дWeights��array��Դ��bias��array��ƫ�ƣ������ȵ�д
	void SetWeights(unsigned int* arr, unsigned int bias);
	// ��Output��array��Ŀ�ģ�bias��array��ƫ��
	void ReadOutput(unsigned int* arr, unsigned int bias) const;
	// һ�������ĳ˼Ӽ���
	void FullMulAdd(unsigned int lenth);

private:
	unsigned int ToInput[N_CIM_ROW] = { 0u };				// 8λ����
	std::bitset<N_CIM_ROW> Input = { 0 };					// ��λ�����λ
	unsigned int Weights[N_CIM_ROW][N_CIM_COL] = { 0 };		// ����˵�Ȩ��
	unsigned int ToOutput[N_CIM_COL] = { 0 };				// һ������͵õ������
	unsigned int Output[N_CIM_COL] = { 0 };					// ToOutput���ۼӺ�

	// Input����һλ
	void SetInput();
	// ������
	void ClearOutput();
	// һ�γ˼Ӳ����������ToOutput
	void Mul_add();
	// һ�μӷ����ݹ���ÿ�����һ�üӷ���
	unsigned int Add(unsigned int a, unsigned int b, unsigned int h) const;
};

/*---------------------------------------Four_CIMs----------------------------------------*/

class Four_CIMs {
public:
	// ���캯�����ṩ4��cimģ��ͳ�ʼmode������һ��4Cims��չģ��
	Four_CIMs();
	// ����ģʽ��0��256x256��1��512x128��2��1024x64
	void SetMode(unsigned int mode);
	// дToInput��array��Դ��bias��array��ƫ�ƣ�һ��һ��д
	void SetToInput(unsigned int* arr, unsigned int bias);
	// дWeights��array��Դ��bias��array��ƫ�ƣ������ȵ�д
	void SetWeights(unsigned int* arr, unsigned int bias);
	// ����4��CIMģ���е�Output���������յ�Output������arr[bias]��
	void ReadOutput(unsigned int* arr, unsigned int bias) const;
	// һ�������ĳ˼Ӽ���
	void FullMulAdd(unsigned int lenth);

	const unsigned int& Mode;			// ������������m_mode��ֵ��

	static const unsigned int row_max[3];		// 0: 256;	1: 512;	2: 1024
	static const unsigned int col_max[3];		// 0: 256;	1: 128;	3: 64
	static const unsigned int row_num[3];		// 1��2��4
	static const unsigned int col_num[3];		// 4��2��1

private:
	unsigned int m_mode;				// 0: 1x4;	1: 2x2;	2: 4x1
	CIM m_cim[4];						// 4��cimоƬ
};

/*---------------------------------------SRAM_CIM----------------------------------------*/

class SRAM_CIM {
public:
	// 
	SRAM_CIM(unsigned int* p_sram, unsigned int n_weight, unsigned int n_toinput, unsigned int n_output,
		unsigned add_step, unsigned int add_turn, unsigned int add_times, unsigned int cal_step);
	SRAM_CIM() = delete;
	SRAM_CIM(const SRAM_CIM& source) = delete;
	SRAM_CIM& operator=(const SRAM_CIM& source) = delete;

	void Setting(unsigned int mode, unsigned int add_step, unsigned int add_turn, unsigned int add_times, unsigned int cal_step);

	void WriteWeight(unsigned int mode, unsigned int* source);
	void WriteToInput(unsigned int mode, unsigned int* source);
	void ReadOutput(unsigned int mode, unsigned int* dest);

	void Calculate();

	void Clear();

	const unsigned int& Tail_Result;
	unsigned int* const P_Sram;
	const unsigned int& I_Result;
	const unsigned int& I_Output;
	unsigned int* const P_SRAM;			// SRAM�׵�ַ

private:
	const unsigned int I_WEIGHT;
	const unsigned int I_TOINPUT;
	const unsigned int I_OUTPUT;
	const unsigned int I_RESULT;

	unsigned int tail_weight;
	unsigned int tail_toinput;
	unsigned int tail_output;
	unsigned int tail_result;

	unsigned int head_weight;
	unsigned int head_toinput;
	unsigned int head_output;
	unsigned int head_result;

	unsigned int cims_mode;

	unsigned int m_add_step;
	unsigned int m_add_turn;
	unsigned int m_add_times;

	unsigned int m_cal_step;

	Four_CIMs cims;

	void LoadMode();
	void LoadWeight();
	void LoadToInput();
	void UpdateOutput();
	void UpdateResult();
};

/*---------------------------------------Conv3x3----------------------------------------*/

#define INPUT_A_MAX (224)
#define INPUT_B_MAX (224)
#define INPUT_C_MAX (512)
#define KERNEL_C_MAX (512)
#define KERNEL_D_MAX (512)

#define OUTPUT_A_MAX (224)
#define OUTPUT_B_MAX (224)
#define OUTPUT_C_MAX (512)

// һά����X, W��Ϊ�����ʼ����һά����O�����
// ע�⣺��������pixel����С��256�������������������256����û�п��ǹ�ĩβ�����������
template<int ia, int ib, int ic, int oc>
class Conv3x3 {
public:
	Conv3x3();
	Conv3x3(const Conv3x3& source) = delete;
	Conv3x3& operator=(const Conv3x3& source) = delete;
	void Conv(int* sourceX, int biasX, int* sourceW, int biasW, int* destO, int biasO, SRAM_CIM& sram_cim);


private:

	int X[INPUT_A_MAX * INPUT_B_MAX * INPUT_C_MAX] = { 0 };			// X[a * ib * ic + b * ic + c] <==> X[a][b][c]
	int W[KERNEL_D_MAX * 3 * 3 * KERNEL_C_MAX] = { 0 };				// W[d * ka * kb * kc + a * kb * kc + b * kc + c] <==> W[d][a][b][c]
	int O[OUTPUT_A_MAX * OUTPUT_B_MAX * OUTPUT_C_MAX] = { 0 };		// O[a * ob * oc + b * oc + c] <==> O[a][b][c]

	// Load�е�����
	//int area[3 * 3 * KERNEL_C_MAX] = { 0 };
	unsigned int load[4 * N_CIM_ROW * N_CIM_COL] = { 0 };
	unsigned int load1[4 * N_CIM_ROW * N_CIM_COL] = { 0 };
	// UpdateO�е�����
	int unload[OUTPUT_A_MAX * OUTPUT_B_MAX * OUTPUT_C_MAX] = { 0 };

	const int ka = 3;
	const int kb = 3;
	const int kc = ic;
	const int kd = oc;

	const int oa = ia;
	const int ob = ib;

	unsigned int chip_mode;
	unsigned int chip_add_step;
	unsigned int chip_add_turn;
	unsigned int chip_add_times;
	unsigned int chip_cal_step;

	void SetX(int* source, int bias);
	void SetW(int* source, int bias);
	void ReadO(int* dest, int bias);

	void LoadX(SRAM_CIM& sram_cim);
	void LoadW(SRAM_CIM& sram_cim);
	void UpdateO(SRAM_CIM& sram_cim);

	inline int GetNextXInBlock(int index, int row, int col) {
		bool end = (index == (row - 1) * ib * ic + (col + 1) * ic + ic) ||
			(index == row * ib * ic + (col + 1) * ic + ic);
		return end ? index + (ib - 3) * ic : index + 1;
	}

	int GetNextIndex(int times, int index, int row, int col);
};

/*---------------------------------------FullConnect----------------------------------------*/
template<int ni, int no>
class FullConnect {
public:
	FullConnect();
	FullConnect(const FullConnect& source) = delete;
	FullConnect& operator=(const FullConnect& source) = delete;
	void FC(int* sourceI, int biasI, int* sourceW, int biasW, int* destO, int biasO, SRAM_CIM& chip);

private:

	int I[INPUT_A_MAX * INPUT_B_MAX * INPUT_C_MAX] = { 0 };					// X[a * ib * ic + b * ic + c] <==> X[a][b][c]
	int W[INPUT_A_MAX * INPUT_B_MAX * INPUT_C_MAX * OUTPUT_C_MAX] = { 0 };	// W[d * ka * kb * kc + a * kb * kc + b * kc + c] <==> W[d][a][b][c]
	int O[OUTPUT_A_MAX * OUTPUT_B_MAX * OUTPUT_C_MAX] = { 0 };				// O[a * ob * oc + b * oc + c] <==> O[a][b][c]

	unsigned int load[4 * N_CIM_ROW * N_CIM_COL] = { 0 };
	unsigned int load1[4 * N_CIM_ROW * N_CIM_COL] = { 0 };
	unsigned int unload[OUTPUT_A_MAX * OUTPUT_B_MAX * OUTPUT_C_MAX] = { 0 };

	unsigned int chip_mode;
	unsigned int chip_add_step;
	unsigned int chip_add_turn;
	unsigned int chip_add_times;
	unsigned int chip_cal_step;

	void SetI(int* source, int bias);
	void SetW(int* source, int bias);
	void ReadO(int* dest, int bias);

	void LoadI(SRAM_CIM& sram_cim);
	void LoadW(SRAM_CIM& sram_cim);
	void UpdateO(SRAM_CIM& sram_cim);
};


/*---------------------------------------MaxPool2x2----------------------------------------*/

// 
template<int ia, int ib, int ic>
class MaxPool {
public:

	void MP(int* sourceX, int biasX, int* destO, int biasO);

private:
	const int oa = ia / 2;
	const int ob = ib / 2;
	const int oc = ic;

	int X[INPUT_A_MAX * INPUT_B_MAX * INPUT_C_MAX];			// X[a * ib * ic + b * ic + c] <==> X[a][b][c]
	int M[OUTPUT_A_MAX * INPUT_B_MAX * INPUT_C_MAX];		// M[a * ib * ic + b * ic + c] <==> M[a][b][c]
	int O[OUTPUT_A_MAX * OUTPUT_B_MAX * OUTPUT_C_MAX];		// O[a * ob * oc + b * oc + c] <==> O[a][b][c]

	// ��array������X��array��SRAM�ж����������ݣ�bias��array��ƫ��
	void SetX(int* arr, int bias);
	// ��array����ȡO��array�Ǽ���д��SRAM�ж����������ݣ�bias��array��ƫ��
	void ReadO(int* arr, int bias) const;
	// һ�ξ�������������O
	void Pooling();

};


/*---------------------------------------Model----------------------------------------*/

#define N_MODEL_MAX (224u * 224u * 64u * 100u)
#define N_TYPE_MAX (224u * 224u * 64u)

// һά���������һά����д�룬����ÿλ4�ֽ�
class Model {
public:
	// �ڴ�д������arrayΪԴSRAMΪĿ�ģ�biasΪarray��ƫ����
	bool Writer(unsigned int type, unsigned int n, unsigned int* arr, unsigned int bias);
	// �ڴ��������arrayΪĿ��SRAMΪԴ��biasΪarray��ƫ����
	bool Reader(unsigned int type, unsigned int n, unsigned int* arr, unsigned int bias) const;

	static const unsigned int ID_INPUT = 0u;
	static const unsigned int ID_OUTPUT = N_TYPE_MAX - 1u;
	static const unsigned int ID_KERNELS = N_TYPE_MAX * 2u - 1u;

private:
	unsigned int m_SRAM[N_MODEL_MAX];
};


#endif // !CIMChip_hpp
