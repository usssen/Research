#include<iostream>
#include<fstream>
#include<string>
#include<time.h>
#include<stdlib.h>
#include "circuit.h"
#include "aging.h"

using namespace std;
vector<CIRCUIT> Circuit;
vector<PATH> PathR;
double **EdgeA;
double **EdgeB;
double **cor;
double **ser;
vector<PATH*> PathC;

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

int main(int argc, char* argv[]){
	if (argc < 5){
		cout << "./research [circuit] [path report] [regration info] [required life time] [restart times] [refine times]" << endl;
		return 0;
	}
	srand(time(NULL));
	string filename;
	filename = argv[1];
	//filename = "s38584.vg";
	cout << "Reading Circuit...";
	ReadCircuit(filename);
	cout << "Finished." << endl;
	filename = argv[2];
	//filename = "s38584.rpt";
	Circuit[0].PutClockSource();
	cout << "Reading Cirtical Paths Information...";
	ReadPath_l(filename);
	cout << "finished." << endl;
	//ReadPath_s(filename);
	//cout << "Read Shortest Path Finished." << endl;	
	int year = atoi(argv[4]);
	//int year = 5;
	ReadAgingData();
	CheckPathAttackbility(year, 1.001, true);
	
	if (PathC.size() <= 0){
		cout << "No Path Can Attack!" << endl;
		return 0;
	}
	CheckNoVio(year + 0.25);

	int ss = PathC.size();
	EdgeA = new double*[ss];		// y = ax+b
	EdgeB = new double*[ss];		
	cor = new double*[ss];			//¬ÛÃö«Y¼Æ	
	ser = new double*[ss];	//error

	for (int i = 0; i < ss; i++){
		EdgeA[i] = new double[ss];
		EdgeB[i] = new double[ss];
		cor[i] = new double[ss];		
		ser[i] = new double[ss];
	}
	filename = argv[3];
	//filename = "s38584.cp";
	cout << "Reading CPInfo...";
	ReadCpInfo(filename);
	cout << "finisned." << endl;
	cout << "Initial Estimate Time" << endl;
	EstimateTimeEV(year);	
	bool* bestnode = new bool[PathC.size()];
	double bestup = 100, bestlow = -100;
	string s;
	fstream fileres;
	for (int tryi = 0; tryi < atoi(argv[5]); tryi++){
		if (tryi>0)
			ChooseVertexWithGreedyMDS(year, -1.0);
		int co = 0;
		while (true){
			cout << tryi + 1 << " - " << co++ << endl;
			if (!ChooseVertexWithGreedyMDS(year, 1.0))
				break;
			GenerateSAT("sat.cnf", year);
			CallSatAndReadReport(0);
			fileres.open("temp.sat");
			getline(fileres, s);
			fileres.close();
			if (s.find("UNSAT") != string::npos)
				break;
			double upper, lower;
			CalQuality(year, upper, lower);
			if (maxf(absf(bestup - static_cast<double>(year)), absf(bestlow - static_cast<double>(year))) > maxf(absf(upper - static_cast<double>(year)), absf(lower - static_cast<double>(year)))){
				for (int i = 0; i < PathC.size(); i++)
					bestnode[i] = PathC[i]->Is_Chosen();
				bestlow = lower;
				bestup = upper;
			}
			cout << "Q = " << upper << " ~ " << lower << endl;
			cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
			for (int i = 1; i <= atoi(argv[6]); i++){
				if (!RefineResult(year))	
					break;
				cout << "Refine #" << i << " : " << endl;
				if (!CallSatAndReadReport(0)){
					//cout << "Can't Refine Anymore" << endl;
					break;
				}				
				CalQuality(year,upper,lower);							
				if (maxf(absf(bestup - static_cast<double>(year)), absf(bestlow - static_cast<double>(year))) > maxf(absf(upper - static_cast<double>(year)), absf(lower - static_cast<double>(year)))){
					for (int i = 0; i < PathC.size(); i++)
						bestnode[i] = PathC[i]->Is_Chosen();
					bestlow = lower;
					bestup = upper;
				}
				cout << "Q = " << upper << " ~ " << lower << endl;
				cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
			}
		}
	}
	bestup = 100, bestlow = -100;
	cout << "Final Refinement" << endl;
	for (int i = 0; i < PathC.size(); i++)
		PathC[i]->SetChoose(bestnode[i]);
	GenerateSAT("sat.cnf", year);
	CallSatAndReadReport(0);
	int t = 10;
	do{
		double upper, lower;
		CalQuality(year, upper, lower);		
		if (maxf(absf(bestup - static_cast<double>(year)), absf(bestlow - static_cast<double>(year))) > maxf(absf(upper - static_cast<double>(year)), absf(lower - static_cast<double>(year)))){
			bestlow = lower;
			bestup = upper;
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
	return 0;
}