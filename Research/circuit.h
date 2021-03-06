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
void RemoveRDCCs();
int RefineResult(double year);
void EstimateTimeEV(double year);
void ReadCpInfo(string filename);
bool CheckNoVio(double year);
void PrintStatus(double year);
void AdjustConnect();
bool AnotherSol();
void CheckOriLifeTime();

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
	bool clock_flag;			//flip-flop的clock-path是否已經建立完成	
	AGINGTYPE add_dcc;
	vector<GATE*>clock_path;	//flip-flop專用,記錄到此filp-flop的clock path
	double in_time, out_time;	//clock-buffer用,記錄clock到達的時間(這不會因為不同FF間路徑而不同)
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
	void SetInTime(double t){ in_time = t; }		//intime,outtime是此gate為clock buffer時用來記錄輸入/輸出時間
	void SetOutTime(double t){ out_time = t; }
	double GetInTime(){ return in_time; }
	double GetOutTime(){ return out_time; }
	void Setflag(){ clock_flag = true; }			//是否為clock buffer?
	void SetDcc(AGINGTYPE dcc){ add_dcc = dcc; }	//這個clock gate位置上的DCC的形態
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
	GATE* GetGate(int i){ return gate_list[i]; }
	void PutClockSource();
};

class PATH{		//先不記trans time
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
	//不用記gate之間的連線名(必為前一個gate的輸出線),前一個gate之輸出和後一個gate之輸入差為連線延遲
	vector<GATE*> gate_list;		//第一個後最後一個必為flip-flop/PI/PO
	vector<TIMING>timing;
	double setuptime;
	double holdtime;
	double clock_to_end;
	double estime;
	double psd;	//類標準差	
	bool attackable;
	bool safe;
	bool choose;
	bool tried;					//用於在refine時用過與否
	unsigned no;	
	PATHTYPE type;
public:
	PATH():attackable(false),choose(false){
		setuptime = holdtime = -1;
		gate_list.clear();
		timing.clear();
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
	int Find_Gate(string name){		//找"號碼"
		for (unsigned i = 0; i < gate_list.size();i++){
			if (gate_list[i]->GetName() == name)
				return i;
		}
		return -1;
	}
	*/
	void SetNo(unsigned n){ no = n; }	//記錄在timing report上的path號碼
	unsigned No(){ return no; }
	void SetST(double t){ setuptime = t; }	//setup time
	void SetHT(double t){ holdtime = t; }
	void SetCTE(double t){ clock_to_end = t; }	//clock 到末端flip-flop的時間
	double GetST(){ return setuptime; }	
	double GetHT(){ return holdtime; }
	double GetCTE(){ return clock_to_end; }
	double GetCTH(){ return timing[0].in_time(); }	//clock到首端flip-flop的時間
	double GetAT(){ return timing[timing.size()-1].in_time(); }		//arrival time
	void SetType(PATHTYPE t){ type = t; }	//short or long path
	PATHTYPE GetType(){ return type; }
	int length(){ return gate_list.size(); }	//path的長度 不含clock
	void SetAttack(bool a){ attackable = a; }	
	bool CheckAttack(){ return attackable; }	//是否為candidate
	bool Is_Chosen(){ return choose; }			
	void SetChoose(bool c){ choose = c; }		//是否被選為shortlist
	void SetEstimateTime(double t){ estime = t; }
	double GetEstimateTime(){ return estime; }	
	void SetPSD(double t) { psd = t; }
	double GetPSD(){ return psd; }				//這四條已刪除
	void SetSafe(bool s){ safe = s; }
	bool IsSafe(){ return safe; }				//是否怎麼放lifetime都>n+error
	void SetTried(bool t){ tried = t; }
	bool IsTried() { return tried; }			//記錄是否已被試著加入shortlist(refinement加點時在用的)
};


#endif