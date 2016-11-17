#ifndef CIRCUIT_H
#define CIRCUIT_H
#include<string>
#include<vector>
#include<map>
#include"typedef.h"
#include"aging.h"

using namespace std;

void ReadCircuit(string filename);
void ReadPath_l(string filename);
//void ReadPath_s(string filename);
void CalVertexWeight();
bool ChooseVertexWithGreedyMDS(double year,bool puthash);
int HashAllClockBuffer();
void GenerateSAT(string filename,double year);
int CallSatAndReadReport(int flag);
void CheckPathAttackbility(double year,double margin,bool flag,double PLUS);
double CalQuality(double year,double &upper,double &lower);
double Monte_CalQuality(double year, double &upper, double &lower);
double Monte_CalQualityS(double year);
void RemoveRDCCs();
int RefineResult(double year);
void EstimateTimeEV(double year);
void ReadCpInfo(string filename);
bool CheckNoVio(double year);
void PrintStatus(double year);
void AdjustConnect();
bool AnotherSol();
void CheckOriLifeTime();
void AdjustProcessVar();

class GATE;

class WIRE{
private:
	WIRETYPE type;
	string name;
	GATE* input;
	vector<GATE*> output;
public:
	WIRE(string n,WIRETYPE t) :name(n), type(t),input(NULL){ output.clear(); }
	~WIRE(){}
	WIRETYPE GetType(){ return type; }
	string GetName(){ return name; }
	GATE* GetInput(){ return input; }
	GATE* GetOutput(int i){ return output[i]; }
	int No_Output(){ return output.size(); }
	void SetInput(GATE* g){ input = g; }
	void SetOutput(GATE* g){ output.push_back(g); }
};

class GATE{
private:
	string name;
	string type;
	vector<WIRE*>input;
	//vector<WIRE*>output;
	WIRE* output;
	bool clock_flag;			//flip-flop��clock-path�O�_�w�g�إߧ���	
	AGINGTYPE add_dcc;
	vector<GATE*>clock_path;	//flip-flop�M��,�O���즹filp-flop��clock path
	double in_time, out_time;	//clock-buffer��,�O��clock��F���ɶ�(�o���|�]�����PFF�����|�Ӥ��P)
public:
	GATE(string n,string t) :name(n),type(t),in_time(-1),out_time(-1),clock_flag(false),add_dcc(DCC_NONE){
		input.clear();
		//output.clear();
		output = NULL;
		clock_path.clear();
	}
	~GATE(){}
	string GetName(){ return name; }
	string GetType(){ return type; }
	int No_Input(){ return input.size(); }
	void SetInput(WIRE* w){ input.push_back(w);	}
	void SetOutput(WIRE* w){ 
		//output.push_back(w);
		output = w;
	}
	WIRE* GetInput(int i){ return input[i]; }
	//WIRE* GetOutput(int i){ return output[i]; }
	WIRE* GetOutput(){ return output; }
	void SetClockPath(GATE* g){
		if (clock_flag)
			return;
		clock_path.push_back(g); 
	}
	GATE* GetClockPath(int i){ return clock_path[i]; }	
	int ClockLength(){ return clock_path.size(); }
	void SetInTime(double t){ in_time = t; }		//intime,outtime�O��gate��clock buffer�ɥΨӰO����J/��X�ɶ�
	void SetOutTime(double t){ out_time = t; }
	double GetInTime(){ return in_time; }
	double GetOutTime(){ return out_time; }
	void Setflag(){ clock_flag = true; }			//�O�_��clock buffer?
	void SetDcc(AGINGTYPE dcc){ add_dcc = dcc; }	//�o��clock gate��m�W��DCC���κA
	AGINGTYPE GetDcc(){ return add_dcc; }
};

class CIRCUIT{
private:
	string name;
	vector<GATE*>gate_list;
	vector<WIRE*>wire_list;
	map<string, GATE*>nametogate;
	map<string, WIRE*>nametowire;
	
public:
	CIRCUIT(string n):name(n){
		gate_list.clear();
		wire_list.clear();
	}	
	string GetName(){ return name; }
	void PutWire(WIRE* w){ 
		wire_list.push_back(w);
		nametowire[w->GetName()] = w;
	}
	void PutGate(GATE* g){ 
		gate_list.push_back(g);
		nametogate[g->GetName()] = g;
	}
	WIRE* GetWire(int i){ return wire_list[i]; }
	WIRE* GetWire(string name){
		if (nametowire.find(name) == nametowire.end()){
			WIRE* t = new WIRE(name, PI);
			wire_list.push_back(t);
			nametowire[name] = t;
			cout << name << endl;
		}
		return nametowire[name]; 
	}
	GATE* GetGate(string name){
		if (nametogate.find(name) == nametogate.end())	return NULL;
		return nametogate[name];
	}
	unsigned GateSize(){ return gate_list.size(); }
	GATE* GetGate(int i){ return gate_list[i]; }
	void PutClockSource();
};

