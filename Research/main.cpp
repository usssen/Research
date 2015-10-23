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
double **EdgeA;	//y = ax+b 不能直接宣告[20000][20000],會爆(連續記憶體不能過2G),要用malloc
double **EdgeB;
double **cor;	//correlation coefficient
vector<PATH*> PathC;

int main(int argc, char* argv[]){
	//if (argc < 3)
	//	return 0;
	string filename;
	filename = argv[1];
	ReadCircuit(filename);
	cout << "Reading Circuit Finished." << endl;
	cout << "Longest Path File Name : " << endl;
	filename = argv[2];
	Circuit[0].PutClockSource();
	ReadPath_l(filename);
	cout << "Read Longest Path Finished."<<endl;
	//lfilename = "testps.rpt";
	//cout << "Shortest Path File Name : " << endl;
	//cin >> filename;
	//ReadPath_s(filename);
	//cout << "Read Shortest Path Finished." << endl;	
	int year = atoi(argv[3]);
	ReadAgingData();
	CheckPathAttackbility(year);
	if (PathC.size() <= 0){
		return 0;
	}
	int m, n, i;	
	double* pData;
	m = n = PathC.size();
	EdgeA = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));	//m個陣列開頭+m*n個空間
	for (i = 0, pData = (double *)(EdgeA + m); i < m; i++, pData += n)	//前m(double*)個為陣列開頭,之後每個開頭(edgea[i])的位置相隔n(double)
		EdgeA[i] = pData;
	EdgeB = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));
	for (i = 0, pData = (double *)(EdgeB + m); i < m; i++, pData += n)
		EdgeB[i] = pData;
	cor = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));
	for (i = 0, pData = (double *)(cor + m); i < m; i++, pData += n)
		cor[i] = pData;	

	srand(time(NULL));
	for (int i = 0; i < m; i++){
		for (int j = i; j < n; j++){
			if (j == i){	//y = x
				EdgeA[i][j] = 1;
				EdgeB[i][j] = 0;
				cor[i][j] = 0;
				continue;
			}
			double a = (double)rand() / RAND_MAX*0.3 + 0.85;	//0.8~1.2
			double b = 0.0;// (double)rand() / RAND_MAX - 0.5;	//-0.5~0.5
			double c = (double)rand() * 2 / RAND_MAX - 1;
			EdgeA[i][j] = a;		// y = ax+b
			EdgeB[i][j] = b;
			EdgeA[j][i] = 1 / a;	//	x = y/a-b/a
			EdgeB[j][i] = -1*(b / a);
			cor[i][j] = cor[j][i] = c;
		}
	}
	
	ChooseVertexWithGreedyMDS();
	string s;
	fstream filer;
	while (PathC.size() > 0){
		cout << PathC.size() << endl;
		GenerateSAT("sat.cnf", year);
		CallSatAndReadReport();		
		filer.open("temp.sat");
		getline(filer, s);
		filer.close();
		if (s.find("UNSAT") == string::npos)
			break;
		PathC.pop_back();
	}
	cout << "Q = " << CalQuality(year) << endl;
	return 0;
}