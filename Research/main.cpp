#include<iostream>
#include<fstream>
#include<string>
#include<stdlib.h>
#include<time.h>
#include "circuit.h"
#include "aging.h"

using namespace std;
vector<CIRCUIT> Circuit;
vector<PATH> PathR;
double **EdgeA;
double **EdgeB;
double **cor;
double **ser;
int **conf;
vector<PATH*> PathC;

int main(int argc, char* argv[]){
	//if (argc < 3)
	//	return 0;
	srand(time(NULL));
	string filename;
	filename = argv[1];
	filename = "s38584.vg";
	ReadCircuit(filename);
	cout << "Reading Circuit Finished." << endl;
	filename = argv[2];
	filename = "s38584.rpt";
	Circuit[0].PutClockSource();
	ReadPath_l(filename);
	cout << "Read Longest Path Finished."<<endl;	
	//ReadPath_s(filename);
	//cout << "Read Shortest Path Finished." << endl;	
	int year = atoi(argv[3]);
	//int year = 5;
	ReadAgingData();
	double MARGIN;
	int MAX_pn = 0;
	for (double mm = 1.0; mm < 1.2; mm += 0.01){
		CheckPathAttackbility(year, mm, false);	//璸衡程э翴period计
		if (PathC.size()>MAX_pn){
			MARGIN = mm;
			MAX_pn = PathC.size();
		}
		PathC.clear();
	}
	CheckPathAttackbility(year, MARGIN, true);
	cout << "Max margin = " << MARGIN << endl;

	if (PathC.size() <= 0){
		cout << "No Path Can Attack!" << endl;
		return 0;
	}

	int ss = PathC.size();
	EdgeA = new double*[ss];		// y = ax+b
	EdgeB = new double*[ss];		
	cor = new double*[ss];			//闽玒计
	conf = new int*[ss];			//がジ秆计
	ser = new double*[ss];	//error

	for (int i = 0; i < ss; i++){
		EdgeA[i] = new double[ss];
		EdgeB[i] = new double[ss];
		cor[i] = new double[ss];
		conf[i] = new int[ss];
		ser[i] = new double[ss];
	}
	filename = argv[5];
	filename = "s38584.cp";
	ReadCpInfo(filename);
	return 0;
	cout << "Initial Estimate Time" << endl;
	EstimateTimeEV(year);	
	cout << "Initial Estimate Soluation" << endl;
	for (int i = 0; i < PathC.size(); i++){
		CalSolMines(year, i);		
		cout << 100*(double)i / (double)PathC.size() << '%' << endl;
	}
	bool* bestnode = new bool[PathC.size()];
	double bestq = 0;
	string s;
	fstream fileres;
	for (int tryi = 0; tryi < atoi(argv[4]); tryi++){
		if (tryi>0)
			ChooseVertexWithGreedyMDS(year, -1.0);
		int co = 0;
		while (true){
			cout << tryi + 1 << " - " << co++ << endl;
			if (!ChooseVertexWithGreedyMDS(year, 1.0))
				break;
			GenerateSAT("sat.cnf", year);
			CallSatAndReadReport();
			fileres.open("temp.sat");
			getline(fileres, s);
			fileres.close();
			if (s.find("UNSAT") != string::npos)
				break;
			double Q = CalQuality(year);
			if (abs(Q - (double)year) < abs(bestq - (double)year)){
				for (int i = 0; i < PathC.size(); i++)
					bestnode[i] = PathC[i]->Is_Chosen();
				bestq = Q;
			}
			cout << "Q = " << Q << endl;
			//cout << "Try to Refine Result : " << endl;
			for (int i = 1; i <= 5; i++){
				if (!RefineResult(year)){
					//cout << "Result is in limit!" << endl;
					break;
				}
				//cout << "Time " << i << " : " << endl;
				if (!CallSatAndReadReport()){
					//cout << "Can't Refine Anymore" << endl;
					break;
				}
				cout << "Q = " << Q << endl;
				Q = CalQuality(year);
				if (abs(Q - (double)year) < abs(bestq - (double)year)){
					for (int i = 0; i < PathC.size(); i++)
						bestnode[i] = PathC[i]->Is_Chosen();
					bestq = Q;					
				}
			}
		}
	}
	
	cout << "Start Output Best Result." << endl;
	for (int i = 0; i < PathC.size(); i++)
		PathC[i]->SetChoose(bestnode[i]);
	GenerateSAT("sat.cnf", year);
	CallSatAndReadReport();
	do{
		double Q = CalQuality(year);
		cout << "Q = " << Q << endl;
		if (abs(Q - (double)year) < abs(bestq - (double)year))
			bestq = Q;
		if (!RefineResult(year))
			break;
	} while (CallSatAndReadReport());
	RefineResult(year);
	cout << "BEST Q = " << bestq << endl;
	return 0;
}