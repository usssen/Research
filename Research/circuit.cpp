#include<iostream>
#include<fstream>
#include<sstream>
#include<math.h>
#include "circuit.h"
#include<stdlib.h>
#include<algorithm>

using namespace std;

extern vector<CIRCUIT> Circuit;
extern vector<PATH> PathR;
extern vector<PATH*> PathC;
extern double **EdgeA;
extern double **EdgeB;
extern double **cor;
extern double **ser;
extern double info[5];
double period;
extern double ERROR;

double absff(double x){
	if (x < 0)
		return -x;
	return x;
}

double TransStringToDouble(string s){
	stringstream ss;
	ss << s;
	double result;
	ss >> result;
	return result;
}

string RemoveSpace(string s){
	unsigned i;
	for (i = 0; i < s.length(); i++)
	if (s[i] != ' ' && s[i] != 9 && s[i] != 13)		//9������tab,13������
		break;
	s = s.substr(i);
	return s;
}

void CIRCUIT::PutClockSource(){
	GATE* gptr = new GATE("ClockSource", "PI");
	gptr->SetInTime(0);
	gptr->SetOutTime(0);
	PutGate(gptr);
}

void ReadCircuit(string filename){		//Ūnetlist
	fstream file;
	file.open(filename.c_str(), ios::in);
	char temp[1000];
	bool cmt = false;
	int Nowmodule = -1;
	while (file.getline(temp, 1000)){		//�@��h�өR�O�L�k�B�z
		string temps = temp;
		if (cmt){
			if (temps.find("*/") != string::npos)
				cmt = false;
			continue;
		}
		if (temps.find("/*") != string::npos){
			if (temps.find("*/") == string::npos)
				cmt = true;
			temps = temps.substr(0, temps.find("/*"));
		}
		if (temps.find("//") != string::npos)
			temps = temps.substr(0, temps.find("//"));
		temps = RemoveSpace(temps);
		if (temps.empty())
			continue;
		if (temps.find("endmodule") != string::npos)
			continue;
		else if (temps.find("assign") != string::npos)
			continue;
		else if (temps.find("module") != string::npos){
			int st = temps.find("module") + 7;
			CIRCUIT TC(temps.substr(st, temps.find(" (") - st));
			Circuit.push_back(TC);
			Nowmodule++;
			while (file.getline(temp, 1000)){
				temps = temp;
				if (temps.find(")") != string::npos)
					break;
			}
		}
		else if (temps.find("input") != string::npos){
			int st = temps.find("input") + 6;
			WIRE* w = new WIRE(temps.substr(st, temps.find(";") - st), PI);
			Circuit[Nowmodule].PutWire(w);
		}
		else if (temps.find("output") != string::npos){
			int st = temps.find("output") + 7;
			WIRE* w = new WIRE(temps.substr(st, temps.find(";") - st), PO);
			Circuit[Nowmodule].PutWire(w);

		}
		else if (temps.find("wire") != string::npos){
			int st = temps.find("wire") + 5;
			WIRE* w = new WIRE(temps.substr(st, temps.find(";") - st), INN);
			Circuit[Nowmodule].PutWire(w);
		}
		else{
			temps = RemoveSpace(temps);
			if (temps.empty())	continue;
			bool ok = false;
			for (int i = 0; i < Nowmodule; i++){
				if (Circuit[i].GetName() == temps.substr(0, temps.find(" "))){
					//�i��A�[�J�ǥѤ��eŪ�J��module(�Dlibrary)�ӫإ�gate,in/output���W�٤��ΰO��(�bmodule����),���n�ΥH���쥿�T���f
					ok = true;
					break;
				}
			}
			if (!ok){	//���]�Ĥ@�Ӭ�output,�䥦��input
				string moduleN = temps.substr(0, temps.find(" "));	//module name,gate name,gate's output�|�g�b�@��
				temps = temps.substr(temps.find(" ") + 1);
				string gateN = temps.substr(0, temps.find(" ("));
				temps = temps.substr(temps.find(" ") + 2);
				GATE* g = new GATE(gateN, moduleN);
				int st = temps.find("(");
				string ioN = temps.substr(st + 1, temps.find(")") - st - 1);
				g->SetOutput(Circuit[Nowmodule].GetWire(ioN));
				Circuit[Nowmodule].GetWire(ioN)->SetInput(g);
				temps = temps.substr(temps.find(")") + 1);
				while (file.getline(temp, 1000)){		//gate's input �|�b�᭱�C��g�@��
					temps = temp;
					st = temps.find("(");
					string ioN = temps.substr(st + 1, temps.find(")") - st - 1);		//�|���u����1'b1,1'b0(�`��)
					g->SetInput(Circuit[Nowmodule].GetWire(ioN));
					Circuit[Nowmodule].GetWire(ioN)->SetOutput(g);
					//temps = temps.substr(temps.find(")") + 1);
					if (temps.find(";") != string::npos)	break;
				}
				Circuit[Nowmodule].PutGate(g);
			}
		}

		//system("pause");
	}
	file.close();
	return;
}

