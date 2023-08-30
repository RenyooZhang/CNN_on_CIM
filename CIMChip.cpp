
#include <bitset>
#include "CIMChip.hpp"
/*---------------------------------------CIM----------------------------------------*/

// private--------------------------------------------------------------------------

void CIM::SetInput() {
	for (unsigned int count = 0u; count < N_CIM_ROW; count++) {
		Input[count] = (ToInput[count] & 0x80) >> 7u;
		ToInput[count] = (ToInput[count] << 1u) & 0xFF;
	}
}

void CIM::ClearOutput() {
	for (unsigned int count = 0u; count < N_CIM_COL; count++) {
		Output[count] = 0u;
	}
}

unsigned int CIM::Add(unsigned int a, unsigned int b, unsigned int h) const {
	if (h == 0u) {
		return Input[a] * Weights[a][b];
	}
	else {
		int c = h - 1u;
		return Add(a, b, c) + Add(a + (1u << (c)), b, c);
	}
}

void CIM::Mul_add() {
	for (unsigned int count = 0u; count < N_CIM_COL; count++) {
		ToOutput[count] = Add(0u, count, 8u);
	}
}

// public------------------------------------------------------------------

void CIM::SetToInput(unsigned int* arr, unsigned int bias) {
	for (unsigned int count = 0u; count < N_CIM_ROW; count++) {
		ToInput[count] = arr[bias + count];
	}
}

void CIM::SetWeights(unsigned int* arr, unsigned int bias) {
	for (unsigned int row = 0u; row < N_CIM_ROW; row++) {
		for (unsigned int col = 0u; col < N_CIM_COL; col++) {
			Weights[row][col] = arr[bias + row * N_CIM_COL + col];
		}
	}
}

void CIM::ReadOutput(unsigned int* arr, unsigned int bias) const{
	for (unsigned int count = 0u; count < N_CIM_COL; count++) {
		arr[bias + count] = Output[count];
	}
}

void CIM::FullMulAdd(unsigned int lenth) {
	ClearOutput();
	for (unsigned int count = 0u; count < lenth; count++) {
		SetInput();
		Mul_add();
		for (unsigned int count2 = 0u; count2 < N_CIM_COL; count2++) {
			Output[count2] <<= 1u;
			Output[count2] += ToOutput[count2];		// 这里的逻辑是高位先进
		}
	}
}

/*---------------------------------------Four_CIMs----------------------------------------*/

// private--------------------------------------------------------------------------

// public---------------------------------------------------------------------------

unsigned const int Four_CIMs::row_max[3] = { 256u, 512u, 1024u };		// 0: 256;	1: 512;	2: 1024
unsigned const int Four_CIMs::col_max[3] = { 256u, 128u, 64u };		// 0: 256;	1: 128;	3: 64
unsigned const int Four_CIMs::row_num[3] = { 1u, 2u, 4u };
unsigned const int Four_CIMs::col_num[3] = { 4u, 2u, 1u };

Four_CIMs::Four_CIMs() : Mode(m_mode) {
	m_mode = 0u;	// 则默认为0
}

void Four_CIMs::SetMode(unsigned int mode) {
	m_mode = (mode == 0u || mode == 1u || mode == 2u) ? mode : m_mode;
}

void Four_CIMs::SetToInput(unsigned int* arr, unsigned int bias) {
	unsigned int row_round = 1u << m_mode;										// 有几行芯片，mode == 0是1行
	unsigned int col_round = 1u << (2u - m_mode);								// 有几列芯片，mode == 0是2行
	// 遍历每一行芯片（因为每一行上的芯片都是共享输入的）
	for (unsigned int count0 = 0u; count0 < row_round; count0++) {
		unsigned int arr_bias = bias + count0 * N_CIM_ROW;						// 第count0行芯片输入在arr上对应的偏移
		// 遍历每一列芯片，对每一列芯片Load输入
		for (unsigned int count1 = 0u; count1 < col_round; count1++) {
			m_cim[count0 * col_round + count1].SetToInput(arr, arr_bias);
		}
	}
}

