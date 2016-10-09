#include<iostream>
#include<fstream>
#include<string>
#include<time.h>
#include<stdlib.h>
#include "circuit.h"
#include "aging.h"
#include<signal.h>

/*
void inthandler(int s){
	CallSatAndReadReport(1);
	exit(1);
}
*/

using namespace std;
vector<CIRCUIT> Circuit;
vector<PATH> PathR;		//所有的path
double **EdgeA;
double **EdgeB;
double **cor;
double **ser;
double info[5];
vector<PATH*> PathC;	//candidate 的path
double ERROR = 1.0;		//可容忍的lifetime誤差(改個名)

inline double absf(double x){
	if (x < 0)
		return -x;
	return x;
}

inline double maxf(double a, double b){
	if (a > b)
		return a;
	return b;
}

inline double minf(double a, double b){
	if (a < b)
		return a;
	return b;
}

bool BInv(double &bu, double &bl, double u1, double l1, double u2, double l2,double n,int &dcb,int dc1,int dc2){
	if (absf(maxf(absf(u1 - n), absf(l1 - n)) - maxf(absf(u2 - n), absf(l2 - n))) < 0.01){
		if (absf(minf(absf(u1 - n), absf(l1 - n)) - minf(absf(u2 - n), absf(l2 - n))) < 0.01){
			if (dc1<dc2){
				bu = u1, bl = l1, dcb = dc1;
				return false;
			}
			bu = u2, bl = l2, dcb = dc2;
			return true;
		}
		else if (minf(absf(u1 - n), absf(l1 - n)) < minf(absf(u2 - n), absf(l2 - n))){
			bu = u1, bl = l1, dcb = dc1;
			return false;
		}
		bu = u2, bl = l2, dcb = dc2;
		return true;
	}
	else if (maxf(absf(u1 - n), absf(l1 - n)) < maxf(absf(u2 - n), absf(l2 - n))){
		bu = u1, bl = l1, dcb = dc1; 
		return false;
	}
	bu = u2, bl = l2, dcb = dc2;
	return true;
}