void ReadPath_l(string filename){	//Ūpath report
	fstream file;
	file.open(filename.c_str(), ios::in);
	string line, sp, ep;
	GATE *gptr = NULL, *spptr = NULL, *epptr = NULL;
	PATH* p = NULL;
	unsigned Path_No = 0;
	while (getline(file, line)){
		//cout << line << endl;
		if (line.find("Startpoint") != string::npos){
			if (PathR.size() >= MAXPATHS)
				return;
			p = new PATH();
			sp = line.substr(line.find("Startpoint") + 12);
			sp = sp.substr(0, sp.find(" "));
			spptr = Circuit[0].GetGate(sp);	//0��top-module
			if (spptr == NULL)	//�_�I��PI
				spptr = new GATE(sp, "PI");
		}
		else if (line.find("Endpoint") != string::npos){
			ep = line.substr(line.find("Endpoint") + 10);
			ep = ep.substr(0, ep.find(" "));
			epptr = Circuit[0].GetGate(ep);
			if (epptr == NULL)
				epptr = new GATE(ep, "PO");
		}

		if (line.find("---") == string::npos || sp == "")	continue;
		if (spptr->GetType() == "PI" && epptr->GetType() == "PO"){
			Path_No++;
			while (line.find("slack (MET)") == string::npos)	getline(file, line);
			continue;
		}
		getline(file, line);
		getline(file, line);		//�o2�檺�|�����b�Ĥ@��gate�ɶ��W ���ΰO
		//if (spptr->ClockLength() == 0 && spptr->GetType()!="PI"){
		if (spptr->GetType() != "PI"){
			while (getline(file, line)){	//clock-source -> startpoint			
				line = RemoveSpace(line);
				if (sp == line.substr(0, line.find("/")))	break;
				if (line.find("(net)") != string::npos)	continue;
				else if (line.find("(in)") != string::npos){	//PI�ɶ����p,�p�G���~������᭱���R�A�[�J
					spptr->SetClockPath(Circuit[0].GetGate("ClockSource"));
				}
				//else if (line.find("(out)") != string::npos){}
				else{
					string name = line.substr(0, line.find("/"));
					double intime = TransStringToDouble(line.substr(line.find("&") + 1));
					getline(file, line);
					double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
					gptr = Circuit[0].GetGate(name);
					spptr->SetClockPath(gptr);
					gptr->SetInTime(intime);
					gptr->SetOutTime(outtime);
				}
			}
		}
		if (spptr->GetType() == "PI"){		//�_�I��PI�����p
			while (line.find("(in)") == string::npos)	getline(file, line);
			p->AddGate(spptr, 0, TransStringToDouble(line.substr(line.find("&") + 1)));		//clock �� �_�I���ɶ���0 (PI),tcq = �~��delay
			getline(file, line);
		}
		do{
			line = RemoveSpace(line);
			if (ep == line.substr(0, line.find("/")) || line.find("(out)") != string::npos)	break;
			if (line.find("(net)") != string::npos)	continue;
			string name = line.substr(0, line.find("/"));
			double intime = TransStringToDouble(line.substr(line.find("&") + 1));
			getline(file, line);
			double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
			gptr = Circuit[0].GetGate(name);
			p->AddGate(gptr, intime, outtime);
		} while (getline(file, line));

		p->AddGate(epptr, TransStringToDouble(line.substr(line.find("&") + 1)), -1);	//arrival time

		getline(file, line);
		while (line.find("edge)") == string::npos)	getline(file, line);	//��clock [clock source] (rise/fall edge)
		if (period < 1)
			period = TransStringToDouble(line.substr(line.find("edge)") + 5));

		//�blong path�� ���I���ɶ����O�[�J�F�@��period�����p �n��h �]���᭱�|��ΧO��Tc'
		if (epptr->GetType() == "PO"){
			while (line.find("output external delay") == string::npos)	getline(file, line);
			double delay = TransStringToDouble(line.substr(line.find("-") + 1));
			p->SetCTE(0.0);
			p->SetST(delay);	//PO��setup time���~��delay
		}
		else{
			while (line.find("clock source latency") == string::npos) getline(file, line);
			//if (epptr->ClockLength() == 0){
			while (getline(file, line)){
				line = RemoveSpace(line);
				if (ep == line.substr(0, line.find("/"))){
					double cte = TransStringToDouble(line.substr(line.find("&") + 1));
					p->SetCTE(cte - period);		//�o�䤴�O��+1��period
					break;
				}
				if (line.find("(net)") != string::npos)	continue;
				else if (line.find("(in)") != string::npos){
					epptr->SetClockPath(Circuit[0].GetGate("ClockSource"));
				}
				else{
					string name = line.substr(0, line.find("/"));
					double intime = TransStringToDouble(line.substr(line.find("&") + 1));
					getline(file, line);
					double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
					gptr = Circuit[0].GetGate(name);
					epptr->SetClockPath(gptr);
					gptr->SetInTime(intime - period);		//�ɮפ���source -> ff���ɶ� + 1��period �ݮ��h
					gptr->SetOutTime(outtime - period);
				}
			}
			//}
			while (line.find("setup") == string::npos)	getline(file, line);
			double setup = TransStringToDouble(line.substr(line.find("-") + 1));
			p->SetST(setup);
		}
		spptr->Setflag();
		epptr->Setflag();
		p->SetType(LONG);
		p->SetNo(Path_No++);
		if (p->length()>2)		//������gate ���O�����s
			PathR.push_back(*p);
		sp = "";
	}
	file.close();
}

void ReadCpInfo(string filename){		//Ū���p�ʩM�j�k�u
	fstream file;
	file.open(filename.c_str());
	map<unsigned, unsigned> mapping;	//��s��(�S���h��PI->PO & NO_GATE��) -> PathC���s��
	for (int i = 0; i < PathC.size(); i++){
		mapping[PathC[i]->No()] = i;
	}
	int im, jn;
	double a, b, cc, err;
	string line;
	getline(file, line);
	while (file >> im >> jn){			// aging(j) = aging(i)*EdgeA[i][j] + EdgeB[i][j]
		file >> line;
		if (line == "nan"){				//�ײv�L�a�j => x = const => �L�k�� =>���L
			a = b = err = 10000;
			cc = 0;
			file >> line;
			file >> line;
			file >> line;
		}
		else{
			a = atof(line.c_str());
			file >> b;
			file >> line;
			if (line == "nan")			//y = b => �ײv��0 =>y���зǮt��0 =>�����Y�ƵL�a�j
				cc = 0;
			else
				cc = atof(line.c_str());
			file >> err;
		}
		if (mapping.find(im) == mapping.end() || mapping.find(jn) == mapping.end())
			continue;
		int ii = mapping[im], jj = mapping[jn];
		EdgeA[ii][jj] = a;
		EdgeB[ii][jj] = b;
		cor[ii][jj] = cc;
		ser[ii][jj] = err;
	}
	file.close();
}

void CalPreInv(double x, double &upper, double &lower, int a, int b, double year){		//�p��w���϶�,y = ax+b �O��>100%(�[�W�쥻����)�h��
	if (EdgeA[a][b] > 9999){
		upper = lower = 10000;
		return;
	}
	double dis = 1.96;	//+-�h�֭ӼзǮt 90% 1.65 95% 1.96 99% 2.58
	double y1 = EdgeA[a][b] * (x + 1) + EdgeB[a][b] * ((1 + AgingRate(WORST, year)) / (1 + AgingRate(WORST, 10)));
	upper = y1 + ser[a][b] * dis* ((1 + AgingRate(WORST, year)) / (1 + AgingRate(WORST, 10))) - 1;
	lower = y1 - ser[a][b] * dis* ((1 + AgingRate(WORST, year)) / (1 + AgingRate(WORST, 10))) - 1;		//���Ӥ�ҽվ�
}

