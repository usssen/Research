#include<iostream>
#include<fstream>
#include<string>
#include<time.h>
#include<stdlib.h>
#include "circuit.h"
#include "aging.h"
#include<signal.h>

void inthandler(int s){
	CallSatAndReadReport(1);
	exit(1);
}


using namespace std;
vector<CIRCUIT> Circuit;
vector<PATH> PathR;
double **EdgeA;
double **EdgeB;
double **cor;
double **ser;
double info[5];
vector<PATH*> PathC;
double ERROR = 1.0;

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

bool BInv(double &bu, double &bl, double u1, double l1, double u2, double l2,double n){
	if (absf(maxf(absf(u1 - n), absf(l1 - n)) - maxf(absf(u2 - n), absf(l2 - n))) < 0.001){
		if (minf(absf(u1 - n), absf(l1 - n)) < minf(absf(u2 - n), absf(l2 - n))){
			bu = u1, bl = l1;
			return false;
		}
		bu = u2, bl = l2;
		return true;
	}
	if (maxf(absf(u1 - n), absf(l1 - n)) < maxf(absf(u2 - n), absf(l2 - n))){
		bu = u1, bl = l1;
		return false;
	}
	bu = u2, bl = l2;
	return true;
}


int main(int argc, char* argv[]){
	if (argc < 7){
		cout << "./research [circuit] [path report] [regration info] [required life time] [restart times] [refine times] [ERROR limit]" << endl;
		return 0;
	}
	signal(SIGINT, inthandler);
	srand(time(NULL));
	string filename;
	filename = argv[1];
	//filename = "s38417.vg";
	cout << "Reading Circuit...";
	ReadCircuit(filename);
	cout << "Finished." << endl;
	filename = argv[2];
	//filename = "s38417.rpt";
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
	//int year = 5;
	ReadAgingData();
	AdjustConnect();
	double PLUS = ERROR;
	fstream file;
	string line;
	file.open("Parameter.txt"); 
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
			break;
		}
	}
	cout << ERROR << ' ' << PLUS << endl;
	CheckPathAttackbility(year, 1.001, true, PLUS);

	if (PathC.size() <= 0){
		cout << "No Path Can Attack!" << endl;
		return 0;
	}
	CheckNoVio(year + ERROR);

	int ss = PathC.size();
	EdgeA = new double*[ss];		// y = ax+b
	EdgeB = new double*[ss];		
	cor = new double*[ss];			//相關係數	
	ser = new double*[ss];	//error

	for (int i = 0; i < ss; i++){
		EdgeA[i] = new double[ss];
		EdgeB[i] = new double[ss];
		cor[i] = new double[ss];		
		ser[i] = new double[ss];
	}
	filename = argv[3];
	//filename = "s38417.cp";
	cout << "Reading CPInfo...";
	ReadCpInfo(filename);
	cout << "finisned." << endl;
	/*
	cout << "Initial Estimate Time" << endl;
	EstimateTimeEV(year);
	*/
	if (argc > 8){
		cout << "Please Input Command : ";
		PrintStatus(year);
	}
	bool* bestnode = new bool[PathC.size()];
	double bestup = 100, bestlow = -100;
	string s;
	fstream fileres;
	int fr = 999999;
	int trylimit = atoi(argv[5]),tryi = 0;	
	do{
		for (; tryi < trylimit; tryi++){
			cout << "Round : " << tryi << endl;
			if (!ChooseVertexWithGreedyMDS(year, false)){
				ChooseVertexWithGreedyMDS(year, true);
				cout << "Not Domination Set!" << endl;
				continue;
			}
			GenerateSAT("sat.cnf", year);
			CallSatAndReadReport(0);

			fileres.open("temp.sat");
			getline(fileres, s);
			fileres.close();
			if (s.find("UNSAT") != string::npos){
				if (bestup<10 && bestlow>1)
					cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
				ChooseVertexWithGreedyMDS(year, true);
				continue;
			}
			if (fr > tryi)
				fr = tryi;
			
			cout << endl << "Try to Remove Redundant DCCs." << endl;
			int mindccs = CallSatAndReadReport(0);
			system("cp sat.cnf bestdccs.cnf");

			for (int i = 0; i < PathC.size(); i++){
				if (PathC[i]->Is_Chosen())
					continue;
				system("cp sat.cnf backup.cnf");
				RemoveRDCCs(i);
				int dccs = CallSatAndReadReport(0);
				if (!dccs){
					system("cp backup.cnf sat.cnf");
					continue;
				}
				if (dccs < mindccs){
					mindccs = dccs;
					system("cp sat.cnf bestdccs.cnf");
				}			
			}
			//如何取捨Quality和DCC ?
			system("cp bestdccs.cnf sat.cnf");
			
			CallSatAndReadReport(0);		
			double upper, lower;
			CalQuality(year, upper, lower);
			if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year)){
				for (int i = 0; i < PathC.size(); i++)
					bestnode[i] = PathC[i]->Is_Chosen();
				system("cp sat.cnf best.cnf");
			}
			cout << "Q = " << upper << " ~ " << lower << endl;
			cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
	

			for (int i = 1; i <= atoi(argv[6]); i++){
				if (!RefineResult(year))
					break;
				cout << "Refine #" << i << " : " << endl;
				if (!CallSatAndReadReport(0)){	//0: 一般找解 1:最佳解
					//cout << "Can't Refine Anymore" << endl;
					break;
				}
				CalQuality(year, upper, lower);
				if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year)){
					for (int i = 0; i < PathC.size(); i++)
						bestnode[i] = PathC[i]->Is_Chosen();
					system("cp sat.cnf best.cnf");
				}
				cout << "Q = " << upper << " ~ " << lower << endl;
				cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
			}
		}
		if (bestup>10){
			cout << "NO SOLUTION!, Input New Try Limit or 0 for Give Up. " << endl;
			int addt;
			cin >> addt;
			if (addt <= 0)
				return 0;
			trylimit += addt;
		}
	} while (bestup > 10);

	bestup = 100, bestlow = -100;
	cout << "Final Refinement" << endl;
	for (int i = 0; i < PathC.size(); i++)
		PathC[i]->SetChoose(bestnode[i]);
	system("cp best.cnf sat.cnf");
	CallSatAndReadReport(0);
	int t = 10;	//最終refine 10次
	do{
		double upper, lower;
		CalQuality(year, upper, lower);		
		if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year)){			
			system("cp sat.cnf best.cnf");
		}
		cout << "Q = " << upper << " ~ " << lower << endl;
		cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
		if (!RefineResult(year))
			break;
	} while (t--&&CallSatAndReadReport(0));
	cout << endl << endl << "Final Result : " << endl;
	CallSatAndReadReport(1);
	RefineResult(year);
	cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
	cout << "Clock Period = " << info[0] << endl;
	for (int i = 1; i <= 4; i++)
		cout << info[i] << ' ';
	cout << endl;
	cout << "Enter the year : " << endl;
	double y;
	cin >> y;
	PrintStatus(y);
	cout << fr << endl;
	return 0;
}