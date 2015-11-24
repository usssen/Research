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
double **cor;	//correlation coefficient
int **conf;	//conflict soluation
vector<PATH*> PathC;

int main(int argc, char* argv[]){
	if (argc < 3)
		return 0;	
	string filename;
	filename = argv[1];
	//filename = "test.vg";
	ReadCircuit(filename);
	cout << "Reading Circuit Finished." << endl;
	filename = argv[2];
	//filename = "testp.rpt";
	Circuit[0].PutClockSource();
	ReadPath_l(filename);
	cout << "Read Longest Path Finished."<<endl;	
	//cout << "Shortest Path File Name : " << endl;
	//ReadPath_s(filename);
	//cout << "Read Shortest Path Finished." << endl;	
	int year = atoi(argv[3]);
	//int year = 5;
	ReadAgingData();
	double MARGIN;
	int MAX_pn = 0;
	for (double mm = 1.0; mm < 1.2; mm += 0.01){
		CheckPathAttackbility(year, mm, false);	//計算最多可改點的period乘數
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
	EdgeA = new double*[ss];
	EdgeB = new double*[ss];
	cor = new double*[ss];
	conf = new int*[ss];

	for (int i = 0; i < ss; i++){
		EdgeA[i] = new double[ss];
		EdgeB[i] = new double[ss];
		cor[i] = new double[ss];
		conf[i] = new int[ss];
	}
	
	srand(time(NULL));
	for (int i = 0; i < ss; i++){
		for (int j = i; j < ss; j++){
			if (j == i){	//y = x
				EdgeA[i][j] = 1;
				EdgeB[i][j] = 0;
				cor[i][j] = 0;
				continue;
			}
			double a = (double)rand() / RAND_MAX*0.3 + 0.85;	//差距大概在15%
			double b = 0.0;// (double)rand() / RAND_MAX - 0.5;	//-0.5~0.5
			double c = (double)rand() * 2 / RAND_MAX - 1;
			EdgeA[i][j] = a;		// y = ax+b
			EdgeB[i][j] = b;
			EdgeA[j][i] = 1 / a;	//	x = y/a-b/a
			EdgeB[j][i] = -1*(b / a);
			cor[i][j] = cor[j][i] = c;
		}
	}
	cout << "Initial Estimate Time" << endl;
	EstimateTimeEV(year);	
	cout << "Initial Estimate Soluation" << endl;
	for (int i = 0; i < PathC.size(); i++){
		CalSolMines(year, i);		
		cout << 100*(double)i / (double)PathC.size() << '%' << endl;
	}
	bool* bestnode = new bool[PathC.size()];
	double bestq = 0.0;
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
			//cout << "Q = " << CalQuality(year) << endl;
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
				//cout << "Q = " << CalQuality(year) << endl;
				Q = CalQuality(year);
				if (abs(Q - (double)year) < abs(bestq - (double)year)){
					for (int i = 0; i < PathC.size(); i++)
						bestnode[i] = PathC[i]->Is_Chosen();
					bestq = Q;
				}
			}
		}
	}
	for (int i = 0; i < PathC.size(); i++)
		PathC[i]->SetChoose(bestnode[i]);
	GenerateSAT("sat.cnf", year);
	CallSatAndReadReport();
	cout << "Q = " << CalQuality(year) << endl;
	return 0;
}