double CalPreAging(double x, int a, int b, double year){		//�p��w����
	if (EdgeA[a][b] > 9999)
		return 10000;
	return EdgeA[a][b] * (x + 1) + EdgeB[a][b] * ((1 + AgingRate(WORST, year)) / (1 + AgingRate(WORST, 10))) - 1;		//���Ӥ�ҽվ�
}
//�p��timing, �p�G�S���H�Ϭ�true, ��J�Opath����, dcc��b�_/���I���ĴX��clock buffer, �񪺫��A, �ɶ�
bool Vio_Check(PATH* pptr, int stn, int edn, AGINGTYPE ast, AGINGTYPE aed, double year){
	GATE* stptr = pptr->Gate(0);
	GATE* edptr = pptr->Gate(pptr->length() - 1);
	double clks = 0.0;
	if (stptr->GetType() != "PI"){
		clks = pptr->GetCTH();	//�Ѥƫ� = �쥻 + �C��gate delay x �ѤƲv **�o�I�n�ץ� wire�]���ݭp��Ѥ�
		double smallest = stptr->GetClockPath(1)->GetOutTime() - stptr->GetClockPath(1)->GetInTime();
		for (int i = 2; i < stptr->ClockLength(); i++)
			if (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime() < smallest)
				smallest = stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime();
		for (int i = 0; i < stn; i++)
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		for (int i = stn; i < stptr->ClockLength(); i++)
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(ast, year);
		switch (ast){
		case DCC_S:				//S��20%, M��40%, F��80% DCC
		case DCC_M:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}
	double clkt = 0.0;
	if (edptr->GetType() != "PO"){
		clkt = pptr->GetCTE();
		double smallest = edptr->GetClockPath(1)->GetOutTime() - edptr->GetClockPath(1)->GetInTime();
		for (int i = 2; i < edptr->ClockLength(); i++)
		if (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime() < smallest)
			smallest = edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime();
		for (int i = 0; i < edn; i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		for (int i = edn; i < edptr->ClockLength(); i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(aed, year);
		switch (aed){
		case DCC_S:
		case DCC_M:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}
	double Tcq = (pptr->Out_time(0) - pptr->In_time(0));
	if (stptr->GetType() != "PI")
		Tcq *= (AgingRate(FF, year) + 1.0);
	double DelayP = pptr->GetPathDelay();
	//for (int i = 1; i < pptr->length() - 1; i++)		//�e�᪺ff/PO/PI���κ�
	//	DelayP += (pptr->Out_time(i) - pptr->In_time(i))*AgingRate(NORMAL,year);
	DelayP += DelayP*AgingRate(NORMAL, year);
	if (pptr->GetType() == LONG){
		if (clks + Tcq + DelayP < clkt - pptr->GetST() + period)
			return true;
		return false;
	}
	else{
		if (clks + Tcq + DelayP>clkt + pptr->GetHT())
			return true;
		return false;
	}
}
//aging_p�O�ѤƲv
bool Vio_Check(PATH* pptr, double year, double Aging_P){

	GATE* stptr = pptr->Gate(0);
	GATE* edptr = pptr->Gate(pptr->length() - 1);
	int ls = stptr->ClockLength();
	int le = edptr->ClockLength();
	double clks = 0.0;
	if (stptr->GetType() != "PI"){
		clks = pptr->GetCTH();
		double smallest = stptr->GetClockPath(1)->GetOutTime() - stptr->GetClockPath(1)->GetInTime();
		for (int i = 2; i < stptr->ClockLength(); i++)
		if (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime() < smallest)
			smallest = stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime();
		AGINGTYPE DCC_insert = DCC_NONE;
		int i;
		for (i = 0; i < ls && stptr->GetClockPath(i)->GetDcc() == DCC_NONE; i++){
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		}
		if (i < ls)
			DCC_insert = stptr->GetClockPath(i)->GetDcc();
		for (; i < ls; i++){
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_insert, year);
		}
		switch (DCC_insert){
		case DCC_S:
		case DCC_M:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}

	double clkt = 0.0;
	if (edptr->GetType() != "PO"){
		clkt = pptr->GetCTE();
		double smallest = edptr->GetClockPath(1)->GetOutTime() - edptr->GetClockPath(1)->GetInTime();	//���tclock-source
		for (int i = 2; i < edptr->ClockLength(); i++)
		if (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime() < smallest)
			smallest = edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime();
		int i;
		AGINGTYPE DCC_insert = DCC_NONE;
		for (i = 0; i < le && edptr->GetClockPath(i)->GetDcc() == DCC_NONE; i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		if (i < le)
			DCC_insert = edptr->GetClockPath(i)->GetDcc();
		for (; i < le; i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_insert, year);
		switch (DCC_insert){
		case DCC_S:
		case DCC_M:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}
	double Tcq = (pptr->Out_time(0) - pptr->In_time(0));
	if (stptr->GetType() != "PI")
		Tcq *= (AgingRate(FF, year) + 1.0);
	double DelayP = pptr->GetPathDelay();
	DelayP += DelayP*Aging_P;
	if (pptr->GetType() == LONG){
		if (clks + Tcq + DelayP < clkt - pptr->GetST() + period)
			return true;
		return false;
	}
	else{
		if (clks + Tcq + DelayP>clkt + pptr->GetHT())
			return true;
		return false;
	}
}

inline double absl(double x){
	if (x < 0)
		return -x;
	return x;
}

double thershold = 0.8;	//R����
double errlimit = 0.01;	//�ѤƮt(��W�@�U)

void AdjustConnect(){	//Ū�H�W��ӭ�
	fstream ff;
	ff.open("Parameter.txt", ios::in);
	string line;
	while (getline(ff, line)){
		if (line.find("thershold") != string::npos){
			thershold = atof(line.c_str() + 9);
		}
		if (line.find("edge error") != string::npos){
			errlimit = atof(line.c_str() + 10);
		}
	}
	ff.close();
}

bool Check_Connect(int a, int b, double year){
	if (EdgeA[a][b] > 9999 || EdgeA[a][b] * EdgeA[a][b] < 0.000001)
		return false;
	if (cor[a][b]<0)	//�t����
		return false;
	if ((cor[a][b] * cor[a][b])<thershold)		//�����Y�ƭn�W�Lthershold�~��������
		return false;
	if (absl(CalPreAging(AgingRate(WORST, year), a, b, year) - AgingRate(WORST, year))>errlimit)
		return false;
	return true;
}

struct PN_W{
	int pn;
	double w;
	PN_W(int n, double ww) :pn(n), w(ww){}
};

bool PN_W_comp(PN_W a, PN_W b){
	if (a.w > b.w)
		return true;
	return false;
}

class HASHTABLE{
private:
	bool* exist;
	bool** choose;
	unsigned size;
public:
	HASHTABLE(unsigned s1, unsigned s2){	//s1�Ohash table������(bit),s2�O�����j�p
		size = s1;
		exist = new bool[1 << s1];
		choose = new bool*[1 << s1];
		for (int i = 0; i < (1 << s1); i++){
			exist[i] = false;
			choose[i] = new bool[s2];
		}
	}
	unsigned CalKey(){
		unsigned key = 0x0;
		unsigned temp = 0x0;
		for (int i = 0; i < PathC.size(); i++){
			temp <<= 1;
			if (PathC[i]->Is_Chosen())
				temp++;
			if ((i + 1) % size == 0){
				key ^= temp;
				temp = 0x0;
			}
		}
		key ^= temp;
		return key;
	}
	bool Exist(){
		unsigned key = CalKey();
		if (!exist[key])
			return false;
		for (int i = 0; i < PathC.size(); i++){
			if (PathC[i]->Is_Chosen() != choose[key][i])
				return false;
		}
		return true;
	}
	void PutNowStatus(){
		unsigned key = CalKey();
		exist[key] = true;
		for (int i = 0; i < PathC.size(); i++)
			choose[key][i] = PathC[i]->Is_Chosen();
	}
};

double Overlap(int p){		//�p��FF�Y�����|�v => ���̤j��=> �p�����Ȧ��Y/����? => ��������\ �����[�@�ӭ�
	double max = 0;
	PATH* pptr = PathC[p];
	GATE* stptr = pptr->Gate(0);
	GATE* edptr = pptr->Gate(pptr->length() - 1);
	for (int i = 0; i < PathC.size(); i++){
		if (!PathC[i]->Is_Chosen() || !PathC[i]->CheckAttack())
			continue;
		PATH* iptr = PathC[i];
		double score, s1, s2;
		if (stptr->GetType() != "PI"){
			GATE* gptr = iptr->Gate(iptr->length() - 1);
			int c = 0;
			while (c < stptr->ClockLength() && c < gptr->ClockLength()){
				if (stptr->GetClockPath(c) != gptr->GetClockPath(c))
					break;
				c++;
			}
			s1 = (double)c / (double)stptr->ClockLength();
		}
		else{
			s1 = 1;		//�Y�@�䬰FF �����������| (����G��FF�t)
		}

		if (edptr->GetType() != "PO"){
			GATE* gptr = iptr->Gate(0);
			int c = 0;
			while (c < edptr->ClockLength() && c < gptr->ClockLength()){
				if (edptr->GetClockPath(c) != gptr->GetClockPath(c))
					break;
				c++;
			}
			s2 = (double)c / (double)edptr->ClockLength();
		}
		else{
			s2 = 1;		//�����p�U���ݭ��|�׳]1,�����t
		}

		if (s1>s2)
			score = s1 * 2 + s2;
		else
			score = s2 * 2 + s1;

		//score = s1 + s2;

		if (max < score)
			max = score;
	}
	return max;
}

double AtkPointRate(PATH* pptr){		//�p��ۨ������|�v
	GATE* stptr = pptr->Gate(0);
	GATE* edptr = pptr->Gate(pptr->length() - 1);
	if (stptr->GetType() == "PI" || edptr->GetType() == "PO")
		return 0.75;
	int c = 0;
	while (c < stptr->ClockLength() && c < edptr->ClockLength()){
		if (stptr->GetClockPath(c) != edptr->GetClockPath(c))
			break;
		c++;
	}
	return (double)c * 2 / (double)(stptr->ClockLength() + edptr->ClockLength());
}

bool ChooseVertexWithGreedyMDS(double year, bool puthash){	//��shortlist, �^�ǭ�true��ܬ�domination set

	static HASHTABLE hashp(16, PathC.size());				//puthash��ܱN�W���諸�I�[�Jhash(�������I)
	if (puthash){
		hashp.PutNowStatus();
		return true;
	}
	
	int No_node = PathC.size();
	int *degree = new int[No_node], *color = new int[No_node];
	int cc = 0;
	for (int i = 0; i < No_node; i++){
		PathC[i]->SetChoose(false);
		degree[i] = 0;
		if (!PathC[i]->CheckAttack()){
			color[i] = -1;
			continue;
		}
		color[i] = 1;
		for (int j = 0; j < No_node; j++){				//�`�N�i�঳�bCPInfo���o�����B���o����
			if (Check_Connect(i, j, year) && i != j && PathC[j]->CheckAttack())
				degree[i]++;
		}
	}

	int mini, w_point;
	vector<PN_W> cand;
	while (true){
		for (mini = 0, w_point = 0; mini < PathC.size(); mini++){
			if (color[mini] == 1)
				w_point++;
		}
		if (w_point == 0){
			//break;
			cout << endl << cc << endl;
			return true;
		}
		cand.clear();

		for (int i = 0; i < No_node; i++){
			if (color[i] == -1)				//�ª�����
				continue;
			if (color[i] == 0 && degree[i] == 0)	//�S��degree�����I���n��
				continue;

			PathC[i]->SetChoose(true);		//�dhash��,�Y�s�b�N���L
			if (hashp.Exist()){
				PathC[i]->SetChoose(false);
				continue;
			}

			PathC[i]->SetChoose(false);

			double w = 0;
			//w -= EstimateAddTimes(year, i);	//�[�Ji�I��W�[���Ѯt��	
			//w -= EstimatePSD(i);				//�[�Ji�I��W�[��"���зǮt" (���ⶵ�w�R��)

			w += (double)degree[i] / (double)w_point;	//�[�Ji�I��i��֪����I����
			w -= 1 * Overlap(i);							//�[�J�I�M�䥦�I�����|�v
			w -= 2 * AtkPointRate(PathC[i]);				//�ۨ������|�v
			cand.push_back(PN_W(i, w));
		}
		if (cand.size() == 0){
			//hashp.PutNowStatus();
			//break;
			return false;
		}

		sort(cand.begin(), cand.end(), PN_W_comp);

		if (cc == 0){	//�Ĥ@���I�g����
			int s = 0;
			for (int i = 0; i < cand.size(); i++){
				s += degree[cand[i].pn];
			}
			int target;
			if (s>0)
				target = rand() % s;		//�u���@���I�ɷ|�X�{ s = 0
			else
				target = 0;

			s = 0;
			for (int i = 0; i < cand.size(); i++){
				s += degree[cand[i].pn];
				if (s >= target){
					mini = cand[i].pn;
					break;
				}
			}
		}
		else{
			int ed = 0;											//��degree�̤j��,�p�G�ۦP�N�H��
			while (ed < cand.size()){
				if (absl(cand[ed].w - cand[0].w) < 0.5)
					ed++;
				else
					break;
			}
			mini = cand[rand() % ed].pn;
		}
		for (int i = 0; i < No_node; i++){
			if (mini == i || color[i] == -1)	continue;

			if (Check_Connect(mini, i, year) && color[i] == 1){
				for (int j = 0; j < No_node; j++){
					if (i == j)	continue;
					if (Check_Connect(j, i, year) && color[j] != -1)	//��->��,�����I��degree -1 (���I�w�]��degree = 0 ���L)
						degree[j]--;
				}
				color[i] = 0;	//�Q���I���j���אּ��				
			}
			if (Check_Connect(i, mini, year) && color[mini] == 1){
				degree[i] --;
			}
		}
		PathC[mini]->SetChoose(true);		//�]��true�N���I�Q�襤
		degree[mini] = 0;
		color[mini] = -1;	//�Q���I�אּ��
		cc++;
		cout << PathC[mini]->No() << ' ';
	}
}

map<GATE*, int> cbuffer_code;
map<int, GATE*> cbuffer_decode;

int HashAllClockBuffer(){
	cbuffer_code.clear();
	cbuffer_decode.clear();
	int k = 0;
	for (unsigned i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		if (pptr->IsSafe())	continue;
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		if (stptr->GetType() != "PI"){
			for (int j = 0; j < stptr->ClockLength(); j++)
			if (cbuffer_code.find(stptr->GetClockPath(j)) == cbuffer_code.end()){
				cbuffer_code[stptr->GetClockPath(j)] = k;
				cbuffer_decode[k] = stptr->GetClockPath(j);
				k++;
			}
		}
		if (edptr->GetType() != "PO"){
			for (int j = 0; j < edptr->ClockLength(); j++)
			if (cbuffer_code.find(edptr->GetClockPath(j)) == cbuffer_code.end()){
				cbuffer_code[edptr->GetClockPath(j)] = k;
				cbuffer_decode[k] = edptr->GetClockPath(j);
				k++;
			}
		}
	}
	return k;
}


void CheckPathAttackbility(double year, double margin, bool flag, double PLUS){		//�L�o, �N�I���� �ѦҤU����SAT����
	PathC.clear();
	period = 0.0;
	for (int i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		GATE* stptr = pptr->Gate(0);
		double clkt = pptr->GetCTE(), clks = pptr->GetCTH(), Tcq = pptr->Out_time(0) - pptr->In_time(0);
		for (int i = 0; i < edptr->ClockLength(); i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year + PLUS);
		for (int i = 0; i < stptr->ClockLength(); i++)
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year + PLUS);
		if (stptr->GetType() != "PI")
			Tcq *= (1.0 + AgingRate(FF, static_cast<double>(year + PLUS)));
		double pp = (1 + AgingRate(WORST, static_cast<double>(year + PLUS)))*(pptr->GetPathDelay()) + Tcq + (clks - clkt) + pptr->GetST();
		pp *= margin;
		if (pp>period){
			period = pp;
		}
	}
	if (flag){
		cout << "Clock Period = " << period << endl;
		info[0] = period;
	}
	for (int i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		pptr->SetAttack(false);
		pptr->SetSafe(true);
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		int lst = stptr->ClockLength();
		int led = edptr->ClockLength();
		int branch = 1;
		if (stptr->GetType() == "PI"){
			for (int j = 1; j < led; j++){
				for (int x = 1; x <= 3; x++){
					if (!Vio_Check(pptr, 0, j, DCC_NONE, (AGINGTYPE)x, year + ERROR)){
						pptr->SetSafe(false);
						if (Vio_Check(pptr, 0, j, DCC_NONE, (AGINGTYPE)x, year - ERROR))
							pptr->SetAttack(true);
					}
				}
			}
		}
		else if (edptr->GetType() == "PO"){
			for (int j = 1; j < lst; j++){
				for (int x = 1; x <= 3; x++){
					if (!Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year + ERROR))
						pptr->SetSafe(false);
					if (!Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year + ERROR) && Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year - ERROR))
						pptr->SetAttack(true);
				}
			}
		}
		else{
			while (branch < lst&&branch < led){
				if (stptr->GetClockPath(branch) != edptr->GetClockPath(branch))
					break;
				for (int x = 1; x <= 3; x++){
					if (!Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year + ERROR))
						pptr->SetSafe(false);
					if (!Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year + ERROR) && Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year - ERROR))
						pptr->SetAttack(true);
				}
				branch++;
			}
			for (int j = branch; j < lst; j++){
				for (int k = branch; k < led; k++){
					for (int x = 0; x < 3; x++){
						for (int y = 0; y < 3; y++){
							if (x == 0 && y == 0)	continue;
							if (!Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year + ERROR))
								pptr->SetSafe(false);
							if (!Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year + ERROR) && Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year - ERROR))
								pptr->SetAttack(true);
						}
					}
				}
			}
		}
	}
	int aa, bb, cc, dd;
	aa = bb = cc = dd = 0;
	for (unsigned i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		if (!pptr->IsSafe()){	//pathC������אּmine+candidate
			if (stptr->GetType() == "PI")
				aa++;
			else if (edptr->GetType() == "PO")
				bb++;
			else
				cc++;
			PathC.push_back(pptr);
			if (!pptr->CheckAttack())
				dd++;
		}
	}
	if (flag){
		for (int i = 0; i < PathC.size(); i++){
			if (!PathC[i]->CheckAttack())
				cout << "*";
			cout << PathC[i]->Gate(0)->GetName() << " -> " << PathC[i]->Gate(PathC[i]->length() - 1)->GetName() << " Length = " << PathC[i]->length() << endl;
		}
		cout << aa << ' ' << bb << ' ' << cc << ' ' << dd << endl;
		info[1] = aa, info[2] = bb, info[3] = cc, info[4] = dd;
	}
	return;
}

