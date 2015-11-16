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
	ReadCircuit(filename);
	cout << "Reading Circuit Finished." << endl;
	cout << "Longest Path File Name : " << endl;
	filename = argv[2];	
	Circuit[0].PutClockSource();
	ReadPath_l(filename);
	cout << "Read Longest Path Finished."<<endl;	
	//cout << "Shortest Path File Name : " << endl;
	//ReadPath_s(filename);
	//cout << "Read Shortest Path Finished." << endl;	
	int year = atoi(argv[3]);	
	ReadAgingData();
	CheckPathAttackbility(year);
	
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
			double a = (double)rand() / RAND_MAX*0.3 + 0.85;	//®t¶Z¤j·§¦b15%
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
	//for (int i = 0; i < PathC.size(); i++)
	//	cout << PathC[i]->GetEstimateTime() << endl;	
	cout << "Initial Estimate Soluation" << endl;
	for (int i = 0; i < PathC.size(); i++){
		CalSolMines(year, i);		
		cout << 100*(double)i / (double)PathC.size() << '%' << endl;
	}
	/*
	for (int i = 0; i < PathC.size(); i++){
		for (int j = 0; j < PathC.size(); j++)
			cout << conf[i][j] << endl;		
	}	
	*/
	cout << "Start Choosing Point" << endl;
	ChooseVertexWithGreedyMDS(year);
	string s;
	fstream fileres;
	while (PathC.size() > 0){
		cout << PathC.size() << endl;
		GenerateSAT("sat.cnf", year);
		CallSatAndReadReport();		
		fileres.open("temp.sat");
		getline(fileres, s);
		fileres.close();
		if (s.find("UNSAT") == string::npos)
			break;
		for (int i = 0; i < PathC.size(); i++){
			if (PathC[i]->Is_Chosen()){
				PathC[i]->SetChoose(false);
				break;
			}
		}
	}
	cout << "Q = " << CalQuality(year) << endl;
	cout << "Try to Refine Result : " << endl;
	for (int i = 1; i <= 5; i++){
		if (!RefineResult(year)){
			cout << "Result is in limit!" << endl;
			break;
		}
		cout << "Time " << i << " : " << endl;
		if (!CallSatAndReadReport()){
			cout << "Can't Refine Anymore" << endl;
			break;
		}
		cout << "Q = " << CalQuality(year) << endl;
	}	
	return 0;
}