void Four_CIMs::SetWeights(unsigned int* arr, unsigned int bias) {
	unsigned int row_round = 1u << m_mode;										// 有几行芯片，mode == 0是1行
	unsigned int col_round = 1u << (2u - m_mode);								// 有几列芯片，mode == 0是2行
	// 遍历每一行芯片
	for (unsigned int count0 = 0u; count0 < row_round; count0++) {
		// 遍历每一列芯片
		for (unsigned int count1 = 0u; count1 < col_round; count1++) {
			unsigned int arr_bias = bias + (count0 * col_round + count1) * N_CIM_ROW * N_CIM_COL;
			m_cim[count0 * col_round + count1].SetWeights(arr, arr_bias);
		}
	}
}

void Four_CIMs::ReadOutput(unsigned int* arr, unsigned int bias) const {
	unsigned int res[4 * N_CIM_COL] = { 0u };					// 最终结果的Output
	// 遍历4个模块，把Output存到res里
	for (unsigned int count = 0u; count < 4u; count++) {
		m_cim[count].ReadOutput(res, count * N_CIM_COL);
	}
	unsigned int distance = col_max[Mode];						// 要求和的两项间隔的距离
	unsigned int times = row_num[Mode];
	// 求和，要把后面的1到time-1部分加到第0部分
	for (unsigned int count0 = 1u; count0 < times; count0++) {
		for (unsigned int count1 = 0u; count1 < distance; count1++) {
			res[count1] += res[count0 * distance + count1];
		}
	}
	// 赋值给arr数组，传出去
	for (unsigned int count = 0u; count < distance; count++) {
		arr[bias + count] = res[count];
	}
}

void Four_CIMs::FullMulAdd(unsigned int lenth) {
	for (unsigned int count = 0u; count < 4u; count++) {
		m_cim[count].FullMulAdd(lenth);
	}
}

/*---------------------------------------SRAM_CIM----------------------------------------*/

// private----------------------------------------------------------------------------

void SRAM_CIM::LoadMode() {
	cims.SetMode(cims_mode);
}

void SRAM_CIM::LoadWeight() {
	cims.SetWeights(P_SRAM, I_WEIGHT + head_weight);
	head_weight += 4u * N_CIM_ROW * N_CIM_COL;
	if (head_weight == tail_weight) {
		head_weight = 0u;
	}
}

void SRAM_CIM::LoadToInput() {
	unsigned int row_max = Four_CIMs::row_max[cims_mode];
	cims.SetToInput(P_SRAM, I_TOINPUT + head_toinput);
	head_toinput += row_max;
	if (head_toinput == tail_toinput) {
		head_toinput = 0u;
	}
}

void SRAM_CIM::UpdateOutput() {
	unsigned int col_max = Four_CIMs::col_max[cims_mode];
	cims.FullMulAdd(8u);
	cims.ReadOutput(P_SRAM, I_OUTPUT + tail_output);
	tail_output += col_max;
}

void SRAM_CIM::UpdateResult() {
	// 因为kernel会分很多次放，所以Output也被自动切割成了几部分
	for (unsigned int count = 0; count < m_add_times; count++) {
		for (unsigned int count0 = 0u; count0 < m_add_step; count0++) {
			P_SRAM[I_RESULT + tail_result + count0] = 0u;
		}
		for (unsigned int count0 = 0u; count0 < m_add_turn; count0++) {
			for (unsigned int count1 = 0u; count1 < m_add_step; count1++) {
				P_SRAM[I_RESULT + tail_result + count1] += P_SRAM[I_OUTPUT + head_output + count0 * m_add_step + count1];
			}
		}
		for (unsigned int count0 = 0u; count0 < m_add_step; count0++) {
			P_SRAM[I_RESULT + tail_result + count0] = P_SRAM[I_RESULT + tail_result + count0] < 0u ? 0u : P_SRAM[I_RESULT + tail_result + count0];
		}
		head_output += m_add_step * m_add_turn;
		if (head_output == tail_output) { head_output = 0u; }
		tail_result += m_add_step;
	}
}
// public-----------------------------------------------------------------------------

