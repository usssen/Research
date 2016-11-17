#include<iostream>
#include<fstream>
#include<string>
#include<time.h>
#include<stdlib.h>
#include "circuit.h"
#include "aging.h"
#include<signal.h>
#include<algorithm>


/*
void inthandler(int s){
	CallSatAndReadReport(1);
	exit(1);
}
*/

using namespace std;
vector<CIRCUIT> Circuit;
vector<PATH> PathR;		//�Ҧ���path
double **EdgeA;
double **EdgeB;
double **cor;
double **ser;
double info[5];
vector<PATH*> PathC;	//candidate ��path
double ERROR = 1.0;		//�i�e�Ԫ�lifetime�~�t(��ӦW)

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
	clock_t tst, tsolst;
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

	for (int i = 0; i<Circuit[0].GateSize(); i++)
		Circuit[0].GetGate(i)->SetDcc(DCC_NONE);	
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
			else if (line.find("fixed") != string::npos){
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
	cor = new double*[ss];			//�����Y��	
	ser = new double*[ss];			//�зǻ~

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
	int trylimit = atoi(argv[5]), tryi = 0;
	do{
		for (; tryi < trylimit; tryi++){
			tsolst = clock();
			cout << "Round : " << tryi << endl;
			if (!ChooseVertexWithGreedyMDS(year, false)){
				cout << "Not Domination Set!" << endl;
				ChooseVertexWithGreedyMDS(year, true);	//��shortlist��function, �^�ǭ�false�ɪ�S��domination set
				continue;
			}
			GenerateSAT("sat.cnf", year);				//����CNF��F��
			int oridccs = CallSatAndReadReport(0);		//�ΨөI�sSAT tool�ñNŪ�����G, input 0: �@���� 1:�̨θ�, �^�ǭȬ�DCC���ƶq
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
			for (int i = 0; i < PathC.size(); i++)			//�յۥ[path�ishortlist�ӧﵽ���G
				PathC[i]->SetTried(PathC[i]->Is_Chosen());
			for (int i = 0; i < atoi(argv[6]); i++){		//�Ӧh���i��y���[�J�L�h�I���ѨS�t�h�� �ӳy���h�l��DCC
				int AddNodeIndex = RefineResult(year);
				if (AddNodeIndex < 0)
					break;
				PathC[AddNodeIndex]->SetTried(true);			//�O�����I�w�g�յۥ[�L�F
				int dccs = CallSatAndReadReport(0);
				if (!dccs){
					PathC[AddNodeIndex]->SetChoose(false);
					continue;
				}
				oridccs = dccs;
			}
			GenerateSAT("sat.cnf", year);

			system("cp sat.cnf backup.cnf");
			RemoveRDCCs();									//�ΨӸյ۱qMine�W����DCC
			int dccss = CallSatAndReadReport(0);
			if (dccss == 0 || oridccs < dccss){
				system("cp backup.cnf sat.cnf");
			}

			int dccs = CallSatAndReadReport(0);
			double upper, lower;
			CalQuality(year, upper, lower);					//�p��quality
			if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year, bestdcc, bestdcc, dccs)){	//�ݬO�_�Ѫ��϶���쥻���n
				if (monte_s)
					Monte_CalQuality(year, monteU, monteL);	//Monte-Carlo���p
				for (int i = 0; i < PathC.size(); i++)
					bestnode[i] = PathC[i]->Is_Chosen();
				system("cp sat.cnf best.cnf");
			}
			cout << "Q = " << upper << " ~ " << lower << endl;
			cout << "BEST Q = " << bestup << " ~ " << bestlow << endl;
			for (int i = 0; i < atoi(argv[6]); i++){
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
		if (bestup > 10){
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
	do{												//�����e�h�մX��
		double upper, lower;
		CalQuality(year, upper, lower);
		if (BInv(bestup, bestlow, bestup, bestlow, upper, lower, year, bestdcc, bestdcc, dccs)){
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
	//rec << bestup << " " << bestlow << ' ';
	if (monte_s){
		cout << "BEST Q(Monte) = " << monteU << " ~ " << monteL << endl;
	}
	cout << "Clock Period = " << info[0] << endl;
	//rec << info[0] << ' ';
	for (int i = 1; i <= 4; i++){
		cout << info[i] << ' ';
		//rec << info[i] << ' ';
	}
	cout << endl;
	//cout << fr << endl;
	cout << "Total Runtime : " << (clock() - tst) / CLOCKS_PER_SEC << endl;
	cout << "Solution count = " << c_sol << " Time = " << t_sol << endl;
	cout << "No solution count = " << c_nosol << " Time = " << t_nosol << endl;
	//fstream rec;
	//rec.open("rec2.txt", ios::out);
	//rec << bestup << ' ' << bestlow << endl;
	//double sumu, suml;
	vector<double> lff;
	lff.clear();
	for (int i = 0; i < 100000; i++){
		AdjustProcessVar();		
		//double upp, loww;
		//CalQuality(year, upp, loww);
		//for (int j = 0; j < 1000; j++){
			lff.push_back(Monte_CalQualityS(year));
		//}		
		//sumu += upp;
		//suml += loww;
		//rec << upp << " " << loww << endl;
	}
	sort(lff.begin(), lff.end());
	int front = 0, back = lff.size() - 1;
	double upp = lff[front], loww = lff[back];
	while (front + lff.size() - 1 - back <= lff.size() / 20){
		if (absf(lff[front] - (double)year) > absf(lff[back] - (double)year))
			upp = lff[++front];
		else
			loww = lff[--back];
	}
	//rec << sumu / 100 << ' ' << suml / 100 << endl;
	//rec.close();
	cout << upp << ' ' << loww << endl;
	cout << "Enter the year : " << endl;
	double y;
	cin >> y;
	//PrintStatus(y);	
	return 0;
}