class PATH{		//�����Otrans time
private:
	class TIMING{
	private:
		double in, out;
	public:
		TIMING(double i, double o) :in(i), out(o){}
		~TIMING(){}
		double in_time(){ return in; }
		double out_time(){ return out; }
	};
	//���ΰOgate�������s�u�W(�����e�@��gate����X�u),�e�@��gate����X�M��@��gate����J�t���s�u����
	vector<GATE*> gate_list;		//�Ĥ@�ӫ�̫�@�ӥ���flip-flop/PI/PO
	vector<TIMING>timing;
	double setuptime;
	double holdtime;
	double clock_to_end;
	double estime;
	double psd;	//���зǮt	
	bool attackable;
	bool safe;
	bool choose;
	bool tried;					//�Ω�brefine�ɥιL�P�_
	unsigned no;	
	PATHTYPE type;
	double processvar[100];		//�s�{�ܲ��Ѽ�
public:
	PATH():attackable(false),choose(false){
		setuptime = holdtime = -1;
		gate_list.clear();
		timing.clear();
		for (int i = 0; i < 100; i++)
			processvar[i] = 1.0;
	}
	~PATH(){}
	void AddGate(GATE* g, double i,double o){
		gate_list.push_back(g);
		TIMING temp(i, o);
		timing.push_back(temp);
	}

	GATE* Gate(int i){ return gate_list[i]; }
	double In_time(int i){ return timing[i].in_time(); }
	double Out_time(int i){ return timing[i].out_time(); }	
	
	/*
	int Find_Gate(string name){		//��"���X"
		for (unsigned i = 0; i < gate_list.size();i++){
			if (gate_list[i]->GetName() == name)
				return i;
		}
		return -1;
	}
	*/
	void SetNo(unsigned n){ no = n; }	//�O���btiming report�W��path���X
	unsigned No(){ return no; }
	void SetST(double t){ setuptime = t; }	//setup time
	void SetHT(double t){ holdtime = t; }
	void SetCTE(double t){ clock_to_end = t; }	//clock �쥽��flip-flop���ɶ�
	double GetST(){ return setuptime; }	
	double GetHT(){ return holdtime; }
	double GetCTE(){ return clock_to_end; }
	double GetCTH(){ return timing[0].in_time(); }	//clock�쭺��flip-flop���ɶ�
	double GetAT(){ return timing[timing.size()-1].in_time(); }		//arrival time
	double GetPathDelay() {
		double d = this->In_time(1) - this->Out_time(0);
		for (int i = 1; i < this->length() - 1; i++)
			d += (this->In_time(i + 1) - this->In_time(i))*processvar[i];
		return d;
		//return processvar[1]*(this->In_time(this->length() - 1) - this->Out_time(0));
	}
	void SetType(PATHTYPE t){ type = t; }	//short or long path
	PATHTYPE GetType(){ return type; }
	int length(){ return gate_list.size(); }	//path������ ���tclock
	void SetAttack(bool a){ attackable = a; }	
	bool CheckAttack(){ return attackable; }	//�O�_��candidate
	bool Is_Chosen(){ return choose; }			
	void SetChoose(bool c){ choose = c; }		//�O�_�Q�אּshortlist
	void SetEstimateTime(double t){ estime = t; }
	double GetEstimateTime(){ return estime; }	
	void SetPSD(double t) { psd = t; }
	double GetPSD(){ return psd; }				//�o�|���w�R��
	void SetSafe(bool s){ safe = s; }
	bool IsSafe(){ return safe; }				//�O�_����lifetime��>n+error
	void SetTried(bool t){ tried = t; }
	bool IsTried() { return tried; }			//�O���O�_�w�Q�յۥ[�Jshortlist(refinement�[�I�ɦb�Ϊ�)
	void SetProVar(int i, double a){ processvar[i] = a; }
	double GetProVar(int i){ return processvar[i]; }
};


#endif