SRAM_CIM::SRAM_CIM(unsigned int* p_sram, unsigned int n_weight, unsigned int n_toinput, unsigned int n_output,
	unsigned int add_step, unsigned int add_turn, unsigned int add_times, unsigned int cal_step) :
	P_SRAM(p_sram),
	I_WEIGHT(0u),
	I_TOINPUT(n_weight),
	I_OUTPUT(n_weight + n_toinput),
	I_RESULT(n_weight + n_toinput + n_output),
	Tail_Result(tail_result),
	P_Sram(P_SRAM),
	I_Result(I_RESULT),
	I_Output(I_OUTPUT)
{
	cims_mode = 0u;
	tail_weight = tail_toinput = tail_output = tail_result = 0u;
	head_weight = head_toinput = head_output = head_result = 0u;
	m_add_step = add_step;
	m_add_turn = add_turn;
	m_add_times = add_times;
	m_cal_step = cal_step;
}

void SRAM_CIM::Setting(unsigned int mode, unsigned int add_step, unsigned int add_turn, unsigned add_times, unsigned int cal_step) {
	cims_mode = mode;
	m_add_step = add_step;
	m_add_turn = add_turn;
	m_add_times = add_times;
	m_cal_step = cal_step;
}

void SRAM_CIM::WriteWeight(unsigned int mode, unsigned int* source) {
	for (unsigned int count = 0u; count < 4u * N_CIM_ROW * N_CIM_COL; count++) {
		P_SRAM[I_WEIGHT + tail_weight + count] = source[count];
	}
	tail_weight += 4u * N_CIM_ROW * N_CIM_COL;
}

void SRAM_CIM::WriteToInput(unsigned int mode, unsigned int* source) {
	unsigned int row_max = Four_CIMs::row_max[mode];
	for (unsigned int count = 0u; count < row_max; count++) {
		P_SRAM[I_TOINPUT + tail_toinput + count] = source[count];
	}
	tail_toinput += row_max;
}

void SRAM_CIM::ReadOutput(unsigned int mode, unsigned int* dest) {
	for (unsigned int count = 0u; count < tail_result; count++) {
		dest[count] = P_SRAM[I_RESULT + count];
	}
}

void SRAM_CIM::Calculate() {
	unsigned int row_max = Four_CIMs::row_max[cims_mode];
	cims.SetMode(cims_mode);
	Cal = Write = WriteBits = Read = ReadBits = 0u;
	for (unsigned int count0 = 0u; count0 < tail_weight; count0 += 4u * N_CIM_ROW * N_CIM_COL) {
		LoadWeight();
		Write += 4u * N_CIM_ROW * N_CIM_COL;
		WriteBits += 4u * N_CIM_ROW * N_CIM_COL * 4u;
		for (unsigned int count1 = 0; count1 < m_cal_step; count1 += row_max) {
			LoadToInput();
			Write += 4u * N_CIM_ROW;
			WriteBits += 4u * N_CIM_ROW * 8u;
			UpdateOutput();
			Read += 4u * N_CIM_COL;
			ReadBits += 4u * N_CIM_COL * 20u;
			Cal++;
		}
	}
	UpdateResult();
}

void SRAM_CIM::Clear() {
	tail_weight = tail_toinput = tail_output = tail_result = 0u;
	head_weight = head_toinput = head_output = head_result = 0u;
}

/*---------------------------------------Conv3x3----------------------------------------*/

// private--------------------------------------------------------------------------

template<int ia, int ib, int ic, int oc>
void Conv3x3<ia, ib, ic, oc>::SetX(int* source, int bias) {
	int n_x = ia * ib * ic;
	for (int count = 0; count < n_x; count++) {
		X[count] = source[bias + count];
	}
}