bool CheckNoVio(double year){
	cout << "Checking Violation... ";
	for (int i = 0; i < PathR.size(); i++){
		if (!Vio_Check(&PathR[i], year, AgingRate(WORST, year))){
			cout << "Path" << i << " Violation!" << endl;
			return false;
		}
	}
	cout << "No Violation!" << endl;
	return true;
}

void GenerateSAT(string filename, double year){
	fstream file;
	fstream temp;
	file.open(filename.c_str(), ios::out);
	map<GATE*, bool> exclusive;
	HashAllClockBuffer();	//�C��clockbuffer���s�����bcbuffer_code�����������X*2+1,*2+2
	GATE* c_source = Circuit[0].GetGate("ClockSource");
	file << '-' << cbuffer_code[c_source] * 2 + 1 << " 0" << endl;
	file << '-' << cbuffer_code[c_source] * 2 + 2 << " 0" << endl;
	for (unsigned i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		if (pptr->IsSafe())	continue;
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		int stn = 0, edn = 0;	//��m�I(����A�]�A�ۨ����|���v�T)
		int lst = stptr->ClockLength();
		int led = edptr->ClockLength();
		if (exclusive.find(stptr) == exclusive.end()){		//DCC constriant, ������Ӧb�P�@clock path
			for (int j = 0; j < lst; j++){
				for (int k = j + 1; k < lst; k++){
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
				}
			}
			exclusive[stptr] = true;
		}
		if (exclusive.find(edptr) == exclusive.end()){
			for (int j = 0; j < led; j++){
				for (int k = j + 1; k < led; k++){
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
				}
			}
			exclusive[edptr] = true;
		}

		if (pptr->Is_Chosen() == false){		//�S���Q��쪺path �[�J���i�L��������			
			if (stptr->GetType() == "PI"){
				for (edn = 0; edn < led; edn++){
					for (int x = 0; x <= 3; x++){	//�N��t�J��DCC���A, �L,20%, 80%, 40%
						if (!Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year - ERROR)){		//�ˬdn-error�~�ݦ��S��violation
							if (x % 2 == 1)		//�@�Ǭ٨ƪ���k�͹�������X...
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
							for (int j = 0; j < led; j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else if (edptr->GetType() == "PO"){
				for (stn = 0; stn < lst; stn++){
					for (int x = 0; x <= 3; x++){
						if (!Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year - ERROR)){
							if (x % 2 == 1)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < lst; j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else{
				stn = edn = 0;
				while (stn < lst && edn < led){		//��b�@�P�ϤW
					if (stptr->GetClockPath(stn) != edptr->GetClockPath(edn))
						break;
					for (int x = 0; x <= 3; x++){	//0/00 NO DCC 1/01 slow aging DCC(20%) 2/10 fast aging DCC(80%)
						if (!Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year - ERROR)){	//Vio_Check�p�G�S��violation�|�^��true
							if (x % 2 == 1)	//01 11 -> 10 00
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)	//10 11
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < stptr->ClockLength(); j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							for (int j = 0; j < edptr->ClockLength(); j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
					stn++;
					edn++;
				}
				int b_point = stn;
				for (; stn < lst; stn++){		//��b�D�@�P��
					for (edn = b_point; edn < led; edn++){
						for (int x = 0; x <= 3; x++){
							for (int y = 0; y <= 3; y++){
								if (!Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year - ERROR)){
									if (x % 2 == 1)	//01 11 -> 1[0] 0[0]
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
									if (x >= 2)	//10 11
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
									if (y % 2 == 1)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
									if (y >= 2)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
									for (int j = 0; j < stptr->ClockLength(); j++){
										if (j == stn)	continue;
										file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									for (int j = 0; j < edptr->ClockLength(); j++){
										if (j == edn)	continue;
										file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									file << 0 << endl;
								}
							}
						}
					}
				}
			}
		}
		else{
			if (stptr->GetType() == "PI"){
				for (edn = 0; edn < led; edn++){
					for (int x = 0; x <= 3; x++){
						if (Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year + ERROR) || !Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year - ERROR)){
							if (x % 2 == 1)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
							for (int j = 0; j < led; j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else if (edptr->GetType() == "PO"){
				for (stn = 0; stn < lst; stn++){
					for (int x = 0; x <= 3; x++){
						if (Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year + ERROR) || !Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year - ERROR)){
							if (x % 2 == 1)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < lst; j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else{
				stn = edn = 0;
				while (stn < lst && edn < led){		//��b�@�P�ϤW
					if (stptr->GetClockPath(stn) != edptr->GetClockPath(edn))
						break;
					for (int x = 0; x <= 3; x++){	//0/00 NO DCC 1/01 slow aging DCC(20%) 2/10 fast aging DCC(80%)
						if (Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year + ERROR) || !Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year - ERROR)){	//Vio_Check�p�G�S��violation�|�^��true
							if (x % 2 == 1)	//01 11 -> 10 00
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)	//10 11
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < stptr->ClockLength(); j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							for (int j = 0; j < edptr->ClockLength(); j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}						
					}
					stn++;
					edn++;
				}
				int b_point = stn;
				for (; stn < lst; stn++){
					for (edn = b_point; edn < led; edn++){
						for (int x = 0; x <= 3; x++){
							for (int y = 0; y <= 3; y++){
								if (Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year + ERROR) || !Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year - ERROR)){
									if (x % 2 == 1)	//01 11 -> 1[0] 0[0]
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
									if (x >= 2)	//10 11
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
									if (y % 2 == 1)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
									if (y >= 2)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
									for (int j = 0; j < stptr->ClockLength(); j++){
										if (j == stn)	continue;
										file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									for (int j = 0; j < edptr->ClockLength(); j++){
										if (j == edn)	continue;
										file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									file << 0 << endl;
								}								
							}
						}
					}
				}
			}
		}
	}
	file.close();
}

int CallSatAndReadReport(int flag){
	//cout << "Start Call SAT" << endl;
	for (int i = 0; i < PathR.size(); i++){
		GATE* stptr = PathR[i].Gate(0);
		GATE* edptr = PathR[i].Gate(PathR[i].length() - 1);
		for (int j = 0; j < stptr->ClockLength(); j++)
			stptr->GetClockPath(j)->SetDcc(DCC_NONE);
		for (int j = 0; j < edptr->ClockLength(); j++)
			edptr->GetClockPath(j)->SetDcc(DCC_NONE);
	}
	if (flag == 1)
		system("./minisat best.cnf temp.sat");
	else
		system("./minisat sat.cnf temp.sat");

	fstream file;
	file.open("temp.sat", ios::in);
	string line;
	getline(file, line);
	if (line.find("UNSAT") != string::npos)
		return 0;
	int n1, n2;
	while (file >> n1 >> n2){

		if (n1 < 0 && n2 < 0)
			cbuffer_decode[(-n1 - 1) / 2]->SetDcc(DCC_NONE);
		else if (n1>0 && n2 < 0)
			cbuffer_decode[(n1 - 1) / 2]->SetDcc(DCC_S);
		else if (n1<0 && n2>0)
			cbuffer_decode[(-n1 - 1) / 2]->SetDcc(DCC_F);
		else
			cbuffer_decode[(n1 - 1) / 2]->SetDcc(DCC_M);
	}
	file.close();
	int cdcc = 0;
	for (int i = 0; i < cbuffer_decode.size(); i++)
	if (cbuffer_decode[i]->GetDcc() != DCC_NONE)
		cout << ++cdcc << " : " << cbuffer_decode[i]->GetName() << ' ' << cbuffer_decode[i]->GetDcc() << endl;
	return cdcc;
}

void CheckOriLifeTime(){							//���i��M�wTc��path���bcandidate��(mine��)
	cout << "Check Original Lifetime." << endl;		//����|��>10? ���᭱��Path slack�j�ܦh(�ۨ�lifetime��) �B�M�e��cp�����s�C(�e�ݦѤƤ���)
	double up = 10.0, low = 0.0;					//����|������7? 1.Path������slack������=>���ަѤƭ��ӳ����|�Ӯt 2.path���������׳���
	for (int i = 0; i < PathC.size(); i++){			//���Pcandidate�|�y�����B���P... ���p��
		if (!PathC[i]->CheckAttack())
			continue;
		double e_upper = 10000, e_lower = 10000;
		for (int j = 0; j < PathC.size(); j++){
			if (EdgeA[i][j]>9999)
				continue;
			double st = 1.0, ed = 50.0, mid;
			while (ed - st > 0.0001){
				mid = (st + ed) / 2;
				double upper, lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j, mid);				//y = ax+b => ����lower bound/upper bound�h�D�̻���t�h��				
				double Aging_P;
				if (upper > AgingRate(WORST, mid))
					Aging_P = AgingRate(WORST, mid);
				else if (upper < AgingRate(BEST, mid))
					Aging_P = AgingRate(BEST, mid);
				else
					Aging_P = upper;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_upper)
				e_upper = mid;				//�̦����I(�]���o�Ϳ��~�̦��b����)	
			st = 1.0, ed = 50.0;
			while (ed - st > 0.0001){
				mid = (st + ed) / 2;
				double upper, lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j, mid);
				double Aging_P;
				if (lower > AgingRate(WORST, mid))
					Aging_P = AgingRate(WORST, mid);
				else if (lower < AgingRate(BEST, mid))
					Aging_P = AgingRate(BEST, mid);
				else
					Aging_P = lower;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_lower)
				e_lower = mid;
		}
		if (up > e_upper)
			up = e_upper;
		if (low < e_lower)
			low = e_lower;

	}
	cout << up << ' ' << low << endl;
}


double CalQuality(double year, double &up, double &low){		//�p�G��mine�M�wTc ���? => �ثe���qcandidate����candidate+mine
	cout << "Start CalQuality" << endl;
	up = 10.0, low = 0.0;
	for (int i = 0; i < PathC.size(); i++){
		if (!PathC[i]->CheckAttack())
			continue;
		double e_upper = 10000, e_lower = 10000;
		for (int j = 0; j < PathC.size(); j++){
			//�p��ɱq�����i�����I(���O�Ⱥ�Q���I)
			if (EdgeA[i][j]>9999)
				continue;
			double st = 1.0, ed = 10.0, mid;
			while (ed - st > 0.0001){				//��binary search, �C�����qa�h��b���Ѥ�
				mid = (st + ed) / 2;
				double upper, lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j, mid);				//y = ax+b => ����lower bound/upper bound�h�D�̻���t�h��				
				double Aging_P;
				if (upper > AgingRate(WORST, mid))
					Aging_P = AgingRate(WORST, mid);
				else
					Aging_P = upper;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_upper)
				e_upper = mid;				//�̦����I(�]���o�Ϳ��~�̦��b����)	
			st = 1.0, ed = 10.0;
			while (ed - st > 0.0001){
				mid = (st + ed) / 2;
				double upper, lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j, mid);
				double Aging_P;
				if (lower > AgingRate(WORST, mid))
					Aging_P = AgingRate(WORST, mid);
				else
					Aging_P = lower;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_lower)
				e_lower = mid;
		}
		if (up > e_upper)
			up = e_upper;
		if (low < e_lower)
			low = e_lower;
	}
	return 0.0;
}

double Monte_CalQuality(double year, double &up, double &low){
	cout << "Start CalQuality" << endl;
	up = 10.0, low = 0.0;
	vector<double> monte;
	monte.clear();
	int TryT = 3000 / PathC.size();
	if (TryT < 1000)
		TryT = 1000;
	for (int i = 0; i < PathC.size(); i++){		//�C�@��path���X�o{TryT}�� => ����Ҧ���path�olifetime {TryT}�� 
		if (!PathC[i]->CheckAttack())
			continue;
		for (int tt = 0; tt < TryT; tt++){
			double lt = 10000;		//lifetime
			for (int j = 0; j < PathC.size(); j++){
				//�p��ɱq�����i�����I(���O�Ⱥ�Q���I)
				if (EdgeA[i][j]>9999)
					continue;
				double st = 1.0, ed = 10.0, mid;
				double U = rand() / (double)RAND_MAX;
				double V = rand() / (double)RAND_MAX;
				double Z = sqrt(-2 * log(U))*cos(2 * 3.14159265354*V);
				while (ed - st > 0.0001){
					mid = (st + ed) / 2;
					double Aging_mean = CalPreAging(AgingRate(WORST, mid), i, j, mid);
					double Aging_P = Z*(ser[i][j] * (1 + AgingRate(WORST, mid)) / (1 + AgingRate(WORST, 10))) + Aging_mean;
					if (Aging_P > AgingRate(WORST, mid))
						Aging_P = AgingRate(WORST, mid);
					if (Vio_Check(PathC[j], mid, Aging_P))
						st = mid;
					else
						ed = mid;
				}
				if (mid < lt)
					lt = mid;
			}
			//cout << i*tt << " : " << lt << endl;
			monte.push_back(lt);
		}
	}
	sort(monte.begin(), monte.end());
	int front = 0, back = monte.size() - 1;
	up = monte[front], low = monte[back];
	while (front + monte.size() - 1 - back <= monte.size() / 20){
		if (absff(monte[front] - (double)year)>absff(monte[back] - (double)year))
			up = monte[++front];
		else
			low = monte[--back];
	}
	return 0.0;
}

double Monte_CalQualityS(double year){
	double up = 10.0, low = 0.0;
	int i;
	do{
		i = rand() % PathC.size();
	} while (i >= PathC.size() || !PathC[i]->CheckAttack());
	double lt = 10000;		//lifetime
	for (int j = 0; j < PathC.size(); j++){
		//�p��ɱq�����i�����I(���O�Ⱥ�Q���I)
		if (EdgeA[i][j]>9999)
			continue;
		double st = 1.0, ed = 10.0, mid;
		double U = rand() / (double)RAND_MAX;
		double V = rand() / (double)RAND_MAX;
		double Z = sqrt(-2 * log(U))*cos(2 * 3.14159265354*V);
		while (ed - st > 0.0001){
			mid = (st + ed) / 2;
			double Aging_mean = CalPreAging(AgingRate(WORST, mid), i, j, mid);
			double Aging_P = Z*(ser[i][j] * (1 + AgingRate(WORST, mid)) / (1 + AgingRate(WORST, 10))) + Aging_mean;
			if (Aging_P > AgingRate(WORST, mid))
				Aging_P = AgingRate(WORST, mid);
			if (Vio_Check(PathC[j], mid, Aging_P))
				st = mid;
			else
				ed = mid;
		}
		if (mid < lt)
			lt = mid;
	}
	//cout << i*tt << " : " << lt << endl;
	return lt;	
}

bool CheckImpact(PATH* pptr){
	GATE* gptr;
	gptr = pptr->Gate(0);
	if (gptr->GetType() != "PI"){
		for (int i = 0; i < gptr->ClockLength(); i++)
		if (gptr->GetClockPath(i)->GetDcc() != DCC_NONE)
			return true;
	}
	gptr = pptr->Gate(pptr->length() - 1);
	if (gptr->GetType() != "PO"){
		for (int i = 0; i < gptr->ClockLength(); i++)
		if (gptr->GetClockPath(i)->GetDcc() != DCC_NONE)
			return true;
	}
	return false;
}

void RemoveRDCCs(){		//�յ۲���DCC
	map<GATE*, bool> must;
	for (int i = 0; i < PathC.size(); i++){
		if (!PathC[i]->Is_Chosen())
			continue;
		PATH* pptr = PathC[i];
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		for (int j = 0; j < stptr->ClockLength(); j++){
			if (stptr->GetClockPath(j)->GetDcc() != DCC_NONE)
				must[stptr->GetClockPath(j)] = true;
		}
		for (int j = 0; j < edptr->ClockLength(); j++){
			if (edptr->GetClockPath(j)->GetDcc() != DCC_NONE)
				must[edptr->GetClockPath(j)] = true;
		}
	}
	fstream file;
	file.open("sat.cnf", ios::out | ios::app);
	for (int i = 0; i < PathR.size(); i++){			//unsafe���I���i��Q��L����n��DCC(�u�n���|�L��)
		PATH* pptr = &PathR[i];
		if (pptr->Is_Chosen() || pptr->IsSafe())
			continue;
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		bool flag = true;
		for (int j = 0; j < stptr->ClockLength(); j++){
			if (stptr->GetClockPath(j)->GetDcc() != DCC_NONE && must.find(stptr->GetClockPath(j)) != must.end()){
				flag = false;
				break;
			}
		}
		for (int j = 0; j < edptr->ClockLength() && flag; j++){
			if (edptr->GetClockPath(j)->GetDcc() != DCC_NONE && must.find(edptr->GetClockPath(j)) != must.end()){
				flag = false;
				break;
			}
		}
		if (!flag)
			continue;
		for (int j = 0; j < stptr->ClockLength(); j++){
			file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << " 0" << endl;
			file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << " 0" << endl;
		}
		for (int j = 0; j < edptr->ClockLength(); j++){
			file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << " 0" << endl;
			file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << " 0" << endl;
		}
	}
	file.close();
}
int RefineResult(double year){
	int catk = 0, cimp = 0;
	for (int i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		if (CheckImpact(pptr))	//��DCC��bclock path�W
			cimp++;
		if (!Vio_Check(pptr, (double)year + ERROR, AgingRate(WORST, year + ERROR))){		//lifetime���������
			catk++;
		}
		if (!Vio_Check(pptr, (double)year - ERROR, AgingRate(WORST, year - ERROR))){
			if (pptr->CheckAttack())
				cout << "*";
			double st = 1.0, ed = 10, mid;
			while (ed - st>0.0001){
				mid = (st + ed) / 2;
				if (Vio_Check(&PathR[i], mid, AgingRate(WORST, mid)))
					st = mid;
				else
					ed = mid;
			}
			cout << i << " = " << mid << ' ';
		}
	}
	cout << endl << catk << " Paths Be Attacked." << endl;
	cout << cimp << " Paths Be Impacted." << endl;

	double maxe = 0;
	int maxep = -1;
	for (int i = 0; i < PathC.size(); i++){			//��"�q����i�I���X�h���d�����",�Ni�[�J
		if (PathC[i]->IsTried() || !PathC[i]->CheckAttack())
			continue;
		double e_upper = 10000, e_lower = 10000;
		for (int j = 0; j < PathC.size(); j++){
			if (EdgeA[i][j]>9999)
				continue;
			if (!PathC[i]->CheckAttack())
				continue;
			double st = 1.0, ed = 10.0, mid;
			while (ed - st > 0.0001){
				mid = (st + ed) / 2;
				double upper, lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j, mid);				//y = ax+b => ����lower bound/upper bound�h�D�̻���t�h��				
				double Aging_P;
				if (upper > AgingRate(WORST, mid))
					Aging_P = AgingRate(WORST, mid);
				else
					Aging_P = upper;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_upper)
				e_upper = mid;
			st = 1.0, ed = 10.0;
			while (ed - st > 0.0001){
				mid = (st + ed) / 2;
				double upper, lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j, mid);
				double Aging_P;
				if (lower > AgingRate(WORST, mid))
					Aging_P = AgingRate(WORST, mid);
				else
					Aging_P = lower;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_lower)
				e_lower = mid;
		}
		if (absl(e_upper - year) > maxe){
			maxe = absl(e_upper - year);
			maxep = i;
		}
		if (absl(e_lower - year) > maxe){
			maxe = absl(e_lower - year);
			maxep = i;
		}
	}
	if (maxep >= 0){
		PathC[maxep]->SetChoose(true);
		GenerateSAT("sat.cnf", year);
	}
	return maxep;
}

bool AnotherSol(){

	fstream file;
	fstream solution;
	file.open("sat.cnf", ios::out | ios::app);
	solution.open("temp.sat", ios::in);
	if (!file)
		cout << "fail to open sat.cnf" << endl;
	if (!solution)
		cout << "fail to open temp.sat" << endl;
	string line;
	getline(solution, line);
	if (line.find("UNSAT") != string::npos)
		return false;
	int dccno;
	while (solution >> dccno){
		file << -dccno << ' ';
	}
	file << endl;
	file.close();
	solution.close();
	return true;
}

void PrintStatus(double year){
	string command;
	const double dis = 2.58;
	getline(cin, command);
	getline(cin, command);
	while (command.find("quit") == string::npos){
		if (command.find("impact") != string::npos){
			for (int i = 0; i < PathR.size(); i++){
				if (CheckImpact(&PathR[i]))
					cout << PathR[i].No() << ' ';
			}
			cout << endl;
		}
		else if (command.find("victim") != string::npos){
			for (int i = 0; i < PathC.size(); i++){
				if (!Vio_Check(PathC[i], year + ERROR, AgingRate(WORST, year + ERROR)))
					cout << PathC[i]->No() << ' ';
			}
			cout << endl;
		}
		else if (command.find("cc") != string::npos){
			unsigned a, ac;
			a = atoi(command.c_str() + 3);
			cout << a << endl;
			for (int i = 0; i < PathC.size(); i++){
				if (PathC[i]->No() == a){
					ac = i;
					break;
				}
			}
			double upper, lower;
			for (int i = 0; i < PathC.size(); i++){
				if (EdgeA[i][ac] >9999)
					cout << "INF" << endl;
				else{
					cout << "Ag" << a << " = " << EdgeA[i][ac] << "Ag" << PathC[i]->No() << " + " << EdgeB[i][ac] << " +- " << ser[i][ac] * dis* (AgingRate(WORST, year) / AgingRate(WORST, 10)) << endl;
					CalPreInv(AgingRate(WORST, year), upper, lower, i, ac, year);
					cout << AgingRate(WORST, year) << " -> " << lower << " ~ " << upper << endl;
				}
			}
		}
		else if (command.find("unattack") != string::npos){
			for (int i = 0; i < PathC.size(); i++){
				if (Vio_Check(PathC[i], year + ERROR, AgingRate(WORST, year + ERROR)))
					cout << PathC[i]->No() << ' ';
			}
			cout << endl;
		}
		else if (command.find("push") != string::npos){
			unsigned a, ac;
			a = atoi(command.c_str() + 5);
			cout << a << endl;
			for (int i = 0; i < PathC.size(); i++){
				if (PathC[i]->No() == a){
					ac = i;
					break;
				}
			}
			cout << PathC[ac]->Gate(0)->GetName() << " -> " << PathC[ac]->Gate(PathC[ac]->length() - 1)->GetName() << endl;
			double upper, lower;
			for (int i = 0; i < PathC.size(); i++){
				if (Vio_Check(PathC[i], year + ERROR, AgingRate(WORST, year + ERROR)))	//�uprint�����|�������\��
					continue;
				if (EdgeA[ac][i] >9999)
					cout << "INF" << endl;
				else{
					cout << "Ag" << PathC[i]->No() << " = " << EdgeA[ac][i] << "Ag" << a << " + " << EdgeB[ac][i] << " +- " << ser[ac][i] * dis* (AgingRate(WORST, year) / AgingRate(WORST, 10)) << endl;
					CalPreInv(AgingRate(WORST, year), upper, lower, ac, i, year);
					cout << AgingRate(WORST, year) * 100 << "% -> " << lower * 100 << "% ~ " << upper * 100 << '%' << endl;
					double st = 1.0, ed = 10.0, mid;
					while (ed - st > 0.0001){
						mid = (st + ed) / 2;
						CalPreInv(AgingRate(WORST, mid), upper, lower, ac, i, mid);
						if (Vio_Check(PathC[i], mid, upper))
							st = mid;
						else
							ed = mid;
					}
					cout << mid << " ~ ";
					st = 1.0, ed = 10.0;
					while (ed - st > 0.0001){
						mid = (st + ed) / 2;
						CalPreInv(AgingRate(WORST, mid), upper, lower, ac, i, mid);
						if (Vio_Check(PathC[i], mid, lower))
							st = mid;
						else
							ed = mid;
					}
					cout << mid << endl;
				}
			}
		}
		else if (command.find("count edge") != string::npos){
			int count = 0;
			for (int i = 0; i < PathC.size(); i++)
			for (int j = 0; j < PathC.size(); j++)
			if (Check_Connect(i, j, year) && i != j)
				count++;
			cout << "Edge : " << count << endl;
		}
		else if (command.find("count group") != string::npos){
			cout << "May not have \"group\" in fact." << endl;
			bool *used = new bool[PathC.size()];
			vector<int>gsize;
			for (int i = 0; i < PathC.size(); i++)
				used[i] = false;
			for (int i = 0; i < PathC.size(); i++){
				if (used[i])
					continue;
				gsize.push_back(1);
				used[i] = true;
				for (int j = 0; j < PathC.size(); j++){
					if (Check_Connect(i, j, year) && i != j && !used[j]){
						used[j] = true;
						gsize[gsize.size() - 1]++;
					}
				}
			}
			cout << "GROUP : " << gsize.size() << endl;
			for (int i = 0; i < gsize.size(); i++){
				cout << gsize[i] << ' ';
			}
			cout << endl;
		}
		else if (command.find("one side edge") != string::npos){
			for (int i = 0; i < PathC.size(); i++){
				for (int j = i + 1; j < PathC.size(); j++){
					if (Check_Connect(i, j, year) ^ Check_Connect(j, i, year)){
						cout << AgingRate(WORST, year) - CalPreAging(AgingRate(WORST, year), i, j, year);
						cout << ' ' << AgingRate(WORST, year) - CalPreAging(AgingRate(WORST, year), j, i, year) << endl;
					}
				}
			}
		}
		else if (command.find("output candidate") != string::npos){
			fstream op;
			string fname = command.substr(17);
			op.open(fname.c_str(), ios::out);
			for (int i = 0; i < PathC.size(); i++){
				PATH* pptr = PathC[i];
				op << pptr->No() << endl;
				op << "StartPoint : " << pptr->Gate(0)->GetName() << endl;
				for (int j = 0; j < pptr->Gate(0)->ClockLength(); j++){
					op << pptr->Gate(0)->GetClockPath(j)->GetName() << "=>";
				}
				op << endl;
				op << "EndPoint : " << pptr->Gate(pptr->length() - 1)->GetName() << endl;
				for (int j = 0; j < pptr->Gate(pptr->length() - 1)->ClockLength(); j++){
					op << pptr->Gate(pptr->length() - 1)->GetClockPath(j)->GetName() << "=>";
				}
				op << endl << endl;
			}
			op.close();
			cout << "candidate data is saved in file : " << fname << endl;
		}
		else if (command.find("mine") != string::npos){
			for (int i = 0; i < PathR.size(); i++){
				if (!PathR[i].IsSafe() && !PathR[i].CheckAttack()){
					cout << PathR[i].No() << ' ';
					if (PathR[i].Gate(0)->GetType() == "PI")
						cout << "PI -> ";
					else
						cout << "FF -> ";
					if (PathR[i].Gate(PathR[i].length() - 1)->GetType() == "PO")
						cout << "PO" << endl;
					else
						cout << "FF" << endl;
				}
			}
		}
		getline(cin, command);
	}
}

void AdjustProcessVar(){
	for (int i = 0; i < PathR.size(); i++){
		for (int j = 1; j < PathR[i].length() - 1; j++){
			double U = rand() / (double)RAND_MAX;
			double V = rand() / (double)RAND_MAX;
			double Z = sqrt(-2 * log(U))*cos(2 * 3.14159265354*V);
			double v = Z*(0.05 / 1.96) + 1;
			PathR[i].SetProVar(j, v);
		}
	}
}