int main(int argc, char* argv[]){
	if (argc < 7){
		cout << "./research [circuit] [path report] [regration info] [required life time] [restart times] [refine times] [ERROR limit]" << endl;
		return 0;
	}
	//signal(SIGINT, inthandler);
	clock_t tst,tsolst;
	double t_sol = 0, t_nosol = 0;
	int c_sol = 0, c_nosol = 0;
	tst = clock();
	srand(time(NULL));
	string filename;
	filename = argv[1];	
	cout << "Reading Circuit...";
	ReadCircuit(filename);
	cout << "Finished." << endl;
	filename = argv[2];	
	Circuit[0].PutClockSource();
	cout << "Reading Cirtical Paths Information...";
	ReadPath_l(filename);
	cout << "finished." << endl;	
	//ReadPath_s(filename);
	//cout << "Read Shortest Path Finished." << endl;	
	int year = atoi(argv[4]);
	ERROR = year*0.1;
	if (argc > 7){
		ERROR = atof(argv[7]);
	}		
	ReadAgingData();
	AdjustConnect();
	double PLUS = ERROR;
	fstream file;
	string line;
	file.open("Parameter.txt");
	double tight = 1.000001;
	int FINAL = 0;
	bool monte_s = false;
	while (getline(file, line)){
		if (line.find("PLUS") != string::npos){
			if (line.find("auto") != string::npos){
				PLUS = ERROR;
			}
			else if (line.find("fixed")!=string::npos){
				double f = atof(line.c_str() + 10);
				PLUS = f - year;
			}
			else{
				PLUS = atof(line.c_str() + 4);
			}
		}
		if (line.find("TIGHT") != string::npos){
			tight = atof(line.c_str() + 5);
		}
		if (line.find("FINAL") != string::npos){
			FINAL = atof(line.c_str() + 5);
		}
		if (line.find("MONTE YES") != string::npos){
			monte_s = true;
		}
	}
	cout << "TIGHT = " << tight << endl << "Final Refinament = " << FINAL << endl;
	cout << "Monte-Carlo is ";
	if (monte_s)
		cout << "OPEN" << endl;
	else
		cout << "CLOSE" << endl;
	CheckPathAttackbility(year, tight, true, PLUS);
	
	if (PathC.size() <= 0){
		cout << "No Path Can Attack!" << endl;
		return 0;
	}
	if (!CheckNoVio(year + PLUS)){
		cout << "Too Tight Clock Period!" << endl;
		return 0;
	}

	int ss = PathC.size();
	EdgeA = new double*[ss];		// y = ax+b
	EdgeB = new double*[ss];		
	cor = new double*[ss];			//相關係數	
	ser = new double*[ss];			//標準誤

	for (int i = 0; i < ss; i++){
		EdgeA[i] = new double[ss];
		EdgeB[i] = new double[ss];
		cor[i] = new double[ss];		
		ser[i] = new double[ss];
	}
	filename = argv[3];	
	cout << "Reading CPInfo...";
	ReadCpInfo(filename);
	cout << "finisned." << endl;		
	CheckOriLifeTime();		
	
	//PrintStatus(year);
	
	bool* bestnode = new bool[PathC.size()];
	double bestup = 100, bestlow = -100;
	double monteU, monteL;
	int bestdcc = 10000;
	string s;
	fstream fileres;
	//int fr = 999999;
	int trylimit = atoi(argv[5]),tryi = 0;
	do{
		for (; tryi < trylimit; tryi++){
			tsolst = clock();
			cout << "Round : " << tryi << endl;
			if (!ChooseVertexWithGreedyMDS(year, false)){	
				cout << "Not Domination Set!" << endl;
				ChooseVertexWithGreedyMDS(year, true);	//找shortlist的function, 回傳值false時表沒有domination set
				continue;
			}
			GenerateSAT("sat.cnf", year);				//產生CNF表達式
			int oridccs = CallSatAndReadReport(0);		//用來呼叫SAT tool並將讀取結果, input 0: 一般找解 1:最佳解, 回傳值為DCC的數量
			ChooseVertexWithGreedyMDS(year, true);
			if (!oridccs){
				if (bestup<10 && bestlow>1)
					cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
				c_nosol++;
				t_nosol += (double)(clock() - tsolst) / CLOCKS_PER_SEC;
				continue;
			}
			/*
			if (fr > tryi)
				fr = tryi;	
			*/
			for (int i = 0; i < PathC.size(); i++)			//試著加path進shortlist來改善結果
				PathC[i]->SetTried(PathC[i]->Is_Chosen());
			for (int i = 0; i < atoi(argv[6]); i++){		//太多次可能造成加入過多點但解沒差多少 而造成多餘的DCC
				int AddNodeIndex = RefineResult(year);
				if (AddNodeIndex < 0)
					break;
				PathC[AddNodeIndex]->SetTried(true);			//記錄此點已經試著加過了
				int dccs = CallSatAndReadReport(0);			
				if (!dccs){
					PathC[AddNodeIndex]->SetChoose(false);
					continue;
				}						
				oridccs = dccs;
			}
			GenerateSAT("sat.cnf", year);
			
			system("cp sat.cnf backup.cnf");
			RemoveRDCCs();									//用來試著從Mine上移除DCC
			int dccss = CallSatAndReadReport(0);
			if (dccss == 0 || oridccs < dccss){
				system("cp backup.cnf sat.cnf");
			}
			
			int dccs = CallSatAndReadReport(0);
			double upper, lower;
			CalQuality(year, upper, lower);					//計算quality
			if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year,bestdcc,bestdcc,dccs)){	//看是否解的區間比原本的好
				if (monte_s)
					Monte_CalQuality(year, monteU, monteL);	//Monte-Carlo估計
				for (int i = 0; i < PathC.size(); i++)
					bestnode[i] = PathC[i]->Is_Chosen();
				system("cp sat.cnf best.cnf");
			}
			cout << "Q = " << upper << " ~ " << lower << endl;
			cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
			for (int i = 0; i<atoi(argv[6]); i++){				
				if (!AnotherSol())
					break;
				dccs = CallSatAndReadReport(0);
				if (!dccs)
					break;
				//double upper, lower;
				CalQuality(year, upper, lower);
				if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year, bestdcc, bestdcc, dccs)){
					if (monte_s)
						Monte_CalQuality(year, monteU, monteL);
					system("cp sat.cnf best.cnf");
				}
				cout << "Q = " << upper << " ~ " << lower << endl;
				cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
			}
			c_sol++;
			t_sol += (double)(clock() - tsolst) / CLOCKS_PER_SEC;
		}
		if (bestup>10){
			cout << "NO SOLUTION!, Input new try limit or 0 to exit. " << endl;
			int addt;
			cin >> addt;
			if (addt <= 0)
				return 0;
			trylimit += addt;
		}
	} while (bestup > 10);

	bestup = 100, bestlow = -100, bestdcc = 10000;
	cout << "Final Refinement" << endl;
	for (int i = 0; i < PathC.size(); i++)
		PathC[i]->SetChoose(bestnode[i]);
	system("cp best.cnf sat.cnf");
	int dccs = CallSatAndReadReport(0);
	do{												//結束前多試幾次
		double upper, lower;
		CalQuality(year, upper, lower);		
		if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year,bestdcc,bestdcc,dccs)){			
			if (monte_s)
				Monte_CalQuality(year, monteU, monteL);
			system("cp sat.cnf best.cnf");
		}
		cout << "Q = " << upper << " ~ " << lower << endl;
		cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
		if (!AnotherSol())
			break;
		dccs = CallSatAndReadReport(0);
		if (!dccs)
			break;		
	} while (FINAL--);
	cout << endl << endl << "Final Result : " << endl;
	CallSatAndReadReport(1);
	RefineResult(year);
	cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
	if (monte_s){
		cout << "BEST Q(Monte) = " << monteU << " ~ " << monteL << endl;
	}
	cout << "Clock Period = " << info[0] << endl;
	for (int i = 1; i <= 4; i++)
		cout << info[i] << ' ';
	cout << endl;
	//cout << fr << endl;
	cout << "Total Runtime : " << (clock() - tst) / CLOCKS_PER_SEC << endl;
	cout << "Solution count = " << c_sol << " Time = " << t_sol << endl;
	cout << "No solution count = " << c_nosol << " Time = " << t_nosol << endl;
	cout << "Enter the year : " << endl;
	double y;
	cin >> y;	
	//PrintStatus(y);	
	return 0;
}