template<int ia, int ib, int ic, int oc>
void Conv3x3<ia, ib, ic, oc>::SetW(int* source, int bias) {
	int n_w = kd * ka * kb * kc;
	for (int count = 0; count < n_w; count++) {
		W[count] = source[bias + count];
	}
}

template<int ia, int ib, int ic, int oc>
void Conv3x3<ia, ib, ic, oc>::ReadO(int* dest, int bias) {
	int n_o = oa * ob * oc;
	for (int count = 0; count < n_o; count++) {
		dest[bias + count] = O[count];
	}
}

template<int ia, int ib, int ic, int oc>
void Conv3x3<ia, ib, ic, oc>::LoadX(SRAM_CIM& sram_cim) {
	const unsigned int* row_max = Four_CIMs::row_max;
	const unsigned int* col_max = Four_CIMs::col_max;
	const unsigned int* row_num = Four_CIMs::row_num;
	const unsigned int* col_num = Four_CIMs::col_num;
	//unsigned int load[4 * N_CIM_ROW] = { 0 };
	unsigned int n_a_kernel = ka * kb * kc;
	// 将一个kernel对应的地方, 分区塞入
	for (int count0 = 0; count0 < n_a_kernel; count0 += row_max[chip_mode]) {
		for (int row = 0; row < ia; row++) {
			for (int col = 0; col < ib; col++) {
				// 取对应区域下来
				unsigned int area[3 * 3 * KERNEL_C_MAX] = { 0 };
				int row_index = row - 1 + count0 / (kc * kb);
				int col_index = col - 1 + (count0 / kc) % kb;
				int dep_index = count0 % kc;
				int count1 = 0;
				while (count1 < row_max[chip_mode]) {
					if (row_index != -1 && col_index != -1 && row_index != ia && col_index != ib) {
						area[count1] = X[row_index * ib * ic + col_index * ic + dep_index];
					}
					else {
						area[count1] = 0;
					}
					count1++;
					int i = count1 + count0;
					if (i % kc != 0) { dep_index++; }
					else {
						if (i % (kc * kb) != 0) { col_index++; }
						else {
							row_index++;
							col_index = col - 1;
						}
						dep_index = 0;
					}
				}
				// 把对应区域的一部分放进来
				/*for (int count2 = 0; count2 < row_max[chip_mode]; count2++) {
					if (count2 < n_a_kernel) {
						load[count2] = area[count0 + count2];
					}
					else {
						load[count2] = 0;
					}
				}*/
				sram_cim.WriteToInput(chip_mode, area);
			}
		}
	}
}

template<int ia, int ib, int ic, int oc>
int Conv3x3<ia, ib, ic, oc>::GetNextIndex(int times, int index, int row, int col) {
	int res = index;
	for (int count = 0; count < times; count++) {
		res = GetNextXInBlock(res, row, col);
	}
	return res;
}

template<int ia, int ib, int ic, int oc>
void Conv3x3<ia, ib, ic, oc>::LoadW(SRAM_CIM& sram_cim) {
	const unsigned int* row_max = Four_CIMs::row_max;
	const unsigned int* col_max = Four_CIMs::col_max;
	const unsigned int* row_num = Four_CIMs::row_num;
	const unsigned int* col_num = Four_CIMs::col_num;
	unsigned int n_a_kernel = ka * kb * kc;
	// 分批次放入这么多kernel
	for (int count0 = 0; count0 < kd; count0 += col_max[chip_mode]) {
		// 分区域放入kernel
		for (int count1 = 0; count1 < n_a_kernel; count1 += row_max[chip_mode]) {
			// 更新一次W
			for (int row = 0; row < row_max[chip_mode]; row++) {
				int index = count1 + row;
				for (int col = 0; col < col_max[chip_mode]; col++) {
					int block = count0 + col;
					if (block < kd && index < n_a_kernel) {
						load[row * col_max[chip_mode] + col] = W[block * ka * kb * kc + index];
					}
					else {
						load[row * col_max[chip_mode] + col] = { 0u };
					}
				}
			}
			// 预处理
			for (int x = 0; x < row_max[chip_mode]; x++) {
				for (int y = 0; y < col_max[chip_mode]; y++) {
					int a = x % N_CIM_ROW;
					int b = y % N_CIM_COL;
					int c = (x / N_CIM_ROW) * col_num[chip_mode] + y / N_CIM_COL;
					load1[c * N_CIM_ROW * N_CIM_COL + a * N_CIM_COL + b] = load[x * col_max[chip_mode] + y];
				}
			}
			sram_cim.WriteWeight(chip_mode, load1);
		}
	}
}

template<int ia, int ib, int ic, int oc>
void Conv3x3<ia, ib, ic, oc>::UpdateO(SRAM_CIM& sram_cim) {
	const unsigned int* col_max = Four_CIMs::col_max;
	int n_o = oa * ob * oc;
	sram_cim.Setting(chip_mode, chip_add_step, chip_add_turn, chip_add_times, chip_cal_step);
	sram_cim.Calculate();
	int o_z_chip = (sram_cim.Tail_Result / (oa * ob));
	for (int s = 0; s < sram_cim.Tail_Result / (oa * ob * col_max[chip_mode]); s++) {
		for (int x = 0; x < oa; x++) {
			for (int y = 0; y < ob; y++) {
				for (int z = 0; z < col_max[chip_mode]; z++) {
					int z1 = s * col_max[chip_mode] + z;
					unload[x * ob * o_z_chip + y * o_z_chip + z1] =
						sram_cim.P_SRAM[sram_cim.I_Result + s * oa * ob * col_max[chip_mode] +
						x * ob * col_max[chip_mode] + y * col_max[chip_mode] + z];
				}
			}
		}
	}
	for (int x = 0; x < oa; x++) {
		for (int y = 0; y < ob; y++) {
			for (int z = 0; z < oc; z++) {
				O[x * ob * oc + y * oc + z] = unload[x * ob * o_z_chip + y * o_z_chip + z];
			}
		}
	}
}

// public---------------------------------------------------------------------------
template<int ia, int ib, int ic, int oc>
Conv3x3<ia, ib, ic, oc>::Conv3x3() {
	int n_a_kernel = 3 * 3 * kc;
	const unsigned int* row_max = Four_CIMs::row_max;
	const unsigned int* col_max = Four_CIMs::col_max;
	if (n_a_kernel <= row_max[0u]) {
		chip_mode = 0u;
	}
	else if (row_max[0u] < n_a_kernel && n_a_kernel <= row_max[1u]) {
		chip_mode = 1u;
	}
	else {
		chip_mode = 2u;
	}
	int n_index = !(n_a_kernel % row_max[chip_mode]) ? n_a_kernel / row_max[chip_mode] : n_a_kernel / row_max[chip_mode] + 1u;
	int n_block = !(kd % col_max[chip_mode]) ? kd / col_max[chip_mode] : kd / col_max[chip_mode] + 1u;
	chip_cal_step = ia * ib * row_max[chip_mode];
	chip_add_step = ia * ib * col_max[chip_mode];
	chip_add_turn = n_index;
	chip_add_times = n_block;
}

template<int ia, int ib, int ic, int oc>
void Conv3x3<ia, ib, ic, oc>::Conv(int* sourceX, int biasX, int* sourceW, int biasW, int* destO, int biasO, SRAM_CIM& sram_cim) {
	SetX(sourceX, biasX);
	SetW(sourceW, biasW);
	LoadX(sram_cim);
	LoadW(sram_cim);
	UpdateO(sram_cim);
	ReadO(destO, 0);
}

/*---------------------------------------FullConnect----------------------------------------*/
// private----------------------------------------------------------------------------------
template<int ni, int no>
void FullConnect<ni, no>::SetI(int* source, int bias) {
	for (int count = 0; count < ni; count++) {
		I[count] = source[bias + count];
	}
}

template<int ni, int no>
void FullConnect<ni, no>::SetW(int* source, int bias) {
	int nw = ni * no;
	for (int count = 0; count < nw; count++) {
		W[count] = source[bias + count];
	}
}

template<int ni, int no>
void FullConnect<ni, no>::ReadO(int* dest, int bias) {
	for (int count = 0; count < no; count++) {
		dest[bias + count] = O[count];
	}
}


template<int ni, int no>
void FullConnect<ni, no>::LoadI(SRAM_CIM& sram_cim) {
	const unsigned int* row_max = Four_CIMs::row_max;
	const unsigned int* col_max = Four_CIMs::col_max;
	for (int count0 = 0; count0 < ni; count0 += row_max[chip_mode]) {
		for (int row = 0; row < row_max[chip_mode]; row++) {
			if (count0 + row < ni) {
				load[row] = I[count0 + row];
			}
			else {
				load[row] = 0;
			}
		}
		sram_cim.WriteToInput(chip_mode, load);
	}
}

template<int ni, int no>
void FullConnect<ni, no>::LoadW(SRAM_CIM& sram_cim) {
	const unsigned int* row_max = Four_CIMs::row_max;
	const unsigned int* col_max = Four_CIMs::col_max;
	const unsigned int* row_num = Four_CIMs::row_num;
	const unsigned int* col_num = Four_CIMs::col_num;
	for (int count0 = 0; count0 < no; count0 += col_max[chip_mode]) {
		for (int count1 = 0; count1 < ni; count1 += row_max[chip_mode]) {
			for (int row = 0; row < row_max[chip_mode]; row++) {
				int index = count1 + row;
				for (int col = 0; col < col_max[chip_mode]; col++) {
					int block = count0 + col;
					if (block < no && index < ni) {
						load[row * col_max[chip_mode] + col] = W[block * ni + index];
					}
					else {
						load[row * col_max[chip_mode] + col] = 0;
					}
				}
			}
			// 预处理
			for (int x = 0; x < row_max[chip_mode]; x++) {
				for (int y = 0; y < col_max[chip_mode]; y++) {
					int a = x % N_CIM_ROW;
					int b = y % N_CIM_COL;
					int c = (x / N_CIM_ROW) * col_num[chip_mode] + y / N_CIM_COL;
					load1[c * N_CIM_ROW * N_CIM_COL + a * N_CIM_COL + b] = load[x * col_max[chip_mode] + y];
				}
			}
			sram_cim.WriteWeight(chip_mode, load1);
		}
	}
}

template<int ni, int no>
void FullConnect<ni, no>::UpdateO(SRAM_CIM& sram_cim) {
	const unsigned int* row_max = Four_CIMs::row_max;
	const unsigned int* col_max = Four_CIMs::col_max;
	sram_cim.Setting(chip_mode, chip_add_step, chip_add_turn, chip_add_times, chip_cal_step);
	sram_cim.Calculate();
	sram_cim.ReadOutput(chip_mode, unload);
	for (int count = 0; count < no; count++) {
		O[count] = unload[count];
	}
}

// public-----------------------------------------------------------------------------------
template<int ni, int no>
FullConnect<ni, no>::FullConnect() {
	const unsigned int* row_max = Four_CIMs::row_max;
	const unsigned int* col_max = Four_CIMs::col_max;
	if (ni <= row_max[0u]) {
		chip_mode = 0u;
	}
	else if (row_max[0u] < ni && ni <= row_max[1u]) {
		chip_mode = 1u;
	}
	else {
		chip_mode = 2u;
	}
	chip_cal_step = row_max[chip_mode];
	int n_index = !(ni % row_max[chip_mode]) ? ni / row_max[chip_mode] : ni / row_max[chip_mode] + 1;
	int n_block = !(no % col_max[chip_mode]) ? no / col_max[chip_mode] : no / col_max[chip_mode] + 1;
	chip_add_step = col_max[chip_mode];
	chip_add_turn = n_index;
	chip_add_times = n_block;
}

template<int ni, int no>
void FullConnect<ni, no>::FC(int* sourceI, int biasI, int* sourceW, int biasW, int* destO, int biasO, SRAM_CIM& sram_cim) {
	SetI(sourceI, biasI);
	SetW(sourceW, biasW);
	LoadI(sram_cim);
	LoadW(sram_cim);
	UpdateO(sram_cim);
	ReadO(destO, biasO);
}

/*---------------------------------------MaxPool2x2----------------------------------------*/

// private-----------------------------------------------------------------------------

template<int ia, int ib, int ic>
void MaxPool<ia, ib, ic>::SetX(int* arr, int bias) {
	for (int count = 0; count < ia * ib * ic; count++) {
		X[count] = arr[bias + count];
	}
}

template<int ia, int ib, int ic>
void MaxPool<ia, ib, ic>::ReadO(int* arr, int bias) const {
	for (int count = 0; count < oa * ob * oc; count++) {
		arr[bias + count] = O[count];
	}
}

template<int ia, int ib, int ic>
void MaxPool<ia, ib, ic>::Pooling() {
	for (int o1 = 0; o1 < oa; o1++) {
		int i1 = o1 * 2;
		int i1_ = o1 * 2 + 1;
		for (int o2 = 0; o2 < ib; o2++) {
			for (int o3 = 0; o3 < ic; o3++) {
				M[o1 * ib * ic + o2 * ic + o3] =
					X[i1 * ib * ic + o2 * ic + o3] > X[i1_ * ib * ic + o2 * ic + o3] ?
					X[i1 * ib * ic + o2 * ic + o3] : X[i1_ * ib * ic + o2 * ic + o3];
			}
		}
	}
	for (int o2 = 0; o2 < ob; o2++) {
		int i2 = o2 * 2;
		int i2_ = o2 * 2 + 1;
		for (int o1 = 0; o1 < oa; o1++) {
			for (int o3 = 0; o3 < oc; o3++) {
				O[o1 * ob * oc + o2 * oc + o3] =
					M[o1 * ib * ic + i2 * ic + o3] > M[o1 * ib * ic + i2_ * ic + o3] ?
					M[o1 * ib * ic + i2 * ic + o3] : M[o1 * ib * ic + i2_ * ic + o3];
			}
		}
	}
}


// public------------------------------------------------------------------------------

template<int ia, int ib, int ic>
void MaxPool<ia, ib, ic>::MP(int* sourceX, int biasX, int* destO, int biasO) {
	SetX(sourceX, biasX);
	Pooling();
	ReadO(destO, biasO);
}


/*---------------------------------------Model----------------------------------------*/

bool Model::Writer(unsigned int id_type, unsigned int n, unsigned int* arr, unsigned int bias) {
	if (n >= N_TYPE_MAX || (id_type != ID_INPUT && id_type != ID_OUTPUT && id_type != ID_KERNELS)) {
		return false;
	}
	for (unsigned int count = 0u; count < n; count++) {
		m_SRAM[id_type + count] = arr[bias + count];
	}
	return true;
}

bool Model::Reader(unsigned int id_type, unsigned int n, unsigned int* arr, unsigned int bias) const {
	if (n >= N_TYPE_MAX || (id_type != ID_INPUT && id_type != ID_OUTPUT && id_type != ID_KERNELS)) {
		return false;
	}
	for (unsigned int count = 0u; count < n; count++) {
		arr[bias + count] = m_SRAM[id_type + count];
	}
	return true;
}

template class Conv3x3<3, 3, 257, 10>;
//template class FullConnect<2000, 300>;
//template class MaxPool<20, 